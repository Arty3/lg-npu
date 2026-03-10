// ============================================================================
// conv_array.sv - Spatial PE array (ARRAY_ROWS x ARRAY_COLS)
//   Current scope: 1x1 (single PE).
// ============================================================================

module conv_array
    import npu_types_pkg::*;
    import npu_cfg_pkg::*;
(
    input  logic                      clk,
    input  logic                      rst_n,

    // Single PE interface (1x1 array)
    input  logic signed [DATA_W-1:0]  act_in,
    input  logic signed [DATA_W-1:0]  wt_in,
    input  logic signed [ACC_W-1:0]   acc_in,
    output logic signed [ACC_W-1:0]   acc_out,

    input  logic                      valid_in,
    output logic                      ready_out,
    output logic                      valid_out,
    input  logic                      ready_in
);

    conv_pe u_pe (
        .clk       (clk),
        .rst_n     (rst_n),
        .act_in    (act_in),
        .wt_in     (wt_in),
        .acc_in    (acc_in),
        .acc_out   (acc_out),
        .valid_in  (valid_in),
        .ready_out (ready_out),
        .valid_out (valid_out),
        .ready_in  (ready_in)
    );

endmodule : conv_array
