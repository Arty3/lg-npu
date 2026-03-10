// ============================================================================
// gemm_addr_gen.sv - Compute SRAM addresses for GEMM operands
//   A is row-major M x K, B is row-major K x N.
// ============================================================================

module gemm_addr_gen
    import npu_types_pkg::*;
(
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

    // A[m, k] = a_base + m * K + k  (row-major M x K)
    assign a_addr = a_base + ADDR_W'(m * k_dim + k_idx);

    // B[k, n] = b_base + k * N + n  (row-major K x N)
    assign b_addr = b_base + ADDR_W'(k_idx * n_dim + n);

endmodule : gemm_addr_gen
