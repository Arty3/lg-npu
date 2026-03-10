// ============================================================================
// npu_core.sv - Core: scheduler -> dispatch -> backends + memory + completion
//   Supports both GEMM and convolution backends.  Only one backend is
//   active at a time (in-order scheduler).  Memory ports are muxed based
//   on which backend is busy.
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

    // Dispatch -> GEMM backend
    conv_cmd_t gemm_be_cmd;
    logic      gemm_be_cmd_valid, gemm_be_cmd_ready;

    // Dispatch -> Conv backend
    conv_cmd_t conv_be_cmd;
    logic      conv_be_cmd_valid, conv_be_cmd_ready;

    logic be_done, be_busy;

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
        .sched_cmd      (disp_cmd),
        .sched_valid    (disp_valid),
        .sched_ready    (disp_ready),
        .gemm_cmd       (gemm_be_cmd),
        .gemm_cmd_valid (gemm_be_cmd_valid),
        .gemm_cmd_ready (gemm_be_cmd_ready),
        .conv_cmd       (conv_be_cmd),
        .conv_cmd_valid (conv_be_cmd_valid),
        .conv_cmd_ready (conv_be_cmd_ready)
    );

    // Shared memory signals (to/from mem_top)
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

    // GEMM backend
    logic [ADDR_W-1:0] gemm_act_rd_addr, gemm_wt_rd_addr, gemm_out_wr_addr;
    logic [DATA_W-1:0] gemm_out_wr_data;
    logic              gemm_act_rd_req,  gemm_wt_rd_req,  gemm_out_wr_req;
    logic [ADDR_W-1:0] gemm_bias_addr;
    logic              gemm_bias_req;
    logic              gemm_done, gemm_busy;

    gemm_backend u_gemm (
        .clk            (clk),
        .rst_n          (rst_n),
        .cmd            (gemm_be_cmd),
        .cmd_valid      (gemm_be_cmd_valid),
        .cmd_ready      (gemm_be_cmd_ready),
        .act_rd_addr    (gemm_act_rd_addr),
        .act_rd_req     (gemm_act_rd_req),
        .act_rd_gnt     (be_act_rd_gnt),
        .act_rd_rdata   (be_act_rd_rdata),
        .act_rd_rvalid  (be_act_rd_rvalid),
        .wt_rd_addr     (gemm_wt_rd_addr),
        .wt_rd_req      (gemm_wt_rd_req),
        .wt_rd_gnt      (be_wt_rd_gnt),
        .wt_rd_rdata    (be_wt_rd_rdata),
        .wt_rd_rvalid   (be_wt_rd_rvalid),
        .out_wr_addr    (gemm_out_wr_addr),
        .out_wr_data    (gemm_out_wr_data),
        .out_wr_req     (gemm_out_wr_req),
        .out_wr_gnt     (be_out_wr_gnt),
        .bias_rd_addr   (gemm_bias_addr),
        .bias_rd_req    (gemm_bias_req),
        .bias_rd_gnt    (be_bias_gnt),
        .bias_rd_rdata  (be_bias_rdata),
        .bias_rd_rvalid (be_bias_rvalid),
        .done           (gemm_done),
        .busy           (gemm_busy)
    );

    // Convolution backend
    logic [ADDR_W-1:0] conv_act_rd_addr, conv_wt_rd_addr, conv_out_wr_addr;
    logic [DATA_W-1:0] conv_out_wr_data;
    logic              conv_act_rd_req,  conv_wt_rd_req,  conv_out_wr_req;
    logic [ADDR_W-1:0] conv_bias_addr;
    logic              conv_bias_req;
    logic              conv_done, conv_busy;

    conv_backend u_conv (
        .clk            (clk),
        .rst_n          (rst_n),
        .cmd            (conv_be_cmd),
        .cmd_valid      (conv_be_cmd_valid),
        .cmd_ready      (conv_be_cmd_ready),
        .act_rd_addr    (conv_act_rd_addr),
        .act_rd_req     (conv_act_rd_req),
        .act_rd_gnt     (be_act_rd_gnt),
        .act_rd_rdata   (be_act_rd_rdata),
        .act_rd_rvalid  (be_act_rd_rvalid),
        .wt_rd_addr     (conv_wt_rd_addr),
        .wt_rd_req      (conv_wt_rd_req),
        .wt_rd_gnt      (be_wt_rd_gnt),
        .wt_rd_rdata    (be_wt_rd_rdata),
        .wt_rd_rvalid   (be_wt_rd_rvalid),
        .out_wr_addr    (conv_out_wr_addr),
        .out_wr_data    (conv_out_wr_data),
        .out_wr_req     (conv_out_wr_req),
        .out_wr_gnt     (be_out_wr_gnt),
        .bias_rd_addr   (conv_bias_addr),
        .bias_rd_req    (conv_bias_req),
        .bias_rd_gnt    (be_bias_gnt),
        .bias_rd_rdata  (be_bias_rdata),
        .bias_rd_rvalid (be_bias_rvalid),
        .done           (conv_done),
        .busy           (conv_busy)
    );

    // Memory port mux (registered selector avoids combinational loops)
    // Only one backend is active at a time (in-order scheduler), so we
    // track which backend was last dispatched and route accordingly.
    logic gemm_sel_r;
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            gemm_sel_r <= 1'b0;
        else if (gemm_be_cmd_valid && gemm_be_cmd_ready)
            gemm_sel_r <= 1'b1;
        else if (conv_be_cmd_valid && conv_be_cmd_ready)
            gemm_sel_r <= 1'b0;
    end

    assign be_act_rd_addr = gemm_sel_r ? gemm_act_rd_addr : conv_act_rd_addr;
    assign be_act_rd_req  = gemm_sel_r ? gemm_act_rd_req  : conv_act_rd_req;
    assign be_wt_rd_addr  = gemm_sel_r ? gemm_wt_rd_addr  : conv_wt_rd_addr;
    assign be_wt_rd_req   = gemm_sel_r ? gemm_wt_rd_req   : conv_wt_rd_req;
    assign be_out_wr_addr = gemm_sel_r ? gemm_out_wr_addr  : conv_out_wr_addr;
    assign be_out_wr_data = gemm_sel_r ? gemm_out_wr_data  : conv_out_wr_data;
    assign be_out_wr_req  = gemm_sel_r ? gemm_out_wr_req   : conv_out_wr_req;
    assign be_bias_addr   = gemm_sel_r ? gemm_bias_addr    : conv_bias_addr;
    assign be_bias_req    = gemm_sel_r ? gemm_bias_req     : conv_bias_req;

    assign be_done = gemm_done | conv_done;
    assign be_busy = gemm_busy | conv_busy;

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
    assign backend_stall  = be_busy & ~be_act_rd_gnt;

endmodule : npu_core
