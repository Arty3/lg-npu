// ============================================================================
// npu_dispatch.sv - Routes command to backend by opcode
//   Priority: GEMM > Softmax > Vec > LayerNorm > Convolution (default).
// ============================================================================

module npu_dispatch
    import npu_types_pkg::*;
    import npu_cmd_pkg::*;
(
    // From scheduler
    input  conv_cmd_t sched_cmd,
    input  logic      sched_valid,
    output logic      sched_ready,

    // To GEMM backend (checked first)
    output conv_cmd_t gemm_cmd,
    output logic      gemm_cmd_valid,
    input  logic      gemm_cmd_ready,

    // To softmax backend
    output conv_cmd_t softmax_cmd,
    output logic      softmax_cmd_valid,
    input  logic      softmax_cmd_ready,

    // To vec backend
    output conv_cmd_t vec_cmd,
    output logic      vec_cmd_valid,
    input  logic      vec_cmd_ready,

    // To lnorm backend
    output conv_cmd_t lnorm_cmd,
    output logic      lnorm_cmd_valid,
    input  logic      lnorm_cmd_ready,

    // To conv backend (default)
    output conv_cmd_t conv_cmd,
    output logic      conv_cmd_valid,
    input  logic      conv_cmd_ready
);

    assign gemm_cmd    = sched_cmd;
    assign softmax_cmd = sched_cmd;
    assign vec_cmd     = sched_cmd;
    assign lnorm_cmd   = sched_cmd;
    assign conv_cmd    = sched_cmd;

    always_comb begin
        gemm_cmd_valid    = 1'b0;
        softmax_cmd_valid = 1'b0;
        vec_cmd_valid     = 1'b0;
        lnorm_cmd_valid   = 1'b0;
        conv_cmd_valid    = 1'b0;
        sched_ready       = 1'b0;

        if (sched_cmd.opcode == OP_GEMM) begin
            gemm_cmd_valid = sched_valid;
            sched_ready    = gemm_cmd_ready;
        end else if (sched_cmd.opcode == OP_SOFTMAX) begin
            softmax_cmd_valid = sched_valid;
            sched_ready       = softmax_cmd_ready;
        end else if (sched_cmd.opcode == OP_VEC) begin
            vec_cmd_valid = sched_valid;
            sched_ready   = vec_cmd_ready;
        end else if (sched_cmd.opcode == OP_LNORM) begin
            lnorm_cmd_valid = sched_valid;
            sched_ready     = lnorm_cmd_ready;
        end else begin
            conv_cmd_valid = sched_valid;
            sched_ready    = conv_cmd_ready;
        end
    end

endmodule : npu_dispatch
