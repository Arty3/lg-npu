// ============================================================================
// npu_mem_top.sv - Top-level memory subsystem
//   Instantiates npu_buffer_router (which contains the buffer banks).
// ============================================================================

module npu_mem_top
    import npu_types_pkg::*;
    import npu_cfg_pkg::*;
    import npu_addrmap_pkg::*;
(
    input  logic                    clk,
    input  logic                    rst_n,

    // Host path (buffer window portion of MMIO)
    input  logic [MMIO_ADDR_W-1:0]  host_addr,
    input  logic [MMIO_DATA_W-1:0]  host_wdata,
    input  logic                    host_wr,
    input  logic                    host_req,
    output logic                    host_gnt,
    output logic [MMIO_DATA_W-1:0]  host_rdata,
    output logic                    host_rvalid,

    // Backend weight read
    input  logic [ADDR_W-1:0]       be_wt_addr,
    input  logic                    be_wt_req,
    output logic                    be_wt_gnt,
    output logic [DATA_W-1:0]       be_wt_rdata,
    output logic                    be_wt_rvalid,

    // Backend activation read
    input  logic [ADDR_W-1:0]       be_act_rd_addr,
    input  logic                    be_act_rd_req,
    output logic                    be_act_rd_gnt,
    output logic [DATA_W-1:0]       be_act_rd_rdata,
    output logic                    be_act_rd_rvalid,

    // Backend activation write
    input  logic [ADDR_W-1:0]       be_act_wr_addr,
    input  logic [DATA_W-1:0]       be_act_wr_data,
    input  logic                    be_act_wr_req,
    output logic                    be_act_wr_gnt,

    // Backend bias read
    input  logic [ADDR_W-1:0]       be_bias_addr,
    input  logic                    be_bias_req,
    output logic                    be_bias_gnt,
    output logic [DATA_W-1:0]       be_bias_rdata,
    output logic                    be_bias_rvalid
);

    npu_buffer_router u_router (
        .clk              (clk),
        .rst_n            (rst_n),
        .host_addr        (host_addr),
        .host_wdata       (host_wdata),
        .host_wr          (host_wr),
        .host_req         (host_req),
        .host_gnt         (host_gnt),
        .host_rdata       (host_rdata),
        .host_rvalid      (host_rvalid),
        .be_wt_addr       (be_wt_addr),
        .be_wt_req        (be_wt_req),
        .be_wt_gnt        (be_wt_gnt),
        .be_wt_rdata      (be_wt_rdata),
        .be_wt_rvalid     (be_wt_rvalid),
        .be_act_rd_addr   (be_act_rd_addr),
        .be_act_rd_req    (be_act_rd_req),
        .be_act_rd_gnt    (be_act_rd_gnt),
        .be_act_rd_rdata  (be_act_rd_rdata),
        .be_act_rd_rvalid (be_act_rd_rvalid),
        .be_act_wr_addr   (be_act_wr_addr),
        .be_act_wr_data   (be_act_wr_data),
        .be_act_wr_req    (be_act_wr_req),
        .be_act_wr_gnt    (be_act_wr_gnt),
        .be_bias_addr     (be_bias_addr),
        .be_bias_req      (be_bias_req),
        .be_bias_gnt      (be_bias_gnt),
        .be_bias_rdata    (be_bias_rdata),
        .be_bias_rvalid   (be_bias_rvalid)
    );

endmodule : npu_mem_top
