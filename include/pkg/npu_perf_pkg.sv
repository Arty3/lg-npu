// ============================================================================
// npu_perf_pkg.sv - Performance counter definitions
// ============================================================================

package npu_perf_pkg;

    // Counter indices
    /* verilator lint_off UNUSEDPARAM */
    localparam int PERF_NUM_COUNTERS  = 3;
    localparam int PERF_IDX_CYCLES    = 0;  // Total clock cycles (free-running)
    localparam int PERF_IDX_ACTIVE    = 1;  // Cycles the backend is computing
    localparam int PERF_IDX_STALL     = 2;  // Cycles the backend is stalled
    /* verilator lint_on UNUSEDPARAM */

    // Counter width
    localparam int PERF_CNT_W = 32;

endpackage : npu_perf_pkg
