// ============================================================================
// lnorm_ctrl.sv - Multi-pass LayerNorm FSM
//   Pass 1 (SUM):       read N elements, accumulate signed sum.
//   Mean division:       serial restoring divider  sum / N.
//   Pass 2 (VAR):       read N elements, accumulate (x - mean)^2.
//   Variance division:  serial restoring divider  var_sum / N.
//   Integer sqrt:       serial sqrt of (variance + 1).
//   Pass 3 (NORMALIZE): for each element, read, compute
//       ((x - mean) << norm_shift) / std, clamp, write output.
//   Repeats for each of M independent rows.
// ============================================================================

module lnorm_ctrl
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
        S_SUM_RD,
        S_SUM_WAIT,
        S_MEAN_DIV,
        S_VAR_RD,
        S_VAR_WAIT,
        S_VAR_DIV,
        S_SQRT,
        S_NORM_RD,
        S_NORM_WAIT,
        S_NORM_DIV,
        S_NORM_WR,
        S_DONE
    }   state_e;

    state_e state, state_next;

    // Latched parameters
    dim_t              num_rows_r, row_len_r;
    logic [4:0]        norm_shift_r;

    // Row tracking
    dim_t              row_r;
    logic [ADDR_W-1:0] in_row_base_r, out_row_base_r;

    // Element index (reused across passes)
    dim_t idx_r;

    // Pass 1: signed sum
    logic signed [31:0] sum_r;

    // Mean result
    logic signed [31:0] mean_r;

    // Pass 2: unsigned variance accumulator
    logic [31:0] var_sum_r;

    // Sqrt result (standard deviation)
    logic [15:0] std_r;

    // Serial divider (shared by mean div, var div, norm div)
    /* verilator lint_off UNUSEDSIGNAL */
    logic [31:0] div_quot_r, div_rem_r, div_denom_r;
    /* verilator lint_on UNUSEDSIGNAL */
    logic [4:0]  div_cnt_r;
    logic        div_sign_r;   // sign of dividend for signed divisions

    // Serial sqrt
    /* verilator lint_off UNUSEDSIGNAL */
    logic [31:0] sqrt_input_r, sqrt_rem_r;
    /* verilator lint_on UNUSEDSIGNAL */
    logic [15:0] sqrt_result_r;
    logic [4:0]  sqrt_cnt_r;

    // Norm pass: sign of current element's diff
    logic norm_sign_r;

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

    // Combinational sqrt step
    logic [31:0] sqrt_rem_shifted_w, sqrt_trial_w;
    logic [31:0] sqrt_rem_next_w;
    logic [15:0] sqrt_result_next_w;

    assign sqrt_rem_shifted_w = {sqrt_rem_r[29:0], sqrt_input_r[31:30]};
    assign sqrt_trial_w       = {14'b0, sqrt_result_r, 2'b01};

    always_comb
    begin
        if (sqrt_rem_shifted_w >= sqrt_trial_w)
        begin
            sqrt_rem_next_w    = sqrt_rem_shifted_w - sqrt_trial_w;
            sqrt_result_next_w = {sqrt_result_r[14:0], 1'b1};
        end
        else
        begin
            sqrt_rem_next_w    = sqrt_rem_shifted_w;
            sqrt_result_next_w = {sqrt_result_r[14:0], 1'b0};
        end
    end

    // Norm pass: scaled absolute diff for division numerator
    /* verilator lint_off UNUSEDSIGNAL */
    logic signed [31:0] diff_w;
    /* verilator lint_on UNUSEDSIGNAL */
    logic [8:0]         abs_diff_w;
    logic [39:0]        wide_scaled_w;
    logic [31:0]        scaled_abs_w;

    assign diff_w        = {{24{act_rd_rdata[DATA_W-1]}}, act_rd_rdata} - mean_r;
    assign abs_diff_w    = diff_w[31] ? 9'(-diff_w[8:0]) : diff_w[8:0];
    assign wide_scaled_w = {31'b0, abs_diff_w} << norm_shift_r;
    assign scaled_abs_w  = (|wide_scaled_w[39:32]) ? 32'hFFFF_FFFF
                                                   : wide_scaled_w[31:0];

    // Norm pass: clamped output from divider result
    logic [DATA_W-1:0] norm_result;
    always_comb
    begin
        if (norm_sign_r)
        begin
            if (div_quot_r > 32'd128) norm_result = 8'h80;
            else                      norm_result = -div_quot_r[7:0];
        end
        else
        begin
            if (div_quot_r > 32'd127) norm_result = 8'd127;
            else                      norm_result = div_quot_r[7:0];
        end
    end

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
            S_IDLE:      if (cmd_valid)       state_next = S_LOAD_CFG;
            S_LOAD_CFG:                       state_next = S_SUM_RD;
            S_SUM_RD:    if (act_rd_gnt)      state_next = S_SUM_WAIT;
            S_SUM_WAIT: begin
                if (act_rd_rvalid)
                begin
                    if (idx_r == row_len_r - 1) state_next = S_MEAN_DIV;
                    else                        state_next = S_SUM_RD;
                end
            end
            S_MEAN_DIV:  if (div_cnt_r == 5'd0) state_next = S_VAR_RD;
            S_VAR_RD:    if (act_rd_gnt)         state_next = S_VAR_WAIT;
            S_VAR_WAIT: begin
                if (act_rd_rvalid)
                begin
                    if (idx_r == row_len_r - 1) state_next = S_VAR_DIV;
                    else                        state_next = S_VAR_RD;
                end
            end
            S_VAR_DIV:   if (div_cnt_r == 5'd0) state_next = S_SQRT;
            S_SQRT:      if (sqrt_cnt_r == 5'd0) state_next = S_NORM_RD;
            S_NORM_RD:   if (act_rd_gnt)         state_next = S_NORM_WAIT;
            S_NORM_WAIT: if (act_rd_rvalid)      state_next = S_NORM_DIV;
            S_NORM_DIV:  if (div_cnt_r == 5'd0)  state_next = S_NORM_WR;
            S_NORM_WR: begin
                if (out_wr_gnt)
                begin
                    if (idx_r == row_len_r - 1)
                    begin
                        if (row_r == num_rows_r - 1) state_next = S_DONE;
                        else                         state_next = S_SUM_RD;
                    end
                    else
                        state_next = S_NORM_RD;
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
            num_rows_r   <= '0;
            row_len_r    <= '0;
            norm_shift_r <= '0;
        end
        else if (state == S_IDLE && cmd_valid)
        begin
            num_rows_r   <= cmd.in_h;
            row_len_r    <= cmd.in_w;
            norm_shift_r <= cmd.quant_shift;
        end
    end

    // Row tracking (row counter + row base addresses)
    always_ff @(posedge clk or negedge rst_n)
    begin
        if (!rst_n)
        begin
            row_r          <= '0;
            in_row_base_r  <= '0;
            out_row_base_r <= '0;
        end
        else if (state == S_LOAD_CFG)
        begin
            row_r          <= '0;
            in_row_base_r  <= cmd.act_in_addr;
            out_row_base_r <= cmd.act_out_addr;
        end
        else if (state == S_NORM_WR && out_wr_gnt &&
                 idx_r == row_len_r - 1 && row_r < num_rows_r - 1)
        begin
            row_r          <= row_r + 1;
            in_row_base_r  <= in_row_base_r  + ADDR_W'(row_len_r);
            out_row_base_r <= out_row_base_r + ADDR_W'(row_len_r);
        end
    end

    // Element index
    always_ff @(posedge clk or negedge rst_n)
    begin
        if (!rst_n)
            idx_r <= '0;
        else
        begin
            case (state)
                // Reset at start of each pass
                S_LOAD_CFG: idx_r <= '0;
                // Sum pass: advance on rvalid
                S_SUM_WAIT: begin
                    if (act_rd_rvalid)
                    begin
                        if (idx_r == row_len_r - 1) idx_r <= '0;
                        else                        idx_r <= idx_r + 1;
                    end
                end
                // Var pass: advance on rvalid
                S_VAR_WAIT: begin
                    if (act_rd_rvalid)
                    begin
                        if (idx_r == row_len_r - 1) idx_r <= '0;
                        else                        idx_r <= idx_r + 1;
                    end
                end
                // Norm pass: advance on write grant
                S_NORM_WR: begin
                    if (out_wr_gnt)
                    begin
                        if (idx_r == row_len_r - 1) idx_r <= '0;
                        else                        idx_r <= idx_r + 1;
                    end
                end
                default: ;
            endcase
        end
    end

    // Sum accumulator (pass 1)
    always_ff @(posedge clk or negedge rst_n)
    begin
        if (!rst_n)
            sum_r <= '0;
        else if (state == S_LOAD_CFG ||
                 (state == S_NORM_WR && out_wr_gnt &&
                  idx_r == row_len_r - 1 && row_r < num_rows_r - 1))
            sum_r <= '0;
        else if (state == S_SUM_WAIT && act_rd_rvalid)
            sum_r <= sum_r + {{24{act_rd_rdata[DATA_W-1]}}, act_rd_rdata};
    end

    // Variance accumulator (pass 2)
    always_ff @(posedge clk or negedge rst_n)
    begin
        if (!rst_n)
            var_sum_r <= '0;
        else if (state == S_LOAD_CFG ||
                 (state == S_NORM_WR && out_wr_gnt &&
                  idx_r == row_len_r - 1 && row_r < num_rows_r - 1))
            var_sum_r <= '0;
        else if (state == S_VAR_WAIT && act_rd_rvalid)
        begin
            // |diff| fits in 9 bits, so diff^2 fits in 17 bits unsigned
            var_sum_r <= var_sum_r + {23'b0, abs_diff_w} * {23'b0, abs_diff_w};
        end
    end

    // Divider register updates
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
            case (state)
                // Init mean division: abs(sum) / N
                S_SUM_WAIT: begin
                    if (act_rd_rvalid && idx_r == row_len_r - 1)
                    begin
                        // Capture final sum (including this element)
                        logic signed [31:0] final_sum;
                        final_sum = sum_r + {{24{act_rd_rdata[DATA_W-1]}}, act_rd_rdata};
                        div_sign_r  <= final_sum[31];
                        div_quot_r  <= final_sum[31] ? 32'(-final_sum) : 32'(final_sum);
                        div_rem_r   <= '0;
                        div_denom_r <= {16'b0, row_len_r};
                        div_cnt_r   <= 5'd31;
                    end
                end
                // Run mean division
                S_MEAN_DIV: begin
                    div_quot_r <= div_quot_next_w;
                    div_rem_r  <= div_rem_next_w;
                    div_cnt_r  <= div_cnt_r - 5'd1;
                end
                // Init variance division: var_sum / N
                S_VAR_WAIT: begin
                    if (act_rd_rvalid && idx_r == row_len_r - 1)
                    begin
                        logic [31:0] final_var;
                        final_var = var_sum_r + {23'b0, abs_diff_w} * {23'b0, abs_diff_w};
                        div_sign_r  <= 1'b0;
                        div_quot_r  <= final_var;
                        div_rem_r   <= '0;
                        div_denom_r <= {16'b0, row_len_r};
                        div_cnt_r   <= 5'd31;
                    end
                end
                // Run variance division
                S_VAR_DIV: begin
                    div_quot_r <= div_quot_next_w;
                    div_rem_r  <= div_rem_next_w;
                    div_cnt_r  <= div_cnt_r - 5'd1;
                end
                // Init norm division when rvalid fires in NORM_WAIT
                S_NORM_WAIT: begin
                    if (act_rd_rvalid)
                    begin
                        norm_sign_r <= diff_w[31];
                        div_sign_r  <= 1'b0;
                        div_quot_r  <= scaled_abs_w;
                        div_rem_r   <= '0;
                        div_denom_r <= {16'b0, std_r};
                        div_cnt_r   <= 5'd31;
                    end
                end
                // Run norm division
                S_NORM_DIV: begin
                    div_quot_r <= div_quot_next_w;
                    div_rem_r  <= div_rem_next_w;
                    div_cnt_r  <= div_cnt_r - 5'd1;
                end
                default: ;
            endcase
        end
    end

    // Mean capture (from combinational divider output on last div cycle)
    always_ff @(posedge clk or negedge rst_n)
    begin
        if (!rst_n)
            mean_r <= '0;
        else if (state == S_MEAN_DIV && div_cnt_r == 5'd0)
        begin
            if (div_sign_r)
                mean_r <= -div_quot_next_w;
            else
                mean_r <= div_quot_next_w;
        end
    end

    // Sqrt register updates
    always_ff @(posedge clk or negedge rst_n)
    begin
        if (!rst_n)
        begin
            sqrt_input_r  <= '0;
            sqrt_rem_r    <= '0;
            sqrt_result_r <= '0;
            sqrt_cnt_r    <= '0;
        end
        else
        begin
            // Init sqrt when variance division completes
            if (state == S_VAR_DIV && div_cnt_r == 5'd0)
            begin
                sqrt_input_r  <= div_quot_next_w + 32'd1;
                sqrt_rem_r    <= '0;
                sqrt_result_r <= '0;
                sqrt_cnt_r    <= 5'd15;
            end
            else if (state == S_SQRT)
            begin
                sqrt_rem_r    <= sqrt_rem_next_w;
                sqrt_result_r <= sqrt_result_next_w;
                sqrt_input_r  <= {sqrt_input_r[29:0], 2'b00};
                sqrt_cnt_r    <= sqrt_cnt_r - 5'd1;
            end
        end
    end

    // Std capture (from combinational sqrt output on last sqrt cycle)
    always_ff @(posedge clk or negedge rst_n)
    begin
        if (!rst_n)
            std_r <= 16'd1;
        else if (state == S_SQRT && sqrt_cnt_r == 5'd0)
            std_r <= (sqrt_result_next_w == 16'd0) ? 16'd1 : sqrt_result_next_w;
    end

    // Memory interface: act read
    always_comb
    begin
        act_rd_addr = '0;
        act_rd_req  = 1'b0;
        case (state)
            S_SUM_RD: begin
                act_rd_addr = in_row_base_r + ADDR_W'(idx_r);
                act_rd_req  = 1'b1;
            end
            S_VAR_RD: begin
                act_rd_addr = in_row_base_r + ADDR_W'(idx_r);
                act_rd_req  = 1'b1;
            end
            S_NORM_RD: begin
                act_rd_addr = in_row_base_r + ADDR_W'(idx_r);
                act_rd_req  = 1'b1;
            end
            default: ;
        endcase
    end

    // Memory interface: output write
    assign out_wr_addr = out_row_base_r + ADDR_W'(idx_r);
    assign out_wr_data = norm_result;
    assign out_wr_req  = (state == S_NORM_WR);

    // Status
    assign cmd_ready = (state == S_IDLE);
    assign done      = (state == S_DONE);
    assign busy      = (state != S_IDLE);

endmodule : lnorm_ctrl
