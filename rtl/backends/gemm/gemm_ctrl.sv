// ============================================================================
// gemm_ctrl.sv - GEMM sequencer FSM
//   Walks the 3-deep loop nest (m, n, k) and drives all datapath
//   control signals.
// ============================================================================

module gemm_ctrl
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
    output dim_t                m,
    output dim_t                n,
    output dim_t                k_idx,

    // GEMM parameters (to addr_gen)
    output logic [ADDR_W-1:0]   a_base,
    output logic [ADDR_W-1:0]   b_base,
    output logic [ADDR_W-1:0]   c_base,
    output logic [ADDR_W-1:0]   bias_base,
    output dim_t                m_dim,
    output dim_t                n_dim,
    output dim_t                k_dim,
    output logic [4:0]          quant_shift,
    output act_mode_e           act_mode,

    // Datapath control
    output logic                iter_valid,
    input  logic                iter_ready,
    output logic                acc_clr,
    output logic                last_inner,

    // Completion
    output logic                gemm_done,

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

    // Internal index registers
    dim_t m_r, n_r, k_r;

    // Signals
    logic inner_last;
    logic outer_last;
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
        if (!rst_n)
            cmd_r <= '0;
        else if (state == IDLE && cmd_valid)
            cmd_r <= cmd;
    end

    // Loop counters
    // Conv cmd fields reused: in_h = M, in_w = N, in_c = K
    assign advance    = (state == COMPUTE) && iter_ready;
    assign inner_last = (k_r == cmd_r.in_c - 1);
    assign outer_last = (m_r == cmd_r.in_h - 1) &&
                        (n_r == cmd_r.in_w - 1);

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            m_r <= '0;
            n_r <= '0;
            k_r <= '0;
        end else if (state == LOAD_CFG) begin
            m_r <= '0;
            n_r <= '0;
            k_r <= '0;
        end else if (advance) begin
            // Inner loop: k
            if (k_r < cmd_r.in_c - 1) begin
                k_r <= k_r + 1;
            end else begin
                k_r <= '0;
                // Outer loop: n -> m
                if (n_r < cmd_r.in_w - 1) begin
                    n_r <= n_r + 1;
                end else begin
                    n_r <= '0;
                    if (m_r < cmd_r.in_h - 1)
                        m_r <= m_r + 1;
                end
            end
        end
    end

    // Output assignments
    assign cmd_ready  = (state == IDLE);
    assign busy       = (state != IDLE);
    assign gemm_done  = (state == DONE);
    assign iter_valid = (state == COMPUTE);
    assign acc_clr    = (state == COMPUTE) && (k_r == '0);
    assign last_inner = inner_last;

    // Loop indices
    assign m     = m_r;
    assign n     = n_r;
    assign k_idx = k_r;

    // Parameters (mapped from conv_cmd_t fields)
    assign a_base      = cmd_r.act_in_addr;
    assign b_base      = cmd_r.weight_addr;
    assign c_base      = cmd_r.act_out_addr;
    assign bias_base   = cmd_r.bias_addr;
    assign m_dim       = cmd_r.in_h;
    assign n_dim       = cmd_r.in_w;
    assign k_dim       = cmd_r.in_c;
    assign quant_shift = cmd_r.quant_shift;
    assign act_mode    = cmd_r.act_mode;

endmodule : gemm_ctrl
