// ============================================================================
// status_props.sv - Status register and busy/done/reset contract assertions
// ============================================================================

module status_props (
    input logic clk,
    input logic rst_n,
    input logic backend_busy,
    input logic queue_empty,
    input logic cmd_pipe_busy,
    input logic idle,
    input logic busy,
    input logic q_full,
    input logic queue_full
);

    // idle and busy are mutually exclusive
    a_idle_busy_mutex: assert property (
        @(posedge clk) disable iff (!rst_n)
        !(idle && busy)
    );

    // idle definition: nothing in flight
    a_idle_def: assert property (
        @(posedge clk) disable iff (!rst_n)
        idle == (!backend_busy && queue_empty && !cmd_pipe_busy)
    );

    // busy mirrors backend_busy
    a_busy_def: assert property (
        @(posedge clk) disable iff (!rst_n)
        busy == backend_busy
    );

    // q_full mirrors queue_full
    a_qfull_def: assert property (
        @(posedge clk) disable iff (!rst_n)
        q_full == queue_full
    );

    // After reset, backend should not be busy
    a_reset_not_busy: assert property (
        @(posedge clk)
        !rst_n |=> !busy
    );

    // After reset, queue should be empty (therefore idle)
    a_reset_idle: assert property (
        @(posedge clk)
        !rst_n |=> idle
    );

endmodule : status_props
