// ============================================================================
// npu_perf_counters.sv - Performance monitoring counters
// ============================================================================

`include "build_defs.svh"

module npu_perf_counters
    import npu_perf_pkg::*;
(
    input  logic clk,
    input  logic rst_n,

    // Event inputs
    input  logic backend_active,
    input  logic backend_stall,

    // Control
    input  logic clear,

    // Read port
    output logic [PERF_CNT_W-1:0] cnt_cycles,
    output logic [PERF_CNT_W-1:0] cnt_active,
    output logic [PERF_CNT_W-1:0] cnt_stall
);

`ifdef NPU_ENABLE_PERF_COUNTERS

    counter #(.WIDTH(PERF_CNT_W)) u_cycles (
        .clk   (clk),
        .rst_n (rst_n),
        .clr   (clear),
        .en    (1'b1),
        .count (cnt_cycles)
    );

    counter #(.WIDTH(PERF_CNT_W)) u_active (
        .clk   (clk),
        .rst_n (rst_n),
        .clr   (clear),
        .en    (backend_active),
        .count (cnt_active)
    );

    counter #(.WIDTH(PERF_CNT_W)) u_stall (
        .clk   (clk),
        .rst_n (rst_n),
        .clr   (clear),
        .en    (backend_stall),
        .count (cnt_stall)
    );

`else

    assign cnt_cycles = '0;
    assign cnt_active = '0;
    assign cnt_stall  = '0;

`endif

endmodule : npu_perf_counters
