# Compute Dataflow

This document describes the execution model and data movement for the
lg-npu compute backends: 2D convolution (`rtl/backends/conv/`), general
matrix multiply (`rtl/backends/gemm/`), softmax (`rtl/backends/softmax/`),
element-wise vector operations (`rtl/backends/vec/`), and layer
normalisation (`rtl/backends/lnorm/`).

All data types are INT8 unless stated otherwise. See
[overview.md](overview.md) for the full system context.

---

## Convolution Parameters

A `CONV` command descriptor carries the following parameters:

| Symbol | Meaning |
|--------|---------|
| N | Batch size (always 1 in current scope) |
| H, W | Input activation spatial height and width |
| C | Number of input channels |
| K | Number of output channels (filters) |
| R, S | Filter height and width |
| stride_h, stride_w | Vertical and horizontal stride |
| pad_h, pad_w | Zero-padding applied to the input |
| OH, OW | Output spatial height and width (derived: `OH = (H + 2·pad_h − R) / stride_h + 1`) |

All tensors live in local SRAM in NHWC layout (channel-last). This means a
single element at position (n, h, w, c) sits at byte offset
`((n·H + h)·W + w)·C + c` relative to the tensor base address.

---

## Data Layout in SRAM

| Buffer | Contents | Element type | Logical shape |
|--------|----------|-------------|---------------|
| Weight buffer | Filter kernels + bias values | INT8 | K x R x S x C weights, then K bias values |
| Activation buffer (input) | Input feature map | INT8 | 1 x H x W x C |
| Activation buffer (output) | Output feature map | INT8 | 1 x OH x OW x K |
| Partial-sum buffer | Accumulator state | INT32 | 1 x OH x OW x K |

Input and output activations share the same physical activation buffer at
different base-address offsets. The command descriptor specifies both offsets
so they do not overlap.

---

## Loop Nest

`conv_ctrl` implements the convolution as a set of nested loops. The
canonical loop order is:

```
for oh in 0 .. OH-1:          // output row
  for ow in 0 .. OW-1:        // output column
    for k in 0 .. K-1:        // output channel
      acc[oh][ow][k] = 0      // INT32
      for r in 0 .. R-1:      // filter row
        for s in 0 .. S-1:    // filter column
          for c in 0 .. C-1:  // input channel
            ih = oh * stride_h + r - pad_h
            iw = ow * stride_w + s - pad_w
            if (ih, iw) in bounds:
              act = activation[ih][iw][c]   // INT8
              wt  = weight[k][r][s][c]      // INT8
              acc[oh][ow][k] += act * wt    // INT8xINT8 -> INT32
      // post-processing
      acc[oh][ow][k] += bias[k]            // INT32 += sign_ext(INT8)
      acc[oh][ow][k]  = activate(acc, mode) // ReLU / None / Leaky ReLU
      out[oh][ow][k]  = quantize(acc[oh][ow][k]) // INT32 -> INT8
```

The inner three loops (r, s, c) accumulate into a single INT32 partial sum.
Once all input channels are accumulated for a given output pixel and filter,
the post-processing chain runs in place.

---

## Hardware Mapping

### conv_ctrl

Finite-state machine that sequences through the loop nest above. It
generates loop indices (oh, ow, k, r, s, c) and control signals for every
pipeline stage.

```mermaid
stateDiagram-v2
    [*] --> IDLE
    IDLE --> LOAD_CFG : cmd_if valid
    LOAD_CFG --> COMPUTE : parameters latched
    COMPUTE --> COMPUTE : next iteration
    COMPUTE --> DRAIN : last iteration issued
    DRAIN --> DONE : pipeline empty
    DONE --> IDLE : completion handshake
```

| State | Action |
|-------|--------|
| IDLE | Wait for a dispatch command on the `cmd_if`. |
| LOAD_CFG | Latch convolution parameters from the command struct. |
| COMPUTE | Drive the loop counters. Advance one inner iteration per cycle (subject to back-pressure from memory or the PE array). |
| DRAIN | Wait for the last result to exit the post-processing pipeline. |
| DONE | Assert completion handshake back to `npu_dispatch`. Return to IDLE. |

### conv_addr_gen

Combinationally computes SRAM read addresses from the current loop indices:

- **Activation address**: `act_base + ((ih * W) + iw) * C + c`
- **Weight address**: `wt_base + ((k * R + r) * S + s) * C + c`

For out-of-bounds (ih, iw) due to padding, `conv_addr_gen` signals
`zero_pad`, and the loader substitutes a zero value instead of issuing a
memory read.

### conv_loader

Issues read requests to the weight buffer and activation buffer through
`mem_req_rsp_if`. It receives address and zero-pad signals from
`conv_addr_gen`, reads one INT8 weight and one INT8 activation per cycle,
and sends the pair downstream on a `stream_if`.

Addresses from `conv_addr_gen` are **latched inside `conv_loader`** at the
cycle the iteration is accepted (the S_IDLE -> S_REQ transition). This is
necessary because `conv_ctrl` advances its loop counters on the same cycle
as `iter_accept`, which means the combinational address outputs change
before the loader issues memory requests in the following state. The
latched copies (`act_addr_r`, `wt_addr_r`) are used for the actual memory
read addresses.

### conv_line_buffer & conv_window_gen (not instantiated)

`conv_line_buffer` and `conv_window_gen` exist in the source tree but are
**not instantiated** by `conv_backend` in the current implementation. The
loader re-fetches every activation value directly from SRAM.

These modules are intended for a future optimisation: `conv_line_buffer`
would cache recent activation rows to reduce read bandwidth when R or S > 1,
and `conv_window_gen` would assemble the receptive-field window from the
line-buffer output.

### conv_array & conv_pe

`conv_array` is a spatial array of `conv_pe` instances. Each PE performs:

```
acc += act_in * wt_in    // INT8 x INT8 -> INT32 accumulate
```

In the minimal configuration the array is a single PE (1 x 1), processing
one multiply-accumulate per cycle. The array width and height are
compile-time parameters in `npu_cfg_pkg`.

Each PE exposes:

| Port | Direction | Width | Description |
|------|-----------|-------|-------------|
| `act_in` | in | 8 | Signed activation sample |
| `wt_in` | in | 8 | Signed weight sample |
| `acc_in` | in | 32 | Partial sum from previous accumulation |
| `acc_out` | out | 32 | Updated partial sum |
| `valid_in` | in | 1 | Upstream handshake |
| `ready_out` | out | 1 | Upstream handshake |
| `valid_out` | out | 1 | Downstream handshake |
| `ready_in` | in | 1 | Downstream handshake |

### conv_accum

Register file or small SRAM that holds the INT32 partial sums for the
output tile currently being computed. When the inner loop (r, s, c) starts
for a new (oh, ow, k), the accumulator is cleared to zero. Each MAC result
from the PE array updates the accumulator entry. Once the inner loop
completes, the accumulated value is forwarded to post-processing.

### conv_postproc

Pipeline stage that chains:

1. **conv_bias** — Adds a sign-extended INT8 bias (`bias[k]`) to the INT32
   accumulator value.
2. **conv_activation** — Configurable activation selected by `act_mode`:
   - `ACT_NONE` (0, default): passthrough, no modification.
   - `ACT_RELU` (1): `max(0, x)` — clamps negative values to zero.
   - `ACT_LEAKY_RELU` (2): `x >= 0 ? x : x >>> 3` (negative slope alpha = 1/8).
3. **conv_quantize** — Right arithmetic shift by a programmable amount,
   then signed saturation to [-128, +127], producing the final INT8 output.

Each sub-stage is a single pipeline register on the `stream_if`
valid/ready path.

### conv_writer

Takes the INT8 output from `conv_postproc` and issues a write request to the
activation buffer at the computed output address:

```
out_addr = out_base + ((oh * OW) + ow) * K + k
```

A write response from `mem_req_rsp_if` confirms the store.

---

## Pipeline Diagram (Single-PE, Steady State)

```
Cycle   conv_loader    conv_pe (MAC)    conv_postproc    conv_writer
──────  ────────────   ──────────────   ──────────────   ────────────
  t     read act[t]    acc += a·w [t-1]     —                —
  t+1   read wt[t+1]   acc += a·w [t]   bias+relu+q [t-1]   —
  t+2   read act[t+2]  acc += a·w [t+1] bias+relu+q [t]  write [t-1]
  ...
```

With a single PE, throughput is **one MAC per cycle**. The total cycle count
for a convolution is approximately:

$$\text{cycles} \approx OH \times OW \times K \times R \times S \times C + \text{pipeline overhead}$$

### Two-Stage Pipeline Alignment

`conv_backend` uses a two-stage pipeline to align control signals with data:

- **Stage 1 (iter_accept)** — When `conv_ctrl` issues an iteration and the
  loader accepts it, `conv_backend` captures `last_inner`, `acc_clr`,
  `wr_addr`, and `bias_addr` into first-stage registers.
- **Stage 2 (pe_accept)** — When the PE consumes the data pair, the
  first-stage values advance into second-stage registers that feed the
  accumulator and post-processing chain.

This alignment is necessary because the loader takes multiple cycles
(request + wait) before the PE sees valid data. Without the two-stage pipe,
control signals would arrive at the accumulator before (or after) their
corresponding data.

### Back-Pressure

Every stage uses `stream_if` valid/ready handshaking. If the writer stalls
(e.g. SRAM port busy), back-pressure propagates through the post-processing
chain, the PE, and into the loader. No data is lost.

---

## Address-Generation Example

Given a 4 x 4 input with C = 1, a 3 x 3 filter with K = 1, stride = 1,
pad = 0, the output is 2 x 2. The loader issues reads in this order:

```
oh=0, ow=0: act[0,0] wt[0,0,0]  act[0,1] wt[0,0,1]  act[0,2] wt[0,0,2]
            act[1,0] wt[0,1,0]  act[1,1] wt[0,1,1]  act[1,2] wt[0,1,2]
            act[2,0] wt[0,2,0]  act[2,1] wt[0,2,1]  act[2,2] wt[0,2,2]
            -> postproc -> write out[0,0]

oh=0, ow=1: act[0,1] wt[0,0,0]  ...
            -> postproc -> write out[0,1]

oh=1, ow=0: act[1,0] wt[0,0,0]  ...
            -> postproc -> write out[1,0]

oh=1, ow=1: act[1,1] wt[0,0,0]  ...
            -> postproc -> write out[1,1]
```

---

## Interaction with the Rest of the System

1. `npu_dispatch` programs `conv_backend` by pushing a command on `cmd_if`
   that contains base addresses, dimensions, and quantization shift.
2. `conv_backend` drives `mem_req_rsp_if` to read weights and activations
   from `npu_weight_buffer` / `npu_act_buffer` and to write results back to
   `npu_act_buffer`. These requests are routed by `npu_buffer_router`.
3. When the last output pixel has been written and acknowledged,
   `conv_backend` asserts a `done` signal. `npu_completion` captures this
   and notifies the host via `npu_irq_ctrl`.

No DMA or external memory interaction occurs. The host is responsible for
filling the weight and activation buffers over MMIO before submitting the
command, and for reading the results back afterwards.

---

## GEMM Dataflow

The GEMM backend (`rtl/backends/gemm/`) performs general matrix
multiplication: C = A * B + bias, where A is M x K, B is K x N, and bias
is a length-N vector. All matrices are row-major INT8 in local SRAM.

The GEMM backend reuses the convolution PE array, accumulator,
post-processing chain (bias -> activation -> quantize), loader, and writer.
Only the control FSM and address generation are GEMM-specific.

### GEMM Parameters

| Symbol | Meaning |
|--------|---------|
| M | Number of rows in A / rows in C |
| N | Number of columns in B / columns in C |
| K | Number of columns in A / rows in B (reduction dimension) |

### Loop Nest

`gemm_ctrl` implements a 3-deep loop nest:

```
for m in 0 .. M-1:            // output row
  for n in 0 .. N-1:          // output column
    acc = 0                    // INT32
    for k in 0 .. K-1:        // reduction
      acc += A[m, k] * B[k, n]  // INT8 x INT8 -> INT32
    // post-processing
    acc += bias[n]             // INT32 += sign_ext(INT8)
    acc  = activate(acc, mode) // ReLU / None / Leaky ReLU
    C[m, n] = quantize(acc)    // INT32 -> INT8
```

The inner loop (k) accumulates into a single INT32 partial sum. Once the
reduction is complete for a given (m, n), post-processing runs identically
to convolution.

### Address Generation

`gemm_addr_gen` computes SRAM read addresses from the current loop indices:

- **A address**: `a_base + m * K + k` (row-major M x K in activation buffer)
- **B address**: `b_base + k * N + n` (row-major K x N in weight buffer)

No zero-padding is applied; the loader is instantiated with `zero_pad = 0`.

### Output Addressing

- **C address**: `c_base + m * N + n`
- **Bias address**: `bias_base + n` (per-column, unlike convolution's per-filter bias)

### FSM States

```mermaid
stateDiagram-v2
    [*] --> IDLE
    IDLE --> LOAD_CFG : cmd_if valid
    LOAD_CFG --> COMPUTE : parameters latched
    COMPUTE --> COMPUTE : next iteration
    COMPUTE --> DRAIN : last iteration issued
    DRAIN --> DONE : pipeline empty
    DONE --> IDLE : completion handshake
```

### Backend Muxing

`npu_core` uses a 3-bit registered backend selector (`be_sel_r`) to mux
memory port signals between the GEMM, softmax, vec, lnorm, and convolution
backends. The selector is set when a command is accepted by the corresponding
backend and routes all memory request/response signals to the active backend.
Only one backend is active at a time (in-order scheduler).

---

## Softmax Dataflow

The softmax backend (`rtl/backends/softmax/`) computes the softmax function
independently for each row of an M x N input matrix. All data types are
INT8. The output is INT8 in the range [0, 127], representing probabilities
scaled by 127.

Unlike convolution and GEMM, softmax does not use the PE array,
accumulator, or weight buffer. It operates directly on the activation
buffer through a multi-pass FSM with an integrated serial divider.

### Softmax Parameters

| Symbol | Meaning |
|--------|---------|
| M | Number of independent softmax rows (`num_rows`) |
| N | Elements per row (`row_len`) |

### Algorithm

For each of M rows, three passes over the N elements:

```
for row in 0 .. M-1:
  // Pass 1: Find maximum value
  max_val = -128
  for i in 0 .. N-1:
    max_val = max(max_val, input[row * N + i])

  // Pass 2: Compute exp approximations and accumulate sum
  sum = 0
  for i in 0 .. N-1:
    diff = uint8(max_val) - uint8(input[row * N + i])
    exp_val = exp_lut(diff)       // uint16 approximation
    sum += exp_val                // uint32 accumulator

  // Pass 3: Normalize (serial per element)
  for i in 0 .. N-1:
    diff = uint8(max_val) - uint8(input[row * N + i])
    exp_val = exp_lut(diff)
    numer = exp_val * 127         // 23-bit
    output[row * N + i] = clamp(numer / sum, 0, 127)  // INT8
```

### Exp LUT Approximation

`softmax_exp_lut` approximates `exp(-d/32) * 65535` using a two-table
decomposition that avoids a 256-entry ROM:

$$\text{exp\_val} = \frac{\text{exp\_int}[d_{7:5}] \times \text{exp\_frac}[d_{4:0}]}{2^{16}}$$

- `exp_int`: 8 entries, uint16 values of `round(65535 * exp(-k))` for k = 0..7
- `exp_frac`: 32 entries, uint16 values of `round(65535 * exp(-f/32))` for f = 0..31

The product of two 16-bit values gives a 32-bit result; the upper 16 bits
are taken as the final approximation. This requires one 16x16 multiply and
40 total ROM entries.

### Serial Divider

The normalize pass computes `(exp_val * 127) / sum` per element using a
restoring binary divider. The 24-bit numerator is divided by the 32-bit
sum over 24 clock cycles, producing an 8-bit quotient clamped to [0, 127].

### Memory Ports

| Port | Used | Purpose |
|------|------|---------|
| `act_rd` | Yes | Read input elements (all three passes) |
| `out_wr` | Yes | Write output elements (normalize pass) |
| `wt_rd` | No | Tied off |
| `bias_rd` | No | Tied off |

### FSM States

```mermaid
stateDiagram-v2
    [*] --> IDLE
    IDLE --> LOAD_CFG : cmd valid
    LOAD_CFG --> FIND_MAX : parameters latched
    FIND_MAX --> EXP_SUM : max found
    EXP_SUM --> NORM_RD : sum accumulated
    NORM_RD --> NORM_WAIT : read issued
    NORM_WAIT --> NORM_DIV : data received
    NORM_DIV --> NORM_WR : division complete (24 cycles)
    NORM_WR --> NORM_RD : more elements in row
    NORM_WR --> FIND_MAX : next row
    NORM_WR --> DONE : last row complete
    DONE --> IDLE : completion handshake
```

### Cycle Count

Per row: approximately `2 * N + 27 * N` cycles (two pipelined read passes
plus ~27 cycles per element for the serial normalize pass). Total for a
full operation: `M * (29 * N + overhead)`.

---

## Vec Dataflow

The vec backend (`rtl/backends/vec/`) performs element-wise operations on
two input vectors of length N. Source A is read from the activation buffer,
source B from the weight buffer, and the result is written to the activation
buffer.

Unlike convolution and GEMM, vec does not use the PE array, accumulator, or
bias port. It operates directly on the activation and weight buffers
through a simple FSM that processes one element per ~3 cycles.

### Vec Parameters

| Symbol | Meaning |
|--------|---------|
| N | Number of elements (`length`) |
| vec_op | Sub-operation: 0 = ADD, non-zero = MUL |

### Algorithm

```
for i in 0 .. N-1:
  a = act_buffer[a_base + i]     // INT8
  b = wt_buffer[b_base + i]      // INT8
  if vec_op == ADD:
    result = clamp(a + b, -128, 127)   // 9-bit signed -> saturate
  else:  // MUL
    acc = a * b                        // INT8 x INT8 -> INT32
    acc = activate(acc, mode)          // ReLU / None / Leaky ReLU
    acc = acc >>> quant_shift           // arithmetic right shift
    result = clamp(acc, -128, 127)     // saturate to INT8
  act_buffer[out_base + i] = result
```

### Parallel Read Design

Source A and B reside in separate SRAM banks (activation buffer and weight
buffer), so read requests are issued simultaneously. Grant-tracking flags
(`a_gnt_r`, `b_gnt_r`) handle the case where one bank grants before the
other. Similarly, data-tracking flags (`a_val_r`, `b_val_r`) latch each
operand as it arrives. The FSM advances to the compute/write state only
after both operands are valid.

### Memory Ports

| Port | Used | Purpose |
|------|------|---------|
| `act_rd` | Yes | Read source A elements |
| `wt_rd` | Yes | Read source B elements |
| `out_wr` | Yes | Write output elements |
| `bias_rd` | No | Tied off |

### FSM States

```mermaid
stateDiagram-v2
    [*] --> IDLE
    IDLE --> LOAD_CFG : cmd valid
    LOAD_CFG --> READ : parameters latched
    READ --> WAIT : both grants received
    WAIT --> WRITE : both data valid
    WRITE --> READ : more elements
    WRITE --> DONE : last element written
    DONE --> IDLE : completion handshake
```

### Cycle Count

Each element takes approximately 3 cycles (read + wait + write), so the
total cycle count is approximately `3 * N + pipeline overhead`.

---

## LayerNorm Dataflow

The lnorm backend (`rtl/backends/lnorm/`) computes row-wise layer
normalisation on an M x N input matrix. All data types are INT8. The
output is INT8 in the range [-128, 127].

Unlike convolution and GEMM, lnorm does not use the PE array, accumulator,
weight buffer, or bias port. It operates directly on the activation
buffer through a multi-pass FSM with integrated serial divider and serial
integer square root.

### LayerNorm Parameters

| Symbol | Meaning |
|--------|---------|
| M | Number of independent rows (`num_rows`) |
| N | Elements per row (`row_len`) |
| norm_shift | Left-shift applied to (x - mean) before division |

### Algorithm

For each of M rows, three read passes over the N elements plus serial
division and square root phases:

```
for row in 0 .. M-1:
  // Pass 1: Accumulate sum
  sum = 0
  for i in 0 .. N-1:
    sum += input[row * N + i]        // signed INT32

  // Mean division (serial restoring divider, 32 cycles)
  mean = sum / N                     // integer toward zero

  // Pass 2: Accumulate variance
  var_sum = 0
  for i in 0 .. N-1:
    diff = input[row * N + i] - mean
    var_sum += abs(diff) * abs(diff)  // unsigned INT32

  // Variance division (serial restoring divider, 32 cycles)
  variance = var_sum / N

  // Integer square root (serial digit-by-digit, 16 cycles)
  std = isqrt(variance + 1)          // minimum 1

  // Pass 3: Normalize (serial per element)
  for i in 0 .. N-1:
    diff = input[row * N + i] - mean
    scaled = abs(diff) << norm_shift  // saturate to 32 bits
    quot = scaled / std               // serial divider, 32 cycles
    output[row * N + i] = clamp(sign(diff) * quot, -128, 127)
```

### Serial Restoring Divider

All three division phases (mean, variance, normalize) share a single set
of divider registers. The divider computes a 32-bit unsigned quotient from
a 32-bit numerator and denominator over 32 clock cycles, shifting one bit
per cycle from MSB to LSB. A sign flag restores the correct sign for the
mean and normalize divisions. Combinational next-value wires allow the
final quotient to be captured on the same clock edge as the last iteration.

### Serial Integer Square Root

The square root uses the digit-by-digit method, processing 2 bits of the
32-bit input per iteration over 16 clock cycles to produce a 16-bit result.
The result is clamped to a minimum of 1 to prevent division by zero.

### Memory Ports

| Port | Used | Purpose |
|------|------|---------|
| `act_rd` | Yes | Read input elements (all three passes) |
| `out_wr` | Yes | Write output elements (normalize pass) |
| `wt_rd` | No | Tied off |
| `bias_rd` | No | Tied off |

### FSM States

```mermaid
stateDiagram-v2
    [*] --> IDLE
    IDLE --> LOAD_CFG : cmd valid
    LOAD_CFG --> SUM_RD : parameters latched
    SUM_RD --> SUM_WAIT : read issued
    SUM_WAIT --> SUM_RD : more elements
    SUM_WAIT --> MEAN_DIV : last element
    MEAN_DIV --> VAR_RD : division complete (32 cycles)
    VAR_RD --> VAR_WAIT : read issued
    VAR_WAIT --> VAR_RD : more elements
    VAR_WAIT --> VAR_DIV : last element
    VAR_DIV --> SQRT : division complete (32 cycles)
    SQRT --> NORM_RD : sqrt complete (16 cycles)
    NORM_RD --> NORM_WAIT : read issued
    NORM_WAIT --> NORM_DIV : data received
    NORM_DIV --> NORM_WR : division complete (32 cycles)
    NORM_WR --> NORM_RD : more elements in row
    NORM_WR --> SUM_RD : next row
    NORM_WR --> DONE : last row complete
    DONE --> IDLE : completion handshake
```

### Cycle Count

Per row: approximately `4 * N + 35 * N + 80` cycles (two pipelined read
passes for sum and variance, serial mean/variance divisions and sqrt,
plus ~35 cycles per element in the normalize pass for reading, dividing,
and writing). Total for a full operation: `M * (39 * N + 80)`.
