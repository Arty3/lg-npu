// ============================================================================
// conv_ctrl.sv - Convolution sequencer FSM
//   Walks the 6-deep loop nest (oh, ow, k, r, s, c) and drives all datapath
//   control signals.
// ============================================================================
module conv_ctrl
    import npu_types_pkg::*;
    import npu_cmd_pkg::*;
(
    input  logic                clk,
    input  logic                rst_n,

    // Command interface (from dispatch)
    input  conv_cmd_t           cmd,
    input  logic                cmd_valid,
    output logic                cmd_ready,

    // Loop indices (to addr_gen and writer)
    output dim_t                oh, ow, k, r, s, c,

    // Convolution parameters (to addr_gen)
    output logic [ADDR_W-1:0]   act_base,
    output logic [ADDR_W-1:0]   wt_base,
    output logic [ADDR_W-1:0]   out_base,
    output logic [ADDR_W-1:0]   bias_base,
    output dim_t                in_h, in_w, in_c, out_k, filt_r, filt_s,
    output dim_t                stride_h, stride_w, pad_h, pad_w,
    output logic [4:0]          quant_shift,
    output act_mode_e             act_mode,

    // Derived output dimensions
    output dim_t                out_h, out_w,

    // Datapath control
    output logic                iter_valid,   // Current iteration is valid
    input  logic                iter_ready,   // Datapath accepted it
    output logic                acc_clr,      // Clear accumulator (new output pixel)
    output logic                last_inner,   // Last (r,s,c) for this (oh,ow,k)

    // Completion
    output logic                conv_done,

    // Status
    output logic                busy
);

    typedef enum logic [2:0]
	{
        IDLE,
        LOAD_CFG,
        COMPUTE,
        DRAIN,
        DONE
    }   state_e;

    state_e state, state_next;

    // Latched command fields
    /* verilator lint_off UNUSEDSIGNAL */
    conv_cmd_t cmd_r;
    /* verilator lint_on UNUSEDSIGNAL */

    // Derived limits
    dim_t oh_max, ow_max;

    // Internal index registers
    dim_t oh_r, ow_r, k_r, r_r, s_r, c_r;

    // Signals
    logic inner_last;  // last (r,s,c)
    logic outer_last;  // last (oh,ow,k)
    logic advance;

    // State register
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            state <= IDLE;
        else
            state <= state_next;
    end

    // Next-state logic
    always_comb begin
        state_next = state;
        case (state)
            IDLE:     if (cmd_valid)                           state_next = LOAD_CFG;
            LOAD_CFG:                                          state_next = COMPUTE;
            COMPUTE:  if (advance && inner_last && outer_last) state_next = DRAIN;
            DRAIN:    if (iter_ready)                          state_next = DONE;
            DONE:                                              state_next = IDLE;
            default:                                           state_next = IDLE;
        endcase
    end

    // Command latch
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            cmd_r <= '0;
        end else if (state == IDLE && cmd_valid) begin
            cmd_r <= cmd;
        end
    end

    // Derived output dimensions
    //   OH = (H + 2*pad_h - R) / stride_h + 1
    //   OW = (W + 2*pad_w - S) / stride_w + 1
    assign oh_max = (cmd_r.in_h + 2*cmd_r.pad_h - cmd_r.filt_r) / cmd_r.stride_h;
    assign ow_max = (cmd_r.in_w + 2*cmd_r.pad_w - cmd_r.filt_s) / cmd_r.stride_w;

    // Loop counters
    assign advance    = (state == COMPUTE) && iter_ready;
    assign inner_last = (r_r == cmd_r.filt_r - 1) &&
                        (s_r == cmd_r.filt_s - 1) &&
                        (c_r == cmd_r.in_c   - 1);
    assign outer_last = (oh_r == oh_max) &&
                        (ow_r == ow_max) &&
                        (k_r  == cmd_r.out_k - 1);

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            oh_r <= '0; ow_r <= '0; k_r <= '0;
            r_r  <= '0; s_r  <= '0; c_r <= '0;
        end else if (state == LOAD_CFG) begin
            oh_r <= '0; ow_r <= '0; k_r <= '0;
            r_r  <= '0; s_r  <= '0; c_r <= '0;
        end else if (advance) begin
            // Inner loop: c -> s -> r
            if (c_r < cmd_r.in_c - 1) begin
                c_r <= c_r + 1;
            end else begin
                c_r <= '0;
                if (s_r < cmd_r.filt_s - 1) begin
                    s_r <= s_r + 1;
                end else begin
                    s_r <= '0;
                    if (r_r < cmd_r.filt_r - 1) begin
                        r_r <= r_r + 1;
                    end else begin
                        r_r <= '0;
                        // Outer loop: k -> ow -> oh
                        if (k_r < cmd_r.out_k - 1) begin
                            k_r <= k_r + 1;
                        end else begin
                            k_r <= '0;
                            if (ow_r < ow_max) begin
                                ow_r <= ow_r + 1;
                            end else begin
                                ow_r <= '0;
                                if (oh_r < oh_max) begin
                                    oh_r <= oh_r + 1;
                                end
                            end
                        end
                    end
                end
            end
        end
    end

    // Output assignments
    assign cmd_ready   = (state == IDLE);
    assign busy        = (state != IDLE);
    assign conv_done   = (state == DONE);
    assign iter_valid  = (state == COMPUTE);
    assign acc_clr     = (state == COMPUTE) && (r_r == '0) && (s_r == '0) && (c_r == '0);
    assign last_inner  = inner_last;

    // Loop indices
    assign oh = oh_r;
    assign ow = ow_r;
    assign k  = k_r;
    assign r  = r_r;
    assign s  = s_r;
    assign c  = c_r;

    // Parameters
    assign act_base    = cmd_r.act_in_addr;
    assign wt_base     = cmd_r.weight_addr;
    assign out_base    = cmd_r.act_out_addr;
    assign bias_base   = cmd_r.bias_addr;
    assign in_h        = cmd_r.in_h;
    assign in_w        = cmd_r.in_w;
    assign in_c        = cmd_r.in_c;
    assign out_k       = cmd_r.out_k;
    assign filt_r      = cmd_r.filt_r;
    assign filt_s      = cmd_r.filt_s;
    assign stride_h    = cmd_r.stride_h;
    assign stride_w    = cmd_r.stride_w;
    assign pad_h       = cmd_r.pad_h;
    assign pad_w       = cmd_r.pad_w;
    assign quant_shift = cmd_r.quant_shift;
    assign act_mode    = cmd_r.act_mode;
    assign out_h       = oh_max + 1;
    assign out_w       = ow_max + 1;

endmodule : conv_ctrl
