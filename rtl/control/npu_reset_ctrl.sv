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
    logic soft_reset_sync1;
    logic soft_reset_sync2;
    logic rst_n_int;

    reset_sync #(.STAGES(2)) u_sync (
        .clk         (clk),
        .rst_async_n (rst_async_n),
        .rst_sync_n  (hard_rst_n)
    );

    // Synchronize software reset release to avoid reset-tree issues.
    always_ff @(posedge clk or negedge hard_rst_n) begin
        if (!hard_rst_n) begin
            soft_reset_sync1 <= 1'b1;
            soft_reset_sync2 <= 1'b1;
        end else begin
            soft_reset_sync1 <= soft_reset;
            soft_reset_sync2 <= soft_reset_sync1;
        end
    end

    assign rst_n_int = hard_rst_n & ~soft_reset_sync2;
    assign rst_n     = rst_n_int;

endmodule : npu_reset_ctrl
