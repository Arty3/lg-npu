// ============================================================================
// conv_window_gen.sv - Window generator (stub)
//   In the initial scope the controller + addr_gen directly iterate the
//   receptive-field, so this module is a pass-through placeholder.
// ============================================================================

module conv_window_gen
    import npu_types_pkg::*;
(
    input  logic                clk,
    input  logic                rst_n,

    // Configuration (unused in stub)
    input  dim_t                filt_r,
    input  dim_t                filt_s,

    // Stream in
    input  logic [DATA_W-1:0]   in_data,
    input  logic                in_valid,
    output logic                in_ready,

    // Stream out
    output logic [DATA_W-1:0]   out_data,
    output logic                out_valid,
    input  logic                out_ready
);

    assign out_data  = in_data;
    assign out_valid = in_valid;
    assign in_ready  = out_ready;

endmodule : conv_window_gen
