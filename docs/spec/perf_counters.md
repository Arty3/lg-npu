# Performance Counters

Three 32-bit hardware performance counters are available for profiling.

Source of truth: `rtl/control/npu_perf_counters.sv`,
`include/pkg/npu_perf_pkg.sv`.

---

## Conditional Compilation

The counters are gated by the preprocessor define
`` `NPU_ENABLE_PERF_COUNTERS `` (set in `include/defines/build_defs.svh`).
When **not** defined, the counter registers read as zero and the counting
logic is synthesised away.

---

## Registers

| Offset | Name | Description |
|--------|------|-------------|
| `0x0020` | `PERF_CYCLES` | Free-running clock-cycle counter |
| `0x0024` | `PERF_ACTIVE` | Cycles where the backend is actively computing |
| `0x0028` | `PERF_STALL` | Cycles where the backend is stalled |

All three are read-only from the host. Register indices within the counter
array follow `npu_perf_pkg`:

| Index | Constant | Counter |
|-------|----------|---------|
| 0 | `CTR_CYCLES` | `PERF_CYCLES` |
| 1 | `CTR_ACTIVE` | `PERF_ACTIVE` |
| 2 | `CTR_STALL` | `PERF_STALL` |

---

## Counting Rules

| Counter | Increments When |
|---------|-----------------|
| `PERF_CYCLES` | Every `clk` rising edge (free-running) |
| `PERF_ACTIVE` | `backend_active` is asserted |
| `PERF_STALL` | `backend_stall` is asserted |

---

## Clear Behaviour

All three counters are cleared to zero by:

- Hard reset (`rst_n` assertion)
- Soft reset (`CTRL[0] = 1`)

There is no explicit "counter clear" register; a soft reset pulse clears
both the counters and the rest of the NPU state.

---

## Typical Usage

```text
1. Write CTRL = 0x02          # ENABLE, SOFT_RESET off -> counters start from 0
2. Submit commands …
3. Wait for completion IRQ
4. Read PERF_CYCLES, PERF_ACTIVE, PERF_STALL
5. Compute utilisation = PERF_ACTIVE / PERF_CYCLES
```