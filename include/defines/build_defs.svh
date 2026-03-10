`ifndef BUILD_DEFS_SVH
`define BUILD_DEFS_SVH

// Target selection (exactly one should be defined externally or via Makefile):
//   `define TARGET_SIM   - simulation (default)
//   `define TARGET_FPGA  - FPGA build
//   `define TARGET_ASIC  - ASIC build

`ifndef TARGET_SIM
`ifndef TARGET_FPGA
`ifndef TARGET_ASIC
    `define TARGET_SIM
`endif
`endif
`endif

// Enable / disable optional blocks
`ifndef NPU_ENABLE_PERF_COUNTERS
    `define NPU_ENABLE_PERF_COUNTERS
`endif

`endif // BUILD_DEFS_SVH
