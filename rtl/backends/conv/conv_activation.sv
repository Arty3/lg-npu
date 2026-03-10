// ============================================================================
// conv_activation.sv - ReLU activation on INT32: max(0, x)
// ============================================================================

module conv_activation
    import npu_types_pkg::*;
(
    input  logic                       clk,
    input  logic                       rst_n,

    // Stream in  (INT32)
    input  logic signed [ACC_W-1:0]    in_data,
    input  logic                       in_valid,
    output logic                       in_ready,

    // Stream out (INT32, ReLU applied)
    output logic signed [ACC_W-1:0]    out_data,
    output logic                       out_valid,
    input  logic                       out_ready
);

    logic signed [ACC_W-1:0] relu;
    assign relu = (in_data < 0) ? '0 : in_data;

    pipeline_reg #(.DATA_W(ACC_W)) u_reg (
        .clk       (clk),
        .rst_n     (rst_n),
        .in_data   (relu),
        .in_valid  (in_valid),
        .in_ready  (in_ready),
        .out_data  (out_data),
        .out_valid (out_valid),
        .out_ready (out_ready)
    );

endmodule : conv_activation
