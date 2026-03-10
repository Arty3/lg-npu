// ============================================================================
// pool_ctrl.sv - 2D spatial pooling FSM (MAX and AVG)
//   Walks a 5-deep loop nest (oh, ow, c, r, s) over the input activation
//   buffer.  For MAX mode the result is the maximum element in the window;
//   for AVG mode the INT32 sum is divided by (pool_r * pool_s) using a
//   serial restoring divider (32 cycles) and clamped to INT8.
//   Out-of-bounds positions due to padding are skipped for MAX and treated
//   as zero for AVG.
// ============================================================================

module pool_ctrl
    import npu_types_pkg::*;
    import npu_cmd_pkg::*;
(
    input  logic                clk,
    input  logic                rst_n,

    // Command interface (from dispatch)
    /* verilator lint_off UNUSEDSIGNAL */
    input  conv_cmd_t           cmd,
    /* verilator lint_on UNUSEDSIGNAL */
    input  logic                cmd_valid,
    output logic                cmd_ready,

    // Memory - activation buffer read
    output logic [ADDR_W-1:0]   act_rd_addr,
    output logic                act_rd_req,
    input  logic                act_rd_gnt,
    input  logic [DATA_W-1:0]   act_rd_rdata,
    input  logic                act_rd_rvalid,

    // Memory - activation buffer write (output)
    output logic [ADDR_W-1:0]   out_wr_addr,
    output logic [DATA_W-1:0]   out_wr_data,
    output logic                out_wr_req,
    input  logic                out_wr_gnt,

    // Status
    output logic                done,
    output logic                busy
);

    typedef enum logic [3:0]
    {
        S_IDLE,
        S_LOAD_CFG,
        S_RD,
        S_WAIT,
        S_NEXT,
        S_AVG_DIV,
        S_WR,
        S_DONE
    }   state_e;

    state_e state, state_next;

    // Latched parameters
    logic [ADDR_W-1:0] in_base_r, out_base_r;
    dim_t              in_h_r, in_w_r, in_c_r;
    dim_t              pool_r_r, pool_s_r;
    dim_t              stride_h_r, stride_w_r;
    dim_t              pad_h_r, pad_w_r;
    logic              pool_mode_r;    // 0=MAX, 1=AVG

    // Derived output dimensions
    dim_t out_h_r, out_w_r;

    // Outer loop indices: oh, ow, c
    dim_t oh_r, ow_r, c_r;

    // Inner loop indices: r, s (within pooling window)
    dim_t r_r, s_r;

    // Accumulator / MAX tracker
    logic signed [31:0] acc_r;       // shared for AVG sum and MAX
    logic               has_valid_r; // MAX: did we see at least one valid element?

    // Pool window size for AVG divider
    logic [15:0] pool_area_r;        // pool_r * pool_s

    // Serial restoring divider (for AVG)
    /* verilator lint_off UNUSEDSIGNAL */
    logic [31:0] div_quot_r, div_rem_r;
    /* verilator lint_on UNUSEDSIGNAL */
    logic [31:0] div_denom_r;
    logic [4:0]  div_cnt_r;
    logic        div_sign_r;

    // Combinational divider step
    logic [31:0] div_trial_w, div_rem_next_w, div_quot_next_w;

    assign div_trial_w = {div_rem_r[30:0], div_quot_r[31]};

    always_comb
    begin
        if (div_trial_w >= div_denom_r)
        begin
            div_rem_next_w  = div_trial_w - div_denom_r;
            div_quot_next_w = {div_quot_r[30:0], 1'b1};
        end
        else
        begin
            div_rem_next_w  = div_trial_w;
            div_quot_next_w = {div_quot_r[30:0], 1'b0};
        end
    end

    // Input coordinate computation (combinational)
    // ih = oh * stride_h + r - pad_h,  iw = ow * stride_w + s - pad_w
    logic signed [16:0] ih_w, iw_w;
    logic               oob_w;   // out of bounds

    assign ih_w = $signed({1'b0, oh_r}) * $signed({1'b0, stride_h_r})
                + $signed({1'b0, r_r})  - $signed({1'b0, pad_h_r});
    assign iw_w = $signed({1'b0, ow_r}) * $signed({1'b0, stride_w_r})
                + $signed({1'b0, s_r})  - $signed({1'b0, pad_w_r});
    assign oob_w = (ih_w < 0) || (ih_w >= $signed({1'b0, in_h_r}))
                || (iw_w < 0) || (iw_w >= $signed({1'b0, in_w_r}));

    // Read address: base + (ih * in_w + iw) * in_c + c
    logic [ADDR_W-1:0] rd_addr_w;
    assign rd_addr_w = in_base_r
                     + ADDR_W'(ih_w[15:0]) * ADDR_W'(in_w_r) * ADDR_W'(in_c_r)
                     + ADDR_W'(iw_w[15:0]) * ADDR_W'(in_c_r)
                     + ADDR_W'(c_r);

    // Output address: out_base + (oh * out_w + ow) * in_c + c
    logic [ADDR_W-1:0] wr_addr_w;
    assign wr_addr_w = out_base_r
                     + ADDR_W'(oh_r) * ADDR_W'(out_w_r) * ADDR_W'(in_c_r)
                     + ADDR_W'(ow_r) * ADDR_W'(in_c_r)
                     + ADDR_W'(c_r);

    // Write data: result from MAX or serial divider for AVG
    logic signed [31:0] signed_quot_w;
    always_comb
    begin
        if (div_sign_r)
            signed_quot_w = -$signed({1'b0, div_quot_r[30:0]});
        else
            signed_quot_w = $signed({1'b0, div_quot_r[30:0]});
    end

    logic [DATA_W-1:0] result_w;
    always_comb
    begin
        if (!pool_mode_r)
        begin
            // MAX: saturate acc_r to INT8 (should already be in range)
            if (acc_r > 32'sd127)       result_w = 8'sd127;
            else if (acc_r < -32'sd128) result_w = 8'h80;
            else                        result_w = acc_r[7:0];
        end
        else
        begin
            // AVG: result from divider, restore sign and clamp
            if (signed_quot_w > 32'sd127)        result_w = 8'sd127;
            else if (signed_quot_w < -32'sd128)  result_w = 8'h80;
            else                                 result_w = signed_quot_w[7:0];
        end
    end

    // Last element in pooling window?
    logic last_inner_w;
    assign last_inner_w = (r_r == pool_r_r - 1) && (s_r == pool_s_r - 1);

    // Last output element?
    logic last_output_w;
    assign last_output_w = (oh_r == out_h_r - 1)
                        && (ow_r == out_w_r - 1)
                        && (c_r  == in_c_r  - 1);

    // State register
    always_ff @(posedge clk or negedge rst_n)
    begin
        if (!rst_n)
            state <= S_IDLE;
        else
            state <= state_next;
    end

    // Next-state logic
    always_comb
    begin
        state_next = state;
        case (state)
            S_IDLE:     if (cmd_valid)           state_next = S_LOAD_CFG;
            S_LOAD_CFG:                          state_next = S_RD;
            S_RD: begin
                if (oob_w)
                    state_next = S_NEXT;         // skip OOB, go straight to NEXT
                else if (act_rd_gnt)
                    state_next = S_WAIT;
            end
            S_WAIT:     if (act_rd_rvalid)       state_next = S_NEXT;
            S_NEXT: begin
                if (last_inner_w)
                begin
                    if (pool_mode_r)
                        state_next = S_AVG_DIV;
                    else
                        state_next = S_WR;
                end
                else
                    state_next = S_RD;
            end
            S_AVG_DIV:  if (div_cnt_r == 5'd0)  state_next = S_WR;
            S_WR: begin
                if (out_wr_gnt)
                begin
                    if (last_output_w)
                        state_next = S_DONE;
                    else
                        state_next = S_RD;
                end
            end
            S_DONE:                              state_next = S_IDLE;
            default:                             state_next = S_IDLE;
        endcase
    end

    // Command latch
    always_ff @(posedge clk or negedge rst_n)
    begin
        if (!rst_n)
        begin
            in_base_r   <= '0;
            out_base_r  <= '0;
            in_h_r      <= '0;
            in_w_r      <= '0;
            in_c_r      <= '0;
            pool_r_r    <= '0;
            pool_s_r    <= '0;
            stride_h_r  <= '0;
            stride_w_r  <= '0;
            pad_h_r     <= '0;
            pad_w_r     <= '0;
            pool_mode_r <= 1'b0;
            out_h_r     <= '0;
            out_w_r     <= '0;
            pool_area_r <= '0;
        end
        else if (state == S_IDLE && cmd_valid)
        begin
            in_base_r   <= cmd.act_in_addr;
            out_base_r  <= cmd.act_out_addr;
            in_h_r      <= cmd.in_h;
            in_w_r      <= cmd.in_w;
            in_c_r      <= cmd.in_c;
            pool_r_r    <= cmd.filt_r;
            pool_s_r    <= cmd.filt_s;
            stride_h_r  <= cmd.stride_h;
            stride_w_r  <= cmd.stride_w;
            pad_h_r     <= cmd.pad_h;
            pad_w_r     <= cmd.pad_w;
            pool_mode_r <= cmd.quant_shift[0];
            // Compute output dimensions
            out_h_r     <= (cmd.in_h + 2 * cmd.pad_h - cmd.filt_r) / cmd.stride_h + 1;
            out_w_r     <= (cmd.in_w + 2 * cmd.pad_w - cmd.filt_s) / cmd.stride_w + 1;
            pool_area_r <= cmd.filt_r[7:0] * cmd.filt_s[7:0];
        end
    end

    // Outer loop indices (oh, ow, c): advance on write grant
    always_ff @(posedge clk or negedge rst_n)
    begin
        if (!rst_n)
        begin
            oh_r <= '0;
            ow_r <= '0;
            c_r  <= '0;
        end
        else if (state == S_LOAD_CFG)
        begin
            oh_r <= '0;
            ow_r <= '0;
            c_r  <= '0;
        end
        else if (state == S_WR && out_wr_gnt && !last_output_w)
        begin
            if (c_r < in_c_r - 1)
                c_r <= c_r + 1;
            else
            begin
                c_r <= '0;
                if (ow_r < out_w_r - 1)
                    ow_r <= ow_r + 1;
                else
                begin
                    ow_r <= '0;
                    oh_r <= oh_r + 1;
                end
            end
        end
    end

    // Inner loop indices (r, s): advance in S_NEXT
    always_ff @(posedge clk or negedge rst_n)
    begin
        if (!rst_n)
        begin
            r_r <= '0;
            s_r <= '0;
        end
        else if (state == S_LOAD_CFG ||
                 (state == S_WR && out_wr_gnt))
        begin
            r_r <= '0;
            s_r <= '0;
        end
        else if (state_next == S_NEXT && state != S_NEXT)
        begin
            // Will be consumed in S_NEXT; advance for next iteration
        end
        else if (state == S_NEXT && !last_inner_w)
        begin
            if (s_r < pool_s_r - 1)
                s_r <= s_r + 1;
            else
            begin
                s_r <= '0;
                r_r <= r_r + 1;
            end
        end
    end

    // Accumulator / MAX update
    always_ff @(posedge clk or negedge rst_n)
    begin
        if (!rst_n)
        begin
            acc_r       <= '0;
            has_valid_r <= 1'b0;
        end
        else if (state == S_LOAD_CFG ||
                 (state == S_WR && out_wr_gnt))
        begin
            // Reset for next output element
            if (!pool_mode_r)
            begin
                acc_r       <= -32'sd128;  // MAX init
                has_valid_r <= 1'b0;
            end
            else
            begin
                acc_r       <= '0;         // AVG sum init
                has_valid_r <= 1'b0;
            end
        end
        else if (state == S_WAIT && act_rd_rvalid)
        begin
            // Valid in-bounds element arrived
            has_valid_r <= 1'b1;
            if (!pool_mode_r)
            begin
                // MAX: update if larger (signed compare)
                logic signed [31:0] elem;
                elem = {{24{act_rd_rdata[DATA_W-1]}}, act_rd_rdata};
                if (!has_valid_r || elem > acc_r)
                    acc_r <= elem;
            end
            else
            begin
                // AVG: accumulate
                acc_r <= acc_r + {{24{act_rd_rdata[DATA_W-1]}}, act_rd_rdata};
            end
        end
    end

    // AVG serial divider register updates
    always_ff @(posedge clk or negedge rst_n)
    begin
        if (!rst_n)
        begin
            div_quot_r  <= '0;
            div_rem_r   <= '0;
            div_denom_r <= '0;
            div_cnt_r   <= '0;
            div_sign_r  <= 1'b0;
        end
        else
        begin
            // Init divider when entering S_AVG_DIV from S_NEXT
            if (state == S_NEXT && last_inner_w && pool_mode_r)
            begin
                div_sign_r  <= acc_r[31];
                div_quot_r  <= acc_r[31] ? 32'(-acc_r) : 32'(acc_r);
                div_rem_r   <= '0;
                div_denom_r <= {16'b0, pool_area_r};
                div_cnt_r   <= 5'd31;
            end
            else if (state == S_AVG_DIV)
            begin
                div_quot_r <= div_quot_next_w;
                div_rem_r  <= div_rem_next_w;
                div_cnt_r  <= div_cnt_r - 5'd1;
            end
        end
    end

    // Memory interface: activation read
    always_comb
    begin
        act_rd_addr = '0;
        act_rd_req  = 1'b0;
        if (state == S_RD && !oob_w)
        begin
            act_rd_addr = rd_addr_w;
            act_rd_req  = 1'b1;
        end
    end

    // Memory interface: output write
    assign out_wr_addr = wr_addr_w;
    assign out_wr_data = result_w;
    assign out_wr_req  = (state == S_WR);

    // Status
    assign cmd_ready = (state == S_IDLE);
    assign done      = (state == S_DONE);
    assign busy      = (state != S_IDLE);

endmodule : pool_ctrl
