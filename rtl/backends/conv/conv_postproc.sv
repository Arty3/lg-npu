// ============================================================================
// conv_postproc.sv - Post-processing pipeline: bias -> ReLU -> quantize
// ============================================================================

module conv_postproc
    import npu_types_pkg::*;
(
    input  logic                       clk,
    input  logic                       rst_n,

    // Configuration
    input  logic signed [DATA_W-1:0]   bias_val,
    input  logic [4:0]                 quant_shift,

    // Stream in  (INT32 accumulator result)
    input  logic signed [ACC_W-1:0]    in_data,
    input  logic                       in_valid,
    output logic                       in_ready,

    // Stream out (INT8 final result)
    output logic signed [DATA_W-1:0]   out_data,
    output logic                       out_valid,
    input  logic                       out_ready
);

    // Bias -> Activation wires
    logic signed [ACC_W-1:0]  bias_out;
    logic                     bias_valid, bias_ready;

    // Activation -> Quantize wires
    logic signed [ACC_W-1:0]  relu_out;
    logic                     relu_valid, relu_ready;

    conv_bias u_bias (
        .clk        (clk),
        .rst_n      (rst_n),
        .bias_val   (bias_val),
        .in_data    (in_data),
        .in_valid   (in_valid),
        .in_ready   (in_ready),
        .out_data   (bias_out),
        .out_valid  (bias_valid),
        .out_ready  (bias_ready)
    );

    conv_activation u_relu (
        .clk        (clk),
        .rst_n      (rst_n),
        .in_data    (bias_out),
        .in_valid   (bias_valid),
        .in_ready   (bias_ready),
        .out_data   (relu_out),
        .out_valid  (relu_valid),
        .out_ready  (relu_ready)
    );

    conv_quantize u_quant (
        .clk         (clk),
        .rst_n       (rst_n),
        .quant_shift (quant_shift),
        .in_data     (relu_out),
        .in_valid    (relu_valid),
        .in_ready    (relu_ready),
        .out_data    (out_data),
        .out_valid   (out_valid),
        .out_ready   (out_ready)
    );

endmodule : conv_postproc
