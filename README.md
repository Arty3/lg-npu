# LG-NPU

## Description

**lg-npu** (Luca Goddijn Neural Processing Unit) is an experimental open
hardware accelerator for neural network inference.

The project aims to design a modular neural processing unit that can be
implemented on FPGA for development and validation, with a long-term goal of
supporting ASIC implementation.

The architecture supports six compute kernels and
is designed around a clear separation between:

- a software-controlled execution model
- modular compute backends and composites
- a shared memory and data movement infrastructure

The project is structured to support full hardware/software co-design,
including:

- SystemVerilog RTL implementation
- simulation and verification infrastructure
- Python reference models for correctness
- a software runtime and driver interface
- FPGA and ASIC integration flows

The accelerator targets INT8 inference, supporting convolution, general
matrix multiply (GEMM), softmax, element-wise vector operations, layer
normalisation, and 2D spatial pooling.

---

## Documentation

The `docs/` directory contains architecture descriptions, hardware/software
contracts, and development guides for the `lg-npu` project.

Documentation is divided into three main categories: architecture,
specifications, and bring-up guides.

### Architecture (`docs/arch/`)

High-level design documents describing how the NPU is structured and how its
components interact.

| File | Description |
|-----|-------------|
| `overview.md` | High-level overview of the lg-npu architecture and design goals. |
| `compute_dataflow.md` | Description of the compute execution model and dataflow through backends and composites. |
| `memory_hierarchy.md` | Overview of the on-chip memory system, buffers, and data movement strategy. |
| `programming_model.md` | Conceptual model for how software interacts with the NPU (commands, execution flow, completion). |

---

### Hardware / Software Specification (`docs/spec/`)

Precise documentation defining the interface between hardware and software.
These documents form the **contract** that both the RTL and driver/runtime
must follow.

| File | Description |
|-----|-------------|
| `register_map.md` | Memory-mapped register layout exposed to the host. |
| `command_format.md` | Structure and semantics of command descriptors submitted to the NPU. |
| `tensor_layouts.md` | Supported tensor memory layouts and dimension ordering. |
| `interrupts.md` | Interrupt and completion signaling behavior. |
| `reset_boot_flow.md` | Device initialization, reset behavior, and boot sequence. |
| `perf_counters.md` | Performance monitoring counters and profiling support. |

---

### Development & Bring-Up (`docs/bringup/`)

Guides for running simulations, testing the hardware, and bringing the system
up on real hardware.

| File | Description |
|-----|-------------|
| `sim_bringup.md` | Instructions for running simulations and verification environments. |
| `fpga_bringup.md` | Steps required to build and run the design on an FPGA platform. |

---

### Relationship to the Source Tree

These documents correspond closely with the main project components:

| Directory | Purpose |
|----------|--------|
| `rtl/` | Hardware implementation of the NPU. |
| `tb/` | Verification testbenches and simulation infrastructure. |
| `model/` | Reference software models used for validation. |
| `sw/uapi/` | Public C headers (device context, commands, tensors, registers). |
| `sw/runtime/` | Runtime library: MMIO, IRQ, DMA, command builders, tensor conversion. |
| `sw/driver/` | Kernel-level driver stubs (future). |
| `sw/shared/` | Cross-platform annotation macros (`annotations.h`). |
| `fpga/` | FPGA integration and board support. |
| `asic/` | ASIC synthesis, layout, and sign-off flow. |
| `tools/` | Linting, formatting, code generation, and utility scripts. |

---

## Quick Start

### Prerequisites

- [Verilator](https://verilator.org/) 5.x (`verilator --version`)
- GNU Make
- Bash (for simulation scripts)
- Python 3.9+ (for generators and reference models)

### Build Targets

Run `make help` to list all targets. The most common ones:

```bash
make lint              # Verilator lint (zero-warning gate)
make compile           # Verilate -> C++ model
make compile-full-tests # Build full regression binary
make sim-smoke         # Smoke regression (conv + control + perf)
make sim-full          # Full regression (all test suites)
make sw-check          # Syntax-check runtime C sources (-fsyntax-only)
make sw-build          # Build liblgnpu_rt.a and liblgnpu_rt.so
make vectors           # Generate test vectors from Python models
make format            # Auto-format SV / shell / Python
make gen               # Regenerate SV packages + C headers
make viz               # Generate architecture diagrams
make waves             # Open latest waveform in Surfer viewer
make clean             # Remove build artifacts
```

### Running Lint

```bash
make lint
```

This invokes Verilator with `-Wall` on all RTL via
`tools/lint/rtl.f`. The design must pass with zero warnings.