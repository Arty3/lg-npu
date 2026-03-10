# Tensor Layouts

This document describes the tensor data layout used by the NPU.

Source of truth: `include/pkg/npu_tensor_pkg.sv`.

---

## Supported Layouts

| Enum | Value | Description |
|------|-------|-------------|
| `LAYOUT_NHWC` | `2'b00` | N-Height-Width-Channel (channel-last) |

v0.1 supports **NHWC only**. Attempting any other layout is undefined.

---

## NHWC Memory Order

Elements are stored in row-major order with channels varying fastest:

```
for n in 0..N-1:
  for h in 0..H-1:
    for w in 0..W-1:
      for c in 0..C-1:
        mem[base + ((n*H + h)*W + w)*C + c] = tensor[n][h][w][c]
```

### Linear Address Formula

$$
\text{addr} = \text{base} + \bigl((n \cdot H + h) \cdot W + w\bigr) \cdot C + c
$$

---

## Tensor Descriptor

The `tensor_desc_t` struct carries metadata for a tensor:

| Field | Width | Description |
|-------|-------|-------------|
| `base_addr` | `ADDR_W` (16) | SRAM byte address of element [0][0][0][0] |
| `dim_n` | `DIM_W` (16) | Batch dimension (v0.1: always 1) |
| `dim_h` | `DIM_W` (16) | Height |
| `dim_w` | `DIM_W` (16) | Width |
| `dim_c` | `DIM_W` (16) | Channel count |
| `layout` | 2 | `tensor_layout_e` (must be `LAYOUT_NHWC`) |

---

## Data Types

| Tensor | Element Type | Width |
|--------|-------------|-------|
| Input activations | INT8 | 8 bits |
| Weights | INT8 | 8 bits |
| Biases | INT8 | 8 bits |
| Partial sums | INT32 | 32 bits |
| Output activations | INT8 | 8 bits (after quantisation) |

All INT8 values are **signed** two's-complement.