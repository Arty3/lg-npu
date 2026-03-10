// ============================================================================
// npu_core.sv - Core: scheduler -> dispatch -> backend + memory + completion
// ============================================================================

module npu_core
    import npu_types_pkg::*;
    import npu_cmd_pkg::*;
(
    input  logic                   clk,
    input  logic                   rst_n,

    // Command queue interface (from npu_queue)
    input  conv_cmd_t              queue_cmd,
    input  logic                   queue_valid,
    output logic                   queue_ready,

    // Memory system (buffer window for host via router)
    input  logic [MMIO_ADDR_W-1:0] buf_addr,
    input  logic [MMIO_DATA_W-1:0] buf_wdata,
    input  logic                   buf_wr,
    input  logic                   buf_req,
    output logic                   buf_gnt,
    output logic [MMIO_DATA_W-1:0] buf_rdata,
    output logic                   buf_rvalid,

    // IRQ
    output logic                   cmd_done,

    // Status
    output logic                   backend_busy,
    output logic                   backend_active, // for perf counters
    output logic                   backend_stall
);

    // Scheduler -> Dispatch wires
    conv_cmd_t disp_cmd;
    logic      disp_valid, disp_ready;

    // Dispatch -> Backend wires
    conv_cmd_t be_cmd;
    logic      be_cmd_valid, be_cmd_ready;

    npu_scheduler u_sched (
        .clk            (clk),
        .rst_n          (rst_n),
        .queue_cmd      (queue_cmd),
        .queue_valid    (queue_valid),
        .queue_ready    (queue_ready),
        .dispatch_cmd   (disp_cmd),
        .dispatch_valid (disp_valid),
        .dispatch_ready (disp_ready),
        .backend_done   (be_done)
    );

    npu_dispatch u_dispatch (
        .sched_cmd    (disp_cmd),
        .sched_valid  (disp_valid),
        .sched_ready  (disp_ready),
        .be_cmd       (be_cmd),
        .be_cmd_valid (be_cmd_valid),
        .be_cmd_ready (be_cmd_ready)
    );

    // Backend memory ports
    logic [ADDR_W-1:0] be_act_rd_addr,   be_wt_rd_addr, be_out_wr_addr;
    logic [DATA_W-1:0] be_out_wr_data;
    logic              be_act_rd_req,    be_wt_rd_req,  be_out_wr_req;
    logic              be_act_rd_gnt,    be_wt_rd_gnt,  be_out_wr_gnt;
    logic [DATA_W-1:0] be_act_rd_rdata,  be_wt_rd_rdata;
    logic              be_act_rd_rvalid, be_wt_rd_rvalid;

    logic [ADDR_W-1:0] be_bias_addr;
    logic              be_bias_req, be_bias_gnt;
    logic [DATA_W-1:0] be_bias_rdata;
    logic              be_bias_rvalid;

    logic              be_done, be_busy;

    conv_backend u_backend (
        .clk            (clk),
        .rst_n          (rst_n),
        .cmd            (be_cmd),
        .cmd_valid      (be_cmd_valid),
        .cmd_ready      (be_cmd_ready),
        .act_rd_addr    (be_act_rd_addr),
        .act_rd_req     (be_act_rd_req),
        .act_rd_gnt     (be_act_rd_gnt),
        .act_rd_rdata   (be_act_rd_rdata),
        .act_rd_rvalid  (be_act_rd_rvalid),
        .wt_rd_addr     (be_wt_rd_addr),
        .wt_rd_req      (be_wt_rd_req),
        .wt_rd_gnt      (be_wt_rd_gnt),
        .wt_rd_rdata    (be_wt_rd_rdata),
        .wt_rd_rvalid   (be_wt_rd_rvalid),
        .out_wr_addr    (be_out_wr_addr),
        .out_wr_data    (be_out_wr_data),
        .out_wr_req     (be_out_wr_req),
        .out_wr_gnt     (be_out_wr_gnt),
        .bias_rd_addr   (be_bias_addr),
        .bias_rd_req    (be_bias_req),
        .bias_rd_gnt    (be_bias_gnt),
        .bias_rd_rdata  (be_bias_rdata),
        .bias_rd_rvalid (be_bias_rvalid),
        .done           (be_done),
        .busy           (be_busy)
    );

    // Memory top
    npu_mem_top u_mem (
        .clk              (clk),
        .rst_n            (rst_n),
        .host_addr        (buf_addr),
        .host_wdata       (buf_wdata),
        .host_wr          (buf_wr),
        .host_req         (buf_req),
        .host_gnt         (buf_gnt),
        .host_rdata       (buf_rdata),
        .host_rvalid      (buf_rvalid),
        .be_wt_addr       (be_wt_rd_addr),
        .be_wt_req        (be_wt_rd_req),
        .be_wt_gnt        (be_wt_rd_gnt),
        .be_wt_rdata      (be_wt_rd_rdata),
        .be_wt_rvalid     (be_wt_rd_rvalid),
        .be_act_rd_addr   (be_act_rd_addr),
        .be_act_rd_req    (be_act_rd_req),
        .be_act_rd_gnt    (be_act_rd_gnt),
        .be_act_rd_rdata  (be_act_rd_rdata),
        .be_act_rd_rvalid (be_act_rd_rvalid),
        .be_act_wr_addr   (be_out_wr_addr),
        .be_act_wr_data   (be_out_wr_data),
        .be_act_wr_req    (be_out_wr_req),
        .be_act_wr_gnt    (be_out_wr_gnt),
        .be_bias_addr     (be_bias_addr),
        .be_bias_req      (be_bias_req),
        .be_bias_gnt      (be_bias_gnt),
        .be_bias_rdata    (be_bias_rdata),
        .be_bias_rvalid   (be_bias_rvalid)
    );

    // Completion
    npu_completion u_completion (
        .backend_done  (be_done),
        .cmd_done      (cmd_done)
    );

    // Status
    assign backend_busy   = be_busy;
    assign backend_active = be_busy & ~backend_stall;
    assign backend_stall  = be_busy & ~be_act_rd_gnt;  // simplified stall metric

endmodule : npu_core
