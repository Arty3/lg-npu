// ============================================================================
// irq_props.sv - IRQ controller contract assertions
// ============================================================================

module irq_props (
    input logic clk,
    input logic rst_n,
    input logic cmd_done,
    input logic err,
    input logic irq_enable,
    input logic irq_clear,
    input logic irq_status,
    input logic irq_out
);

    // Pending latches on cmd_done or err
    a_set_on_event: assert property (
        @(posedge clk) disable iff (!rst_n)
        (cmd_done || err) && !irq_clear |=> irq_status
    );

    // irq_clear deasserts pending (priority over set)
    a_clear_clears: assert property (
        @(posedge clk) disable iff (!rst_n)
        irq_clear |=> !irq_status
    );

    // irq_out equals pending AND enable
    a_out_is_gated: assert property (
        @(posedge clk) disable iff (!rst_n)
        irq_out == (irq_status & irq_enable)
    );

    // After reset, pending is cleared
    a_reset_clears: assert property (
        @(posedge clk)
        !rst_n |=> !irq_status
    );

    // If enable is low, irq_out must be low regardless of pending
    a_out_low_when_disabled: assert property (
        @(posedge clk) disable iff (!rst_n)
        !irq_enable |-> !irq_out
    );

endmodule : irq_props
