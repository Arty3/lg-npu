// ============================================================================
// npu_status.sv - Aggregate internal state for the status register
// ============================================================================

module npu_status (
    // Inputs from various blocks
    input  logic backend_busy,
    input  logic queue_empty,
    input  logic queue_full,
    input  logic cmd_pipe_busy,

    // Status outputs (to reg block)
    output logic idle,
    output logic busy,
    output logic q_full
);

    assign idle   = ~backend_busy & queue_empty & ~cmd_pipe_busy;
    assign busy   = backend_busy;
    assign q_full = queue_full;

endmodule : npu_status
