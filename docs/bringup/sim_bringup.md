# Simulation Bringup

This document covers setting up and running RTL simulation for the lg-npu.

---

## Prerequisites

| Tool | Minimum Version | Purpose |
|------|----------------|---------|
| **Verilator** | 5.x | Lint, compile, simulation |
| **GNU Make** | 3.81+ | Build orchestration |
| **Python 3** | 3.8+ | Test vector generation, utilities |
| **GCC / Clang** | C++17 support | Verilator C++ model compilation |

---

## Quick Start

```bash
# 1. Lint check (no compilation)
make lint

# 2. Generate test vectors
make vectors

# 3. Run unit tests
make sim-unit

# 4. Run smoke regression (unit + block)
make sim-smoke
```

---

## Make Targets

| Target | Description |
|--------|-------------|
| `make lint` | Verilator lint (`-Wall`) on all RTL via `tools/lint/rtl.f` |
| `make compile` | Verilate + build C++ model into `sim/build/` |
| `make sim-unit` | Run all testbenches under `tb/unit/` |
| `make sim-block` | Run all testbenches under `tb/block/` |
| `make sim-integration` | Run all testbenches under `tb/integration/` |
| `make sim-e2e` | Compile and run the end-to-end convolution test (`tb/integration/npu_e2e_harness.cpp`) |
| `make sim-smoke` | Run `sim/regressions/smoke.list` (unit + block) |
| `make sim-full` | Run `sim/regressions/full.list` (all levels) |
| `make vectors` | Generate test vectors from Python reference models |
| `make waves` | Open the latest VCD waveform in Surfer viewer |
| `make viz` | Generate architecture diagrams into `docs/diagrams/` |

---

## Simulation Flow

```mermaid
flowchart LR
    RTL["RTL Sources\n(tools/lint/rtl.f)"]
    TB["Testbench\n(tb/)"]
    VER["Verilator"]
    CPP["C++ Model\n(sim/build/)"]
    RUN["Execute"]
    LOG["Results\n(sim/results/)"]

    RTL --> VER
    TB --> VER
    VER --> CPP
    CPP --> RUN
    RUN --> LOG
```

Each sim script (`sim/scripts/run_*.sh`) performs these steps:

1. Discover testbench files in the target directory.
2. Verilate each testbench against the RTL file list.
3. Build and run the C++ executable.
4. Capture output to `sim/results/`.
5. Report PASS / FAIL based on the exit code.

---

## File List

The RTL compilation order is defined in [tools/lint/rtl.f](../../tools/lint/rtl.f).
This file lists all packages, common modules, and design hierarchy in
dependency order. Both lint and simulation share the same file list.

---

## Test Vectors

Reference vectors are generated from Python models:

```bash
make vectors
```

This runs `model/vectors/gen_conv_vectors.py` and
`model/vectors/gen_quant_vectors.py`, placing output files in `tb/vectors/`.
Testbenches load these vectors to compare RTL output against the golden
reference.

---

## Waveform Viewing

When `--trace` is enabled (default in `make compile`), VCD waveform files
are written to `sim/waves/`. The end-to-end test produces `sim/waves/e2e.vcd`.

### Quick open

```bash
make waves          # launches Surfer (via tools/visualize/open_surfer.sh)
```

Alternatively, open manually with GTKWave or any VCD-compatible viewer:

```bash
gtkwave sim/waves/e2e.vcd
```

### Key signals to inspect

| Signal | Location | Purpose |
|--------|----------|---------|
| `mmio_addr`, `mmio_wdata`, `mmio_ready` | `TOP.npu_shell` | Host MMIO traffic |
| `state` | `conv_ctrl` | Backend FSM state |
| `pair_valid`, `act_out`, `wt_out` | `conv_loader` | Data feeding the PE |
| `acc_out` | `conv_pe` | Accumulator output |
| `idle`, `busy` | `npu_status` | Completion status |

---

## Regression Lists

| File | Contents |
|------|----------|
| `sim/regressions/smoke.list` | Unit + block tests (fast sanity) |
| `sim/regressions/full.list` | Unit + block + integration (complete) |

Custom regression lists can be run with:

```bash
bash sim/scripts/run_all.sh path/to/custom.list
```