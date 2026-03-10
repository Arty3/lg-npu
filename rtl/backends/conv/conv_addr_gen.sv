// ============================================================================
// conv_addr_gen.sv - Compute SRAM addresses from loop indices
// ============================================================================

module conv_addr_gen
    import npu_types_pkg::*;
(
    // Tensor base addresses
    input  logic [ADDR_W-1:0]  act_base,
    input  logic [ADDR_W-1:0]  wt_base,

    // Spatial parameters
    input  dim_t               in_w,
    input  dim_t               in_c,
    input  dim_t               filt_r,
    input  dim_t               filt_s,

    // Current loop indices
    input  dim_t               oh,
    input  dim_t               ow,
    input  dim_t               k,
    input  dim_t               r,
    input  dim_t               s,
    input  dim_t               c,
    input  dim_t               stride_h,
    input  dim_t               stride_w,
    input  dim_t               pad_h,
    input  dim_t               pad_w,

    // Outputs
    output logic [ADDR_W-1:0]  act_addr,
    output logic [ADDR_W-1:0]  wt_addr,
    output logic               zero_pad   // Activation is out-of-bounds
);

    dim_t ih, iw;

    // Compute input spatial coordinates
    assign ih = oh * stride_h + r - pad_h;
    assign iw = ow * stride_w + s - pad_w;

    // Out-of-bounds check (signed comparison via MSB for underflow)
    assign zero_pad = ih[DIM_W-1] | iw[DIM_W-1];

    // act_addr = act_base + ((ih * in_w) + iw) * in_c + c
    assign act_addr = act_base + ADDR_W'(((ih * in_w) + iw) * in_c + c);

    // wt_addr = wt_base + ((k * filt_r + r) * filt_s + s) * in_c + c
    assign wt_addr = wt_base + ADDR_W'(((k * filt_r + r) * filt_s + s) * in_c + c);

endmodule : conv_addr_gen
