# LG-NPU

## Description

**lg-npu** (Luca Goddijn Neural Processing Unit) is an experimental open
hardware accelerator for neural network inference.

The project aims to design a modular neural processing unit that can be
implemented on FPGA for development and validation, with a long-term goal of
supporting ASIC implementation.

The architecture focuses initially on efficient convolution workloads and
is designed around a clear separation between:

- a software-controlled execution model
- a modular compute backend
- a shared memory and data movement infrastructure

The project is structured to support full hardware/software co-design,
including:

- SystemVerilog RTL implementation
- simulation and verification infrastructure
- Python reference models for correctness
- a software runtime and driver interface
- FPGA and ASIC integration flows

The initial development target is a convolution-first accelerator with
INT8 inference support, designed to execute small CNN workloads efficiently
while remaining extensible for future compute backends such as GEMM or
attention engines.

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
| `conv_dataflow.md` | Description of the convolution execution model and dataflow through the compute array. |
| `memory_hierarchy.md` | Overview of the on-chip memory system, buffers, and data movement strategy. |
| `programming_model.md` | Conceptual model for how software interacts with the NPU (commands, execution flow, completion). |
| `fpga_plan.md` | FPGA bring-up strategy and expected integration architecture. |
| `asic_plan.md` | Long-term plan for ASIC synthesis, layout, and physical design considerations. |

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
| `sw/` | Driver and runtime interface for the NPU. |
| `fpga/` | FPGA integration and board support. |
| `asic/` | ASIC synthesis, layout, and sign-off flow. |
