// ============================================================================
// npu_buffer_router.sv - Routes memory accesses between host, backend, buffers
// ============================================================================

module npu_buffer_router
    import npu_types_pkg::*;
    import npu_cfg_pkg::*;
    import npu_addrmap_pkg::*;
(
    input  logic                    clk,
    input  logic                    rst_n,

    // Host MMIO buffer window
    input  logic [MMIO_ADDR_W-1:0]  host_addr,
    input  logic [MMIO_DATA_W-1:0]  host_wdata,
    input  logic                    host_wr,
    input  logic                    host_req,
    output logic                    host_gnt,
    output logic [MMIO_DATA_W-1:0]  host_rdata,
    output logic                    host_rvalid,

    /* verilator lint_off UNUSEDSIGNAL */
    // Backend -> weight buffer read
    input  logic [ADDR_W-1:0]       be_wt_addr,
    input  logic                    be_wt_req,
    output logic                    be_wt_gnt,
    output logic [DATA_W-1:0]       be_wt_rdata,
    output logic                    be_wt_rvalid,

    // Backend -> activation buffer read
    input  logic [ADDR_W-1:0]       be_act_rd_addr,
    input  logic                    be_act_rd_req,
    output logic                    be_act_rd_gnt,
    output logic [DATA_W-1:0]       be_act_rd_rdata,
    output logic                    be_act_rd_rvalid,

    // Backend -> activation buffer write
    input  logic [ADDR_W-1:0]       be_act_wr_addr,
    input  logic [DATA_W-1:0]       be_act_wr_data,
    input  logic                    be_act_wr_req,
    output logic                    be_act_wr_gnt,

    // Backend -> bias read (uses weight buffer address space)
    input  logic [ADDR_W-1:0]       be_bias_addr,
    input  logic                    be_bias_req,
    /* verilator lint_on UNUSEDSIGNAL */
    output logic                    be_bias_gnt,
    output logic [DATA_W-1:0]       be_bias_rdata,
    output logic                    be_bias_rvalid
);

    // Decode host address into buffer select
    logic host_sel_wt, host_sel_act, host_sel_psum;

    assign host_sel_wt   = (host_addr >= WEIGHT_BUF_BASE) &&
                           (host_addr <  ACT_BUF_BASE);
    assign host_sel_act  = (host_addr >= ACT_BUF_BASE) &&
                           (host_addr <  PSUM_BUF_BASE);
    assign host_sel_psum = (host_addr >= PSUM_BUF_BASE);

    logic [SRAM_ADDR_W-1:0] host_wt_offset;
    logic [SRAM_ADDR_W:0]   host_act_offset;  // one bit wider for double bank
    logic [SRAM_ADDR_W-1:0] host_psum_offset;

    assign host_wt_offset   = host_addr[SRAM_ADDR_W-1:0];
    assign host_act_offset  = host_addr[SRAM_ADDR_W:0];
    assign host_psum_offset = host_addr[SRAM_ADDR_W-1:0];

    // Weight buffer instance
    logic                   wt_host_gnt, wt_host_rvalid;
    logic [DATA_W-1:0]      wt_host_rdata;

    npu_weight_buffer u_wt_buf (
        .clk          (clk),
        .rst_n        (rst_n),
        .host_addr    (host_wt_offset),
        .host_wdata   (host_wdata[DATA_W-1:0]),
        .host_we      (host_wr),
        .host_req     (host_req & host_sel_wt),
        .host_gnt     (wt_host_gnt),
        .host_rdata   (wt_host_rdata),
        .host_rvalid  (wt_host_rvalid),
        .be_addr      (be_wt_addr[SRAM_ADDR_W-1:0]),
        .be_req       (be_wt_req),
        .be_gnt       (be_wt_gnt),
        .be_rdata     (be_wt_rdata),
        .be_rvalid    (be_wt_rvalid)
    );

    // Bias reads share the weight buffer (bias is stored as part of weights)
    // For simplicity, bias requests are treated as weight-buffer reads.
    // TODO: When both bias and weight read happen simultaneously,
    //       they will need a second port or time-multiplexing.
    assign be_bias_gnt    = 1'b0;  // stub: bias is fetched inline
    assign be_bias_rdata  = '0;
    assign be_bias_rvalid = 1'b0;

    // Activation buffer instance
    logic                 act_host_gnt, act_host_rvalid;
    logic [DATA_W-1:0]    act_host_rdata;

    npu_act_buffer u_act_buf (
        .clk           (clk),
        .rst_n         (rst_n),
        .host_addr     (host_act_offset[SRAM_ADDR_W:0]),
        .host_wdata    (host_wdata[DATA_W-1:0]),
        .host_we       (host_wr),
        .host_req      (host_req & host_sel_act),
        .host_gnt      (act_host_gnt),
        .host_rdata    (act_host_rdata),
        .host_rvalid   (act_host_rvalid),
        .be_rd_addr    (be_act_rd_addr[SRAM_ADDR_W:0]),
        .be_rd_req     (be_act_rd_req),
        .be_rd_gnt     (be_act_rd_gnt),
        .be_rd_rdata   (be_act_rd_rdata),
        .be_rd_rvalid  (be_act_rd_rvalid),
        .be_wr_addr    (be_act_wr_addr[SRAM_ADDR_W:0]),
        .be_wr_data    (be_act_wr_data),
        .be_wr_req     (be_act_wr_req),
        .be_wr_gnt     (be_act_wr_gnt)
    );

    // Psum buffer instance
    logic              psum_host_gnt, psum_host_rvalid;
    logic [ACC_W-1:0]  psum_host_rdata;

    npu_psum_buffer u_psum_buf (
        .clk          (clk),
        .rst_n        (rst_n),
        .host_addr    (host_psum_offset),
        .host_wdata   (host_wdata),
        .host_we      (host_wr),
        .host_req     (host_req & host_sel_psum),
        .host_gnt     (psum_host_gnt),
        .host_rdata   (psum_host_rdata),
        .host_rvalid  (psum_host_rvalid)
    );

    // Host response mux
    assign host_gnt = (host_sel_wt   & wt_host_gnt)   |
                      (host_sel_act  & act_host_gnt)  |
                      (host_sel_psum & psum_host_gnt);

    // Read data mux (delayed one cycle to match rvalid)
    assign host_rvalid = wt_host_rvalid   | act_host_rvalid | psum_host_rvalid;
    assign host_rdata  = wt_host_rvalid   ? {{(MMIO_DATA_W-DATA_W){1'b0}}, wt_host_rdata} :
                         act_host_rvalid  ? {{(MMIO_DATA_W-DATA_W){1'b0}}, act_host_rdata} :
                         psum_host_rvalid ? psum_host_rdata : '0;

endmodule : npu_buffer_router
