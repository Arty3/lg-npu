# Register Map

This document defines the MMIO register interface exposed by `npu_shell` to
the host. All registers are 32-bit, naturally aligned. The MMIO address space
is 20 bits wide (1 MB).

Source of truth: `include/pkg/npu_addrmap_pkg.sv`.

---

## Address Regions

| Region | Base | Size | Description |
|--------|------|------|-------------|
| Registers | `0x0_0000` | 256 B | Control, status, IRQ, perf counters |
| Command queue | `0x0_1000` | 64 B | Single 16-word command descriptor window |
| Weight buffer | `0x1_0000` | 4 KB | INT8 weight SRAM (MMIO window) |
| Activation buffer | `0x2_0000` | 8 KB | INT8 activation SRAM (MMIO window) |
| Psum buffer | `0x3_0000` | 16 KB | INT32 partial-sum SRAM (MMIO window) |

---

## Control / Status Registers

| Offset | Name | R/W | Description |
|--------|------|-----|-------------|
| `0x0000` | `CTRL` | RW | Control register |
| `0x0004` | `STATUS` | RO | Status register |
| `0x0008` | `DOORBELL` | WO | Write triggers command fetch (value ignored, reads 0) |
| `0x000C` | `IRQ_STATUS` | RO | Interrupt pending flag |
| `0x0010` | `IRQ_ENABLE` | RW | Interrupt enable mask |
| `0x0014` | `IRQ_CLEAR` | WO | Write clears pending interrupt (reads 0) |
| `0x0018` | `FEATURE_ID` | RO | Hardware feature identification |
| `0x0020` | `PERF_CYCLES` | RO | Total clock cycles since last clear |
| `0x0024` | `PERF_ACTIVE` | RO | Backend-active cycles |
| `0x0028` | `PERF_STALL` | RO | Backend-stall cycles |

---

## CTRL Register (`0x0000`)

| Bit | Name | Description |
|-----|------|-------------|
| 0 | `SOFT_RESET` | Write 1 to trigger a single-cycle reset pulse (self-clearing) |
| 1 | `ENABLE` | NPU enable (reserved for future gating) |
| 31:2 | — | Reserved (read-as-zero) |

---

## STATUS Register (`0x0004`)

| Bit | Name | Description |
|-----|------|-------------|
| 0 | `IDLE` | 1 when backend is idle and command queue is empty |
| 1 | `BUSY` | 1 when backend is actively computing |
| 2 | `QUEUE_FULL` | 1 when command queue cannot accept new entries |
| 31:3 | — | Reserved |

---

## IRQ_STATUS Register (`0x000C`)

| Bit | Name | Description |
|-----|------|-------------|
| 0 | `PENDING` | 1 when an interrupt event has occurred |
| 31:1 | — | Reserved |

An interrupt event is latched by command completion (`cmd_done`) or error.
The `irq_out` pin is asserted when `PENDING & IRQ_ENABLE[0]`. Write any
value to `IRQ_CLEAR` to deassert.

---

## FEATURE_ID Register (`0x0018`)

| Bits | Field | Current Value | Description |
|------|-------|---------------|-------------|
| 31:24 | `VERSION_MAJOR` | `0x00` | Major version |
| 23:16 | `VERSION_MINOR` | `0x01` | Minor version |
| 15:12 | `NUM_BACKENDS` | `0x1` | Number of compute backends |
| 11:8 | `ARRAY_ROWS` | `0x1` | PE array row count |
| 7:4 | `ARRAY_COLS` | `0x1` | PE array column count |
| 3:0 | `DTYPES` | `0x1` | Supported data types (bit 0 = INT8) |

---

## Command Queue Window (`0x1000`-`0x103F`)

A 64-byte (16 x 32-bit word) window where the host writes a command
descriptor before ringing the doorbell. See
[command_format.md](command_format.md) for the word layout.

---

## Buffer Windows

The host accesses tensor data through direct MMIO windows that map into the
on-chip SRAM banks. Each write/read is a single 32-bit MMIO transaction; for
INT8 buffers only the low byte is significant.

| Window | Base | Backing SRAM | Element Width |
|--------|------|-------------|---------------|
| Weight | `0x1_0000` | `npu_weight_buffer` | 8-bit |
| Activation | `0x2_0000` | `npu_act_buffer` | 8-bit |
| Psum | `0x3_0000` | `npu_psum_buffer` | 32-bit |

Reads from unallocated register offsets return zero.