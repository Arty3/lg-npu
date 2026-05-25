// ============================================================================
// gemm_addr_gen.sv - Compute SRAM addresses for GEMM operands
//   A is row-major M x K, B is row-major K x N.
// ============================================================================

module gemm_addr_gen
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
    input  logic [ADDR_W-1:0]  a_base,
    input  logic [ADDR_W-1:0]  b_base,

    // Dimensions
    input  dim_t               n_dim,
    input  dim_t               k_dim,

    // Current loop indices
    input  dim_t               m,
    input  dim_t               n,
    input  dim_t               k_idx,

    // Outputs
    output logic [ADDR_W-1:0]  a_addr,
    output logic [ADDR_W-1:0]  b_addr
);

    typedef enum logic [1:0]
    {
        S_IDLE,
        S_STAGE1,
        S_OUT
    }   state_e;

    state_e state, state_next;

    logic [31:0] a_mul_s1, b_mul_s1;
    logic [ADDR_W-1:0] a_base_s1, b_base_s1;
    dim_t k_s1, n_s1;

    logic [ADDR_W-1:0] a_addr_r, b_addr_r;

    assign in_ready  = (state == S_IDLE);
    assign out_valid = (state == S_OUT);
    assign a_addr    = a_addr_r;
    assign b_addr    = b_addr_r;

    always_ff @(posedge clk or negedge rst_n)
    begin
        if (!rst_n)
            state <= S_IDLE;
        else
            state <= state_next;
    end

    always_comb
    begin
        state_next = state;
        case (state)
            S_IDLE:   if (in_valid)  state_next = S_STAGE1;
            S_STAGE1:               state_next = S_OUT;
            S_OUT:    if (out_ready) state_next = S_IDLE;
            default:                state_next = S_IDLE;
        endcase
    end

    always_ff @(posedge clk or negedge rst_n)
    begin
        if (!rst_n) begin
            a_mul_s1  <= '0;
            b_mul_s1  <= '0;
            a_base_s1 <= '0;
            b_base_s1 <= '0;
            k_s1      <= '0;
            n_s1      <= '0;
            a_addr_r  <= '0;
            b_addr_r  <= '0;
        end else begin
            if (state == S_IDLE && in_valid) begin
                a_mul_s1  <= {16'b0, m} * {16'b0, k_dim};
                b_mul_s1  <= {16'b0, k_idx} * {16'b0, n_dim};
                a_base_s1 <= a_base;
                b_base_s1 <= b_base;
                k_s1      <= k_idx;
                n_s1      <= n;
            end

            if (state == S_STAGE1) begin
                a_addr_r <= a_base_s1 + ADDR_W'(a_mul_s1 + {16'b0, k_s1});
                b_addr_r <= b_base_s1 + ADDR_W'(b_mul_s1 + {16'b0, n_s1});
            end
        end
    end

endmodule : gemm_addr_gen
