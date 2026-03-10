// ============================================================================
// npu_cmd_decode.sv - Validates opcode and packs descriptor into conv_cmd_t
// ============================================================================

module npu_cmd_decode
    import npu_types_pkg::*;
    import npu_cmd_pkg::*;
    import npu_addrmap_pkg::*;
(
    input  logic                clk,
    input  logic                rst_n,

    // From npu_cmd_fetch
    input  logic [31:0]         desc_word,
    input  logic [3:0]          desc_word_idx,
    input  logic                desc_word_valid,
    input  logic                desc_done,

    // Decoded command out
    output conv_cmd_t           cmd_out,
    output logic                cmd_valid,
    input  logic                cmd_ready,

    // Error flag (invalid opcode)
    output logic                decode_err,

    // Pipeline busy
    output logic                busy
);

    // Accumulate descriptor words into a register array
    logic [31:0] desc_regs [16];

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            for (int i = 0; i < 16; ++i)
                desc_regs[i] <= '0;
        end else if (desc_word_valid) begin
            desc_regs[desc_word_idx] <= desc_word;
        end
    end

    // Map words to fields when desc_done fires
    // Word layout (matches software command format):
    //   [0]  opcode (lower 4 bits)
    //   [1]  act_in_addr
    //   [2]  act_out_addr
    //   [3]  weight_addr
    //   [4]  bias_addr
    //   [5]  in_h
    //   [6]  in_w
    //   [7]  in_c
    //   [8]  out_k
    //   [9]  filt_r
    //   [10] filt_s
    //   [11] stride_h
    //   [12] stride_w
    //   [13] pad_h
    //   [14] pad_w
    //   [15] quant_shift (lower 5 bits)

    typedef enum logic [1:0]
	{
        S_IDLE,
        S_CHECK,
        S_VALID
    }   state_e;

    state_e     state, state_next;
    conv_cmd_t  cmd_r;
    logic       err_r;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            state <= S_IDLE;
        else
            state <= state_next;
    end

    always_comb begin
        state_next = state;
        case (state)
            S_IDLE:  if (desc_done)               state_next = S_CHECK;
            S_CHECK:                              state_next = S_VALID;
            S_VALID: if (cmd_ready || err_r)      state_next = S_IDLE;
            default:                              state_next = S_IDLE;
        endcase
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            cmd_r <= '0;
            err_r <= 1'b0;
        end else if (state == S_CHECK) begin
            err_r <= (desc_regs[0][3:0] != OP_CONV) &&
                     (desc_regs[0][3:0] != OP_GEMM) &&
                     (desc_regs[0][3:0] != OP_SOFTMAX) &&
                     (desc_regs[0][3:0] != OP_VEC);
            cmd_r.opcode       <= opcode_e'(desc_regs[0][3:0]);
            cmd_r.act_in_addr  <= desc_regs[1][ADDR_W-1:0];
            cmd_r.act_out_addr <= desc_regs[2][ADDR_W-1:0];
            cmd_r.weight_addr  <= desc_regs[3][ADDR_W-1:0];
            cmd_r.bias_addr    <= desc_regs[4][ADDR_W-1:0];
            cmd_r.in_h         <= desc_regs[5][DIM_W-1:0];
            cmd_r.in_w         <= desc_regs[6][DIM_W-1:0];
            cmd_r.in_c         <= desc_regs[7][DIM_W-1:0];
            cmd_r.out_k        <= desc_regs[8][DIM_W-1:0];
            cmd_r.filt_r       <= desc_regs[9][DIM_W-1:0];
            cmd_r.filt_s       <= desc_regs[10][DIM_W-1:0];
            cmd_r.stride_h     <= desc_regs[11][DIM_W-1:0];
            cmd_r.stride_w     <= desc_regs[12][DIM_W-1:0];
            cmd_r.pad_h        <= desc_regs[13][DIM_W-1:0];
            cmd_r.pad_w        <= desc_regs[14][DIM_W-1:0];
            cmd_r.quant_shift  <= desc_regs[15][4:0];
            cmd_r.act_mode     <= act_mode_e'(desc_regs[15][6:5]);
        end
    end

    assign cmd_out    = cmd_r;
    assign cmd_valid  = (state == S_VALID) && !err_r;
    assign decode_err = (state == S_VALID) && err_r;
    assign busy       = (state != S_IDLE);

endmodule : npu_cmd_decode
