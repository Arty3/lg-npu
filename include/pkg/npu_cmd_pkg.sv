// ============================================================================
// npu_cmd_pkg.sv - Command descriptor definitions
// ============================================================================

package npu_cmd_pkg;

    import npu_types_pkg::*;

    // Opcode enum
    typedef enum logic [3:0]
	{
        OP_CONV    = 4'h1,
        OP_GEMM    = 4'h2,
        OP_SOFTMAX = 4'h3,
        OP_VEC     = 4'h4,
        OP_LNORM   = 4'h5,
        OP_POOL    = 4'h6
    }   opcode_e;

    // Shared command descriptor layout (carried as conv_cmd_t).
    // Field usage by opcode:
    //   OP_CONV:    uses all convolution fields including out_h/out_w.
    //   OP_POOL:    uses act_in/out_addr, in_h/in_w/in_c, filt_r/filt_s,
    //               stride_h/stride_w, pad_h/pad_w, out_h/out_w,
    //               quant_shift[0] as pool_mode.
    //   OP_GEMM:    aliases act_in_addr/a, act_out_addr/c, weight_addr/b,
    //               bias_addr, in_h/M, in_w/N, in_c/K, quant_shift, act_mode.
    //   OP_SOFTMAX: aliases act_in_addr/in, act_out_addr/out, in_h/num_rows,
    //               in_w/row_len.
    //   OP_VEC:     aliases act_in_addr/a, act_out_addr/out, weight_addr/b,
    //               in_h/length, in_w/vec_op, quant_shift, act_mode.
    //   OP_LNORM:   aliases act_in_addr/in, act_out_addr/out, in_h/num_rows,
    //               in_w/row_len, quant_shift as norm_shift.
    // Submitted by host software through the command queue.
    typedef struct packed
	{
        opcode_e               opcode;

        logic [ADDR_W-1:0]     act_in_addr;
        logic [ADDR_W-1:0]     act_out_addr;
        logic [ADDR_W-1:0]     weight_addr;
        logic [ADDR_W-1:0]     bias_addr;

        // Spatial dimensions
        dim_t                  in_h;
        dim_t                  in_w;
        dim_t                  in_c;
        dim_t                  out_k;
        dim_t                  filt_r;
        dim_t                  filt_s;

        // Stride / padding
        dim_t                  stride_h;
        dim_t                  stride_w;
        dim_t                  pad_h;
        dim_t                  pad_w;

        // Host-precomputed output shape (avoids runtime divider inference)
        dim_t                  out_h;
        dim_t                  out_w;

        // Quantization shift (right arithmetic shift for INT32->INT8)
        logic [4:0]            quant_shift;

        // Activation function selector
        act_mode_e             act_mode;
    }   conv_cmd_t;

    // Backend-specific command views (aliases over shared descriptor fields).
    typedef struct packed
    {
        logic [ADDR_W-1:0] a_addr;
        logic [ADDR_W-1:0] c_addr;
        logic [ADDR_W-1:0] b_addr;
        logic [ADDR_W-1:0] bias_addr;
        dim_t              m_dim;
        dim_t              n_dim;
        dim_t              k_dim;
        logic [4:0]        quant_shift;
        act_mode_e         act_mode;
    }   gemm_cmd_t;

    typedef struct packed
    {
        logic [ADDR_W-1:0] in_addr;
        logic [ADDR_W-1:0] out_addr;
        dim_t              num_rows;
        dim_t              row_len;
    }   softmax_cmd_t;

    typedef struct packed
    {
        logic [ADDR_W-1:0] a_addr;
        logic [ADDR_W-1:0] out_addr;
        logic [ADDR_W-1:0] b_addr;
        dim_t              length;
        logic              vec_op;
        logic [4:0]        quant_shift;
        act_mode_e         act_mode;
    }   vec_cmd_t;

    typedef struct packed
    {
        logic [ADDR_W-1:0] in_addr;
        logic [ADDR_W-1:0] out_addr;
        dim_t              num_rows;
        dim_t              row_len;
        logic [4:0]        norm_shift;
    }   lnorm_cmd_t;

    typedef struct packed
    {
        logic [ADDR_W-1:0] in_addr;
        logic [ADDR_W-1:0] out_addr;
        dim_t              in_h;
        dim_t              in_w;
        dim_t              in_c;
        dim_t              pool_r;
        dim_t              pool_s;
        dim_t              stride_h;
        dim_t              stride_w;
        dim_t              pad_h;
        dim_t              pad_w;
        dim_t              out_h;
        dim_t              out_w;
        logic              pool_mode;
    }   pool_cmd_t;

    /* verilator lint_off UNUSEDSIGNAL */
    function automatic gemm_cmd_t conv_to_gemm_cmd(input conv_cmd_t cmd);
        gemm_cmd_t out;

        out.a_addr      = cmd.act_in_addr;
        out.c_addr      = cmd.act_out_addr;
        out.b_addr      = cmd.weight_addr;
        out.bias_addr   = cmd.bias_addr;
        out.m_dim       = cmd.in_h;
        out.n_dim       = cmd.in_w;
        out.k_dim       = cmd.in_c;
        out.quant_shift = cmd.quant_shift;
        out.act_mode    = cmd.act_mode;

        return out;
    endfunction

    function automatic softmax_cmd_t conv_to_softmax_cmd(input conv_cmd_t cmd);
        softmax_cmd_t out;

        out.in_addr   = cmd.act_in_addr;
        out.out_addr  = cmd.act_out_addr;
        out.num_rows  = cmd.in_h;
        out.row_len   = cmd.in_w;

        return out;
    endfunction

    function automatic vec_cmd_t conv_to_vec_cmd(input conv_cmd_t cmd);
        vec_cmd_t out;

        out.a_addr      = cmd.act_in_addr;
        out.out_addr    = cmd.act_out_addr;
        out.b_addr      = cmd.weight_addr;
        out.length      = cmd.in_h;
        out.vec_op      = cmd.in_w[0];
        out.quant_shift = cmd.quant_shift;
        out.act_mode    = cmd.act_mode;

        return out;
    endfunction

    function automatic lnorm_cmd_t conv_to_lnorm_cmd(input conv_cmd_t cmd);
        lnorm_cmd_t out;

        out.in_addr    = cmd.act_in_addr;
        out.out_addr   = cmd.act_out_addr;
        out.num_rows   = cmd.in_h;
        out.row_len    = cmd.in_w;
        out.norm_shift = cmd.quant_shift;

        return out;
    endfunction

    function automatic pool_cmd_t conv_to_pool_cmd(input conv_cmd_t cmd);
        pool_cmd_t out;

        out.in_addr   = cmd.act_in_addr;
        out.out_addr  = cmd.act_out_addr;
        out.in_h      = cmd.in_h;
        out.in_w      = cmd.in_w;
        out.in_c      = cmd.in_c;
        out.pool_r    = cmd.filt_r;
        out.pool_s    = cmd.filt_s;
        out.stride_h  = cmd.stride_h;
        out.stride_w  = cmd.stride_w;
        out.pad_h     = cmd.pad_h;
        out.pad_w     = cmd.pad_w;
        out.out_h     = cmd.out_h;
        out.out_w     = cmd.out_w;
        out.pool_mode = cmd.quant_shift[0];

        return out;
    endfunction
    /* verilator lint_on UNUSEDSIGNAL */

    // Generic command wrapper (reserved for future multi-backend dispatch)
    localparam int CMD_PAYLOAD_W = $bits(conv_cmd_t);

    typedef struct packed
	{
        opcode_e                   opcode;
        logic [CMD_PAYLOAD_W-1:0]  payload;
    }   cmd_t;

endpackage : npu_cmd_pkg
