// ============================================================================
// conv_line_buffer.sv - Line buffer for activation reuse (stub)
//   In the initial scope the loader re-fetches every activation from SRAM,
//   so this module is a simple pass-through placeholder.
// ============================================================================
module conv_line_buffer
    import npu_types_pkg::*;
(
    input  logic                clk,
    input  logic                rst_n,

    // Configuration (unused in stub)
    input  dim_t                in_w,
    input  dim_t                in_c,

    // Stream in
    input  logic [DATA_W-1:0]   in_data,
    input  logic                in_valid,
    output logic                in_ready,

    // Stream out
    output logic [DATA_W-1:0]   out_data,
    output logic                out_valid,
    input  logic                out_ready
);

    // Pass-through
    assign out_data  = in_data;
    assign out_valid = in_valid;
    assign in_ready  = out_ready;

endmodule : conv_line_buffer
