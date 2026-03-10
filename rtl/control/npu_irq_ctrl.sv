// ============================================================================
// npu_irq_ctrl.sv - Interrupt controller
//   Level-sensitive interrupt, cleared by software writing to IRQ_CLEAR.
// ============================================================================

module npu_irq_ctrl (
    input  logic clk,
    input  logic rst_n,

    // Event sources
    input  logic cmd_done,      // Pulse: command completed
    input  logic err,           // Pulse: error (reserved)

    // Register interface
    input  logic irq_enable,    // From IRQ_ENABLE register
    input  logic irq_clear,     // Pulse from IRQ_CLEAR register write
    output logic irq_status,    // To IRQ_STATUS register

    // External interrupt pin
    output logic irq_out
);

    logic pending;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            pending <= 1'b0;
        end else begin
            if (irq_clear)
                pending <= 1'b0;
            else if (cmd_done || err)
                pending <= 1'b1;
        end
    end

    assign irq_status = pending;
    assign irq_out    = pending & irq_enable;

endmodule : npu_irq_ctrl
