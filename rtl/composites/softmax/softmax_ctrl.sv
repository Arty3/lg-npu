// ============================================================================
// softmax_ctrl.sv - Multi-pass softmax FSM
//   Pass 1 (FIND_MAX): read N elements, find maximum value.
//   Pass 2 (EXP_SUM):  read N elements, compute exp(max-x) via LUT, sum.
//   Pass 3 (NORMALIZE): for each element, read, compute exp, divide by sum,
//                        scale to [0,127] INT8, write output.
//   Repeats for each of M independent rows.
// ============================================================================

module softmax_ctrl
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

    // Exp LUT interface
    output logic [7:0]          lut_diff,
    input  logic [15:0]         lut_exp_val,

    // Status
    output logic                done,
    output logic                busy
);

    typedef enum logic [3:0]
    {
        S_IDLE,
        S_LOAD_CFG,
        S_FIND_MAX,
        S_EXP_SUM,
        S_NORM_RD,
        S_NORM_WAIT,
        S_NORM_DIV,
        S_NORM_WR,
        S_DONE
    }   state_e;

    state_e state, state_next;

    // Latched command fields
    logic [ADDR_W-1:0] in_base_r, out_base_r;
    dim_t              num_rows_r, row_len_r;

    // Row counter
    dim_t row_r;

    // Element counters
    dim_t req_cnt_r;   // requests issued
    dim_t rcv_cnt_r;   // responses received

    // Per-row state
    logic signed [DATA_W-1:0] max_val_r;
    logic [31:0]              sum_r;

    // Normalize per-element state
    dim_t                     norm_idx_r;
    logic [23:0]              div_numer_r;
    /* verilator lint_off UNUSEDSIGNAL */
    logic [31:0]              div_rem_r;
    /* verilator lint_on UNUSEDSIGNAL */
    logic [7:0]               div_quot_r;
    logic [4:0]               div_cnt_r;

    // Address helpers
    logic [ADDR_W-1:0] row_base_in;
    logic [ADDR_W-1:0] row_base_out;
    assign row_base_in  = in_base_r  + ADDR_W'(row_r * row_len_r);
    assign row_base_out = out_base_r + ADDR_W'(row_r * row_len_r);

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
            S_IDLE:      if (cmd_valid)                            state_next = S_LOAD_CFG;
            S_LOAD_CFG:                                            state_next = S_FIND_MAX;
            S_FIND_MAX:  if (act_rd_rvalid && rcv_cnt_r == row_len_r - 1) state_next = S_EXP_SUM;
            S_EXP_SUM:   if (rcv_cnt_r == row_len_r)              state_next = S_NORM_RD;
            S_NORM_RD:   if (act_rd_gnt)                          state_next = S_NORM_WAIT;
            S_NORM_WAIT: if (act_rd_rvalid)                       state_next = S_NORM_DIV;
            S_NORM_DIV:  if (div_cnt_r == 5'd23)                  state_next = S_NORM_WR;
            S_NORM_WR:   begin
                if (out_wr_gnt) begin
                    if (norm_idx_r == row_len_r - 1) begin
                        if (row_r == num_rows_r - 1)              state_next = S_DONE;
                        else                                      state_next = S_FIND_MAX;
                    end else begin
                                                                  state_next = S_NORM_RD;
                    end
                end
            end
            S_DONE:                                                state_next = S_IDLE;
            default:                                               state_next = S_IDLE;
        endcase
    end

    // Command latch
    always_ff @(posedge clk or negedge rst_n)
    begin
        if (!rst_n) begin
            in_base_r  <= '0;
            out_base_r <= '0;
            num_rows_r <= '0;
            row_len_r  <= '0;
        end else if (state == S_IDLE && cmd_valid) begin
            in_base_r  <= cmd.act_in_addr;
            out_base_r <= cmd.act_out_addr;
            num_rows_r <= cmd.in_h;
            row_len_r  <= cmd.in_w;
        end
    end

    // Row counter
    always_ff @(posedge clk or negedge rst_n)
    begin
        if (!rst_n)
            row_r <= '0;
        else if (state == S_LOAD_CFG)
            row_r <= '0;
        else if (state == S_NORM_WR && out_wr_gnt &&
                 norm_idx_r == row_len_r - 1 && row_r < num_rows_r - 1)
            row_r <= row_r + 1;
    end

    // Request / receive counters (shared between FIND_MAX and EXP_SUM)
    always_ff @(posedge clk or negedge rst_n)
    begin
        if (!rst_n) begin
            req_cnt_r <= '0;
            rcv_cnt_r <= '0;
        end else begin
            case (state)
                S_LOAD_CFG: begin
                    req_cnt_r <= '0;
                    rcv_cnt_r <= '0;
                end
                S_FIND_MAX: begin
                    if (act_rd_gnt && req_cnt_r < row_len_r)
                        req_cnt_r <= req_cnt_r + 1;
                    if (act_rd_rvalid)
                        rcv_cnt_r <= rcv_cnt_r + 1;
                    // Transition to EXP_SUM resets counters
                    if (rcv_cnt_r == row_len_r - 1 && act_rd_rvalid) begin
                        req_cnt_r <= '0;
                        rcv_cnt_r <= '0;
                    end
                end
                S_EXP_SUM: begin
                    if (act_rd_gnt && req_cnt_r < row_len_r)
                        req_cnt_r <= req_cnt_r + 1;
                    if (act_rd_rvalid)
                        rcv_cnt_r <= rcv_cnt_r + 1;
                end
                // Reset for new row
                S_NORM_WR: begin
                    if (out_wr_gnt && norm_idx_r == row_len_r - 1
                        && row_r < num_rows_r - 1) begin
                        req_cnt_r <= '0;
                        rcv_cnt_r <= '0;
                    end
                end
                default: ;
            endcase
        end
    end

    // Max value register (FIND_MAX pass)
    always_ff @(posedge clk or negedge rst_n)
    begin
        if (!rst_n)
            max_val_r <= -8'sd128;
        else if (state == S_LOAD_CFG ||
                 (state == S_NORM_WR && out_wr_gnt &&
                  norm_idx_r == row_len_r - 1 && row_r < num_rows_r - 1))
            max_val_r <= -8'sd128;
        else if (state == S_FIND_MAX && act_rd_rvalid) begin
            if ($signed(act_rd_rdata) > max_val_r)
                max_val_r <= $signed(act_rd_rdata);
        end
    end

    // Sum accumulator (EXP_SUM pass)
    // The LUT diff is computed from rdata received during EXP_SUM;
    // since the LUT is combinational, we register the sum.
    logic        exp_sum_rvalid_d;
    logic [15:0] exp_sum_lut_d;

    always_ff @(posedge clk or negedge rst_n)
    begin
        if (!rst_n) begin
            exp_sum_rvalid_d <= 1'b0;
            exp_sum_lut_d    <= '0;
        end else begin
            exp_sum_rvalid_d <= (state == S_EXP_SUM) && act_rd_rvalid;
            if ((state == S_EXP_SUM) && act_rd_rvalid)
                exp_sum_lut_d <= lut_exp_val;
        end
    end

    always_ff @(posedge clk or negedge rst_n)
    begin
        if (!rst_n)
            sum_r <= '0;
        else if (state == S_LOAD_CFG ||
                 (state == S_NORM_WR && out_wr_gnt &&
                  norm_idx_r == row_len_r - 1 && row_r < num_rows_r - 1))
            sum_r <= '0;
        else if (exp_sum_rvalid_d)
            sum_r <= sum_r + {16'b0, exp_sum_lut_d};
    end

    // Normalize index counter
    always_ff @(posedge clk or negedge rst_n)
    begin
        if (!rst_n)
            norm_idx_r <= '0;
        else if (state == S_EXP_SUM && rcv_cnt_r == row_len_r)
            norm_idx_r <= '0;
        else if (state == S_NORM_WR && out_wr_gnt)
            norm_idx_r <= (norm_idx_r == row_len_r - 1) ? '0 : norm_idx_r + 1;
    end

    // Restoring divider logic
    logic [32:0] div_trial;
    assign div_trial = {div_rem_r[30:0], div_numer_r[23]} - {1'b0, sum_r};

    // Normalize: latch exp_val, start divider, and shift bits
    always_ff @(posedge clk or negedge rst_n)
    begin
        if (!rst_n) begin
            div_numer_r <= '0;
            div_rem_r   <= '0;
            div_quot_r  <= '0;
            div_cnt_r   <= '0;
        end else if (state == S_NORM_WAIT && act_rd_rvalid) begin
            // numerator = exp_val * 127 (max 23 bits)
            div_numer_r <= {8'b0, lut_exp_val} * 24'd127;
            div_rem_r   <= '0;
            div_quot_r  <= '0;
            div_cnt_r   <= '0;
        end else if (state == S_NORM_DIV) begin
            // Restoring divider: 24-bit numerator / 32-bit denominator (sum)
            // Shift remainder left, bring in next numerator bit
            div_cnt_r <= div_cnt_r + 1;
            if (!div_trial[32]) begin
                // trial >= 0: quotient bit = 1
                div_rem_r  <= div_trial[31:0];
                div_quot_r <= {div_quot_r[6:0], 1'b1};
            end else begin
                // trial < 0: quotient bit = 0
                div_rem_r  <= {div_rem_r[30:0], div_numer_r[23]};
                div_quot_r <= {div_quot_r[6:0], 1'b0};
            end
            div_numer_r <= {div_numer_r[22:0], 1'b0};
        end
    end

    // Clamp quotient to [0, 127]
    logic [DATA_W-1:0] norm_result;
    assign norm_result = (div_quot_r > 8'd127) ? 8'd127 : div_quot_r;

    // LUT diff input: always max - current_rdata
    // During EXP_SUM: combinational from rdata
    // During NORM_WAIT: combinational from rdata
    logic [7:0] diff_comb;
    assign diff_comb = 8'(max_val_r) - act_rd_rdata;
    assign lut_diff  = diff_comb;

    // Memory interface: act read
    always_comb
    begin
        act_rd_addr = '0;
        act_rd_req  = 1'b0;
        case (state)
            S_FIND_MAX: begin
                act_rd_addr = row_base_in + ADDR_W'(req_cnt_r);
                act_rd_req  = (req_cnt_r < row_len_r);
            end
            S_EXP_SUM: begin
                act_rd_addr = row_base_in + ADDR_W'(req_cnt_r);
                act_rd_req  = (req_cnt_r < row_len_r);
            end
            S_NORM_RD: begin
                act_rd_addr = row_base_in + ADDR_W'(norm_idx_r);
                act_rd_req  = 1'b1;
            end
            default: ;
        endcase
    end

    // Memory interface: output write
    assign out_wr_addr = row_base_out + ADDR_W'(norm_idx_r);
    assign out_wr_data = norm_result;
    assign out_wr_req  = (state == S_NORM_WR);

    // Status
    assign cmd_ready = (state == S_IDLE);
    assign done      = (state == S_DONE);
    assign busy      = (state != S_IDLE);

endmodule : softmax_ctrl
