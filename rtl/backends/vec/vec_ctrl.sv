// ============================================================================
// vec_ctrl.sv - Element-wise vector operation FSM
//   Supports ADD (saturating INT8) and MUL (with activation + quantize).
//   Reads source A from activation buffer, source B from weight buffer,
//   writes output C to activation buffer.
//   Parallel reads from independent SRAM banks, ~3 cycles per element.
// ============================================================================

module vec_ctrl
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

    // Memory - activation buffer read (source A)
    output logic [ADDR_W-1:0]   act_rd_addr,
    output logic                act_rd_req,
    input  logic                act_rd_gnt,
    input  logic [DATA_W-1:0]   act_rd_rdata,
    input  logic                act_rd_rvalid,

    // Memory - weight buffer read (source B)
    output logic [ADDR_W-1:0]   wt_rd_addr,
    output logic                wt_rd_req,
    input  logic                wt_rd_gnt,
    input  logic [DATA_W-1:0]   wt_rd_rdata,
    input  logic                wt_rd_rvalid,

    // Memory - activation buffer write (output C)
    output logic [ADDR_W-1:0]   out_wr_addr,
    output logic [DATA_W-1:0]   out_wr_data,
    output logic                out_wr_req,
    input  logic                out_wr_gnt,

    // Status
    output logic                done,
    output logic                busy
);

    // Sub-operation codes (encoded in in_w field)
    localparam dim_t VEC_ADD = '0;

    typedef enum logic [2:0]
    {
        S_IDLE,
        S_LOAD_CFG,
        S_READ,
        S_WAIT,
        S_WRITE,
        S_DONE
    }   state_e;

    state_e state, state_next;

    // Latched command fields
    logic [ADDR_W-1:0] a_base_r, b_base_r, out_base_r;
    dim_t              length_r;
    dim_t              vec_op_r;
    logic [4:0]        qshift_r;
    act_mode_e         act_mode_r;

    // Element index
    dim_t idx_r;

    // Read tracking
    logic              a_gnt_r, b_gnt_r;
    logic              a_val_r, b_val_r;
    logic [DATA_W-1:0] a_data_r, b_data_r;

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
            S_IDLE:     if (cmd_valid)                            state_next = S_LOAD_CFG;
            S_LOAD_CFG:                                           state_next = S_READ;
            S_READ:     if ((a_gnt_r || act_rd_gnt) &&
                            (b_gnt_r || wt_rd_gnt))              state_next = S_WAIT;
            S_WAIT:     if ((a_val_r || act_rd_rvalid) &&
                            (b_val_r || wt_rd_rvalid))           state_next = S_WRITE;
            S_WRITE: begin
                if (out_wr_gnt) begin
                    if (idx_r == length_r - 1)                    state_next = S_DONE;
                    else                                          state_next = S_READ;
                end
            end
            S_DONE:                                               state_next = S_IDLE;
            default:                                              state_next = S_IDLE;
        endcase
    end

    // Command latch
    always_ff @(posedge clk or negedge rst_n)
    begin
        if (!rst_n) begin
            a_base_r   <= '0;
            b_base_r   <= '0;
            out_base_r <= '0;
            length_r   <= '0;
            vec_op_r   <= '0;
            qshift_r   <= '0;
            act_mode_r <= ACT_NONE;
        end else if (state == S_IDLE && cmd_valid) begin
            a_base_r   <= cmd.act_in_addr;
            b_base_r   <= cmd.weight_addr;
            out_base_r <= cmd.act_out_addr;
            length_r   <= cmd.in_h;
            vec_op_r   <= cmd.in_w;
            qshift_r   <= cmd.quant_shift;
            act_mode_r <= cmd.act_mode;
        end
    end

    // Element index
    always_ff @(posedge clk or negedge rst_n)
    begin
        if (!rst_n)
            idx_r <= '0;
        else if (state == S_LOAD_CFG)
            idx_r <= '0;
        else if (state == S_WRITE && out_wr_gnt)
            idx_r <= idx_r + 1;
    end

    // Grant tracking flags
    always_ff @(posedge clk or negedge rst_n)
    begin
        if (!rst_n) begin
            a_gnt_r <= 1'b0;
            b_gnt_r <= 1'b0;
        end else if (state == S_LOAD_CFG ||
                     (state == S_WRITE && out_wr_gnt)) begin
            a_gnt_r <= 1'b0;
            b_gnt_r <= 1'b0;
        end else if (state == S_READ) begin
            if (act_rd_gnt) a_gnt_r <= 1'b1;
            if (wt_rd_gnt)  b_gnt_r <= 1'b1;
        end
    end

    // Data tracking flags and latches
    always_ff @(posedge clk or negedge rst_n)
    begin
        if (!rst_n) begin
            a_val_r  <= 1'b0;
            b_val_r  <= 1'b0;
            a_data_r <= '0;
            b_data_r <= '0;
        end else if (state == S_LOAD_CFG ||
                     (state == S_WRITE && out_wr_gnt)) begin
            a_val_r <= 1'b0;
            b_val_r <= 1'b0;
        end else if (state == S_WAIT) begin
            if (act_rd_rvalid) begin
                a_val_r  <= 1'b1;
                a_data_r <= act_rd_rdata;
            end
            if (wt_rd_rvalid) begin
                b_val_r  <= 1'b1;
                b_data_r <= wt_rd_rdata;
            end
        end
    end

    // Compute result
    // ADD: clamp(A + B, -128, 127)
    // MUL: A * B -> activate -> shift -> clamp to INT8
    logic signed [ACC_W-1:0] mul_acc;
    logic signed [ACC_W-1:0] mul_activated;
    logic signed [ACC_W-1:0] mul_shifted;
    logic signed [8:0]       add_sum;
    logic [DATA_W-1:0]       result;

    assign mul_acc = ACC_W'($signed(a_data_r)) * ACC_W'($signed(b_data_r));

    always_comb
    begin
        case (act_mode_r)
            ACT_RELU:       mul_activated = (mul_acc < 0) ? '0 : mul_acc;
            ACT_LEAKY_RELU: mul_activated = (mul_acc < 0) ? (mul_acc >>> LEAKY_SHIFT) : mul_acc;
            default:        mul_activated = mul_acc;
        endcase
    end

    assign mul_shifted = mul_activated >>> qshift_r;
    assign add_sum     = {a_data_r[DATA_W-1], a_data_r} + {b_data_r[DATA_W-1], b_data_r};

    always_comb
    begin
        if (vec_op_r == VEC_ADD) begin
            if (add_sum > 9'sd127)       result = 8'd127;
            else if (add_sum < -9'sd128) result = 8'h80;
            else                         result = add_sum[DATA_W-1:0];
        end else begin
            if (mul_shifted > ACC_W'(signed'(8'sd127)))        result = 8'd127;
            else if (mul_shifted < ACC_W'(signed'(-8'sd128)))  result = 8'h80;
            else                                               result = mul_shifted[DATA_W-1:0];
        end
    end

    // Memory interface - act read (source A)
    assign act_rd_addr = a_base_r + ADDR_W'(idx_r);
    assign act_rd_req  = (state == S_READ) && !a_gnt_r;

    // Memory interface - wt read (source B)
    assign wt_rd_addr = b_base_r + ADDR_W'(idx_r);
    assign wt_rd_req  = (state == S_READ) && !b_gnt_r;

    // Memory interface - output write
    assign out_wr_addr = out_base_r + ADDR_W'(idx_r);
    assign out_wr_data = result;
    assign out_wr_req  = (state == S_WRITE);

    // Status
    assign cmd_ready = (state == S_IDLE);
    assign done      = (state == S_DONE);
    assign busy      = (state != S_IDLE);

endmodule : vec_ctrl
