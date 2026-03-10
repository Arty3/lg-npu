// ============================================================================
// npu_psum_buffer.sv - INT32 partial-sum SRAM bank
//   In the current scope partial sums are held in conv_accum registers,
//   not in this buffer. This module is wired up but functionally a stub
//   for future tiling support.
// ============================================================================

module npu_psum_buffer
    import npu_types_pkg::*;
    import npu_cfg_pkg::*;
(
    input  logic                       clk,
    input  logic                       rst_n,

    // Host MMIO port
    input  logic [SRAM_ADDR_W-1:0]     host_addr,
    input  logic [ACC_W-1:0]           host_wdata,
    input  logic                       host_we,
    input  logic                       host_req,
    output logic                       host_gnt,
    output logic [ACC_W-1:0]           host_rdata,
    output logic                       host_rvalid
);

    npu_local_mem_wrap #(
        .DEPTH  (SRAM_DEPTH),
        .DATA_W (ACC_W)
    ) u_sram (
        .clk    (clk),
        .rst_n  (rst_n),
        .addr   (host_addr),
        .wdata  (host_wdata),
        .we     (host_we),
        .req    (host_req),
        .gnt    (host_gnt),
        .rdata  (host_rdata),
        .rvalid (host_rvalid)
    );

endmodule : npu_psum_buffer
