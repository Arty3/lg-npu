// ============================================================================
// conv_addr_gen.sv - Compute SRAM addresses from loop indices
// ============================================================================

module conv_addr_gen
    import npu_types_pkg::*;
(
    input  logic                clk,
    input  logic                rst_n,

    // Handshake
    input  logic                in_valid,
    output logic                in_ready,
    output logic                out_valid,
    input  logic                out_ready,

    // Tensor base addresses
    input  logic [ADDR_W-1:0]  act_base,
    input  logic [ADDR_W-1:0]  wt_base,

    // Spatial parameters
    input  dim_t               in_h,
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

    typedef enum logic [2:0]
    {
        S_IDLE,
        S_STAGE1,
        S_STAGE2,
        S_STAGE3,
        S_OUT
    }   state_e;

    state_e state, state_next;

    logic signed [DIM_W:0] ih_calc_w, iw_calc_w;
    logic                  zero_pad_s1;
    logic [ADDR_W-1:0]     act_base_s1, wt_base_s1;
    dim_t                  in_c_s1;
    dim_t                  filt_s_s1;
    dim_t                  iw_u_s1;
    dim_t                  c_s1, r_s1, s_s1;
    logic [31:0]           act_m1_s1, wt_m1_s1;

    logic [31:0]           act_m2_s2, wt_m2_s2;
    logic                  zero_pad_s2;
    logic [ADDR_W-1:0]     act_base_s2, wt_base_s2;
    dim_t                  in_c_s2;
    dim_t                  c_s2, s_s2;

    logic [ADDR_W-1:0]     act_idx_s3, wt_idx_s3;
    logic                  zero_pad_s3;
    logic [ADDR_W-1:0]     act_base_s3, wt_base_s3;

    logic [ADDR_W-1:0]     act_addr_r, wt_addr_r;
    logic                  zero_pad_r;

    assign in_ready  = (state == S_IDLE);
    assign out_valid = (state == S_OUT);
    assign act_addr  = act_addr_r;
    assign wt_addr   = wt_addr_r;
    assign zero_pad  = zero_pad_r;

    assign ih_calc_w = $signed({1'b0, oh}) * $signed({1'b0, stride_h})
                     + $signed({1'b0, r})  - $signed({1'b0, pad_h});
    assign iw_calc_w = $signed({1'b0, ow}) * $signed({1'b0, stride_w})
                     + $signed({1'b0, s})  - $signed({1'b0, pad_w});

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            state <= S_IDLE;
        else
            state <= state_next;
    end

    always_comb begin
        state_next = state;
        case (state)
            S_IDLE:   if (in_valid)  state_next = S_STAGE1;
            S_STAGE1:               state_next = S_STAGE2;
            S_STAGE2:               state_next = S_STAGE3;
            S_STAGE3:               state_next = S_OUT;
            S_OUT:    if (out_ready) state_next = S_IDLE;
            default:                state_next = S_IDLE;
        endcase
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            zero_pad_s1  <= 1'b0;
            act_base_s1  <= '0;
            wt_base_s1   <= '0;
            in_c_s1      <= '0;
            filt_s_s1    <= '0;
            iw_u_s1      <= '0;
            c_s1         <= '0;
            r_s1         <= '0;
            s_s1         <= '0;
            act_m1_s1    <= '0;
            wt_m1_s1     <= '0;
            act_m2_s2    <= '0;
            wt_m2_s2     <= '0;
            zero_pad_s2  <= 1'b0;
            act_base_s2  <= '0;
            wt_base_s2   <= '0;
            in_c_s2      <= '0;
            c_s2         <= '0;
            s_s2         <= '0;
            act_idx_s3   <= '0;
            wt_idx_s3    <= '0;
            zero_pad_s3  <= 1'b0;
            act_base_s3  <= '0;
            wt_base_s3   <= '0;
            act_addr_r   <= '0;
            wt_addr_r    <= '0;
            zero_pad_r   <= 1'b0;
        end else begin
            if (state == S_IDLE && in_valid) begin
                zero_pad_s1 <= (ih_calc_w < 0)
                             || (ih_calc_w >= $signed({1'b0, in_h}))
                             || (iw_calc_w < 0)
                             || (iw_calc_w >= $signed({1'b0, in_w}));
                act_base_s1 <= act_base;
                wt_base_s1  <= wt_base;
                in_c_s1     <= in_c;
                filt_s_s1   <= filt_s;
                iw_u_s1     <= dim_t'(iw_calc_w);
                c_s1        <= c;
                r_s1        <= r;
                s_s1        <= s;
                act_m1_s1   <= {16'b0, dim_t'(ih_calc_w)} * {16'b0, in_w};
                wt_m1_s1    <= {16'b0, k} * {16'b0, filt_r};
            end

            if (state == S_STAGE1) begin
                act_m2_s2     <= (act_m1_s1 + {16'b0, iw_u_s1}) * {16'b0, in_c_s1};
                wt_m2_s2      <= (wt_m1_s1 + {16'b0, r_s1}) * {16'b0, filt_s_s1};
                zero_pad_s2   <= zero_pad_s1;
                act_base_s2   <= act_base_s1;
                wt_base_s2    <= wt_base_s1;
                in_c_s2       <= in_c_s1;
                c_s2          <= c_s1;
                s_s2          <= s_s1;
            end

            if (state == S_STAGE2) begin
                act_idx_s3   <= ADDR_W'(act_m2_s2 + {16'b0, c_s2});
                wt_idx_s3    <= ADDR_W'((wt_m2_s2 + {16'b0, s_s2}) * {16'b0, in_c_s2} + {16'b0, c_s2});
                zero_pad_s3  <= zero_pad_s2;
                act_base_s3  <= act_base_s2;
                wt_base_s3   <= wt_base_s2;
            end

            if (state == S_STAGE3) begin
                act_addr_r <= act_base_s3 + ADDR_W'(act_idx_s3);
                wt_addr_r  <= wt_base_s3  + ADDR_W'(wt_idx_s3);
                zero_pad_r <= zero_pad_s3;
            end
        end
    end

    /*
     * Silent-truncation guards. The ADDR_W'(...) casts in S_STAGE2 narrow
     * 32-bit intermediate index products down to ADDR_W bits. As long as
     * actual feature maps fit in ADDR_W (=16) addresses these never fire,
     * but they will catch overflow if the design ever scales up.
     */
`ifdef SIMULATION
    property p_act_idx_no_overflow;
        @(posedge clk) disable iff (!rst_n)
            (state == S_STAGE2) |->
            ((act_m2_s2 + {16'b0, c_s2}) < (32'h1 << ADDR_W));
    endproperty
    a_act_idx_no_overflow: assert property (p_act_idx_no_overflow)
        else $error("conv_addr_gen: act index overflows ADDR_W");

    property p_wt_idx_no_overflow;
        @(posedge clk) disable iff (!rst_n)
            (state == S_STAGE2) |->
            (((wt_m2_s2 + {16'b0, s_s2}) * {16'b0, in_c_s2}
                + {16'b0, c_s2}) < (32'h1 << ADDR_W));
    endproperty
    a_wt_idx_no_overflow: assert property (p_wt_idx_no_overflow)
        else $error("conv_addr_gen: wt index overflows ADDR_W");
`endif

    /*
     * NOTE (future scale-up): the FSM above keeps one transaction in flight
     * at a time (5 cycles/address). For a >1 PE array this becomes the
     * bottleneck. A true pipelined refactor (per-stage valid bits,
     * in_ready = ~out_v || out_ready) must be paired with a matching
     * metadata pipeline in conv_backend (last_inner_iter_r,
     * acc_clr_iter_r, wr_addr_iter_r) -- otherwise iter_accepts fire every
     * cycle while load_accepts trickle out, overwriting the single set of
     * metadata regs before the loader consumes them.
     */

endmodule : conv_addr_gen
