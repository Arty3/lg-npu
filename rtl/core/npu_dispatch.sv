// ============================================================================
// npu_dispatch.sv - Forwards command to the convolution backend
// ============================================================================

module npu_dispatch
    import npu_types_pkg::*;
    import npu_cmd_pkg::*;
(
    // From scheduler
    input  conv_cmd_t sched_cmd,
    input  logic      sched_valid,
    output logic      sched_ready,

    // To conv backend
    output conv_cmd_t be_cmd,
    output logic      be_cmd_valid,
    input  logic      be_cmd_ready
);

    // Direct pass-through (in-order, single backend)
    assign be_cmd       = sched_cmd;
    assign be_cmd_valid = sched_valid;
    assign sched_ready  = be_cmd_ready;

endmodule : npu_dispatch
