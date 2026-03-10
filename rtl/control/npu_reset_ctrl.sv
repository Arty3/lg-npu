// ============================================================================
// npu_reset_ctrl.sv - Reset controller (hard + soft reset)
// ============================================================================

module npu_reset_ctrl (
    input  logic clk,
    input  logic rst_async_n,   // Platform hard reset
    input  logic soft_reset,    // Software-triggered reset from reg block
    output logic rst_n          // Combined synchronous reset (active low)
);

    logic hard_rst_n;

    reset_sync #(.STAGES(2)) u_sync (
        .clk         (clk),
        .rst_async_n (rst_async_n),
        .rst_sync_n  (hard_rst_n)
    );

    assign rst_n = hard_rst_n & ~soft_reset;

endmodule : npu_reset_ctrl
