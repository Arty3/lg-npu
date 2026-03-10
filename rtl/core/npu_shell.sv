// ============================================================================
// npu_shell.sv - Top-level NPU wrapper
//   Instantiated by any platform (sim, FPGA, ASIC).
// ============================================================================

module npu_shell
    import npu_types_pkg::*;
    import npu_cmd_pkg::*;
    import npu_cfg_pkg::*;
    import npu_addrmap_pkg::*;
    import npu_perf_pkg::*;
(
    input  logic                   clk,
    input  logic                   rst_async_n,

    // Host MMIO bus
    input  logic [MMIO_ADDR_W-1:0] mmio_addr,
    input  logic [MMIO_DATA_W-1:0] mmio_wdata,
    input  logic                   mmio_wr,
    input  logic                   mmio_valid,
    output logic                   mmio_ready,
    output logic [MMIO_DATA_W-1:0] mmio_rdata,

    // External memory port (DMA)
    output logic [EXT_ADDR_W-1:0]  ext_mem_addr,
    output logic [MMIO_DATA_W-1:0] ext_mem_wdata,
    output logic                   ext_mem_wr,
    output logic                   ext_mem_req,
    input  logic                   ext_mem_gnt,
    input  logic [MMIO_DATA_W-1:0] ext_mem_rdata,
    input  logic                   ext_mem_rvalid,

    // Interrupt output
    output logic                   irq
);

    // Reset
    logic rst_n, soft_reset;

    npu_reset_ctrl u_reset (
        .clk         (clk),
        .rst_async_n (rst_async_n),
        .soft_reset  (soft_reset),
        .rst_n       (rst_n)
    );

    // Feature ID
    logic [31:0] feature_id;

    npu_feature_id u_fid (
        .feature_id (feature_id)
    );

    // Register block
    logic                   doorbell;
    /* verilator lint_off UNUSEDSIGNAL */
    logic                   npu_enable;
    /* verilator lint_on UNUSEDSIGNAL */
    logic                   irq_status_w, irq_enable_w, irq_clear_w;
    logic                   core_idle, core_busy, queue_full_w;

    logic [MMIO_ADDR_W-1:0] buf_addr;
    logic [MMIO_DATA_W-1:0] buf_wdata;
    logic                   buf_wr, buf_req, buf_gnt;
    logic [MMIO_DATA_W-1:0] buf_rdata;
    logic                   buf_rvalid;

    // Register block -> buffer window (host MMIO path, before DMA mux)
    logic [MMIO_ADDR_W-1:0] reg_buf_addr;
    logic [MMIO_DATA_W-1:0] reg_buf_wdata;
    logic                   reg_buf_wr, reg_buf_req;

    logic [MMIO_ADDR_W-1:0] fetch_addr;
    logic                   fetch_req;
    logic [MMIO_DATA_W-1:0] fetch_rdata;
    logic                   fetch_rvalid;

    logic [PERF_CNT_W-1:0]  perf_cycles, perf_active, perf_stall;

    // DMA configuration wires
    logic [EXT_ADDR_W-1:0]  dma_ext_addr_w;
    logic [MMIO_ADDR_W-1:0] dma_loc_addr_w;
    logic [15:0]            dma_len_w;
    logic                   dma_start_w, dma_dir_w;
    logic                   dma_busy_w, dma_done_w;

    npu_reg_block u_regs (
        .clk          (clk),
        .rst_n        (rst_n),
        .mmio_addr    (mmio_addr),
        .mmio_wdata   (mmio_wdata),
        .mmio_wr      (mmio_wr),
        .mmio_valid   (mmio_valid),
        .mmio_ready   (mmio_ready),
        .mmio_rdata   (mmio_rdata),
        .soft_reset   (soft_reset),
        .npu_enable   (npu_enable),
        .doorbell     (doorbell),
        .irq_status   (irq_status_w),
        .irq_enable   (irq_enable_w),
        .irq_clear    (irq_clear_w),
        .feature_id   (feature_id),
        .perf_cycles  (perf_cycles),
        .perf_active  (perf_active),
        .perf_stall   (perf_stall),
        .core_idle    (core_idle),
        .core_busy    (core_busy),
        .queue_full   (queue_full_w),
        .buf_addr     (reg_buf_addr),
        .buf_wdata    (reg_buf_wdata),
        .buf_wr       (reg_buf_wr),
        .buf_req      (reg_buf_req),
        .buf_gnt      (buf_gnt),
        .buf_rdata    (buf_rdata),
        .buf_rvalid   (buf_rvalid),
        .fetch_addr   (fetch_addr),
        .fetch_req    (fetch_req),
        .fetch_rdata  (fetch_rdata),
        .fetch_rvalid (fetch_rvalid),
        .dma_ext_addr (dma_ext_addr_w),
        .dma_loc_addr (dma_loc_addr_w),
        .dma_len      (dma_len_w),
        .dma_start    (dma_start_w),
        .dma_dir      (dma_dir_w),
        .dma_busy     (dma_busy_w)
    );

    // DMA frontend
    logic [MMIO_ADDR_W-1:0] dma_buf_addr;
    logic [MMIO_DATA_W-1:0] dma_buf_wdata;
    logic                   dma_buf_wr, dma_buf_req;

    npu_dma_frontend u_dma (
        .clk            (clk),
        .rst_n          (rst_n),
        .dma_ext_addr   (dma_ext_addr_w),
        .dma_loc_addr   (dma_loc_addr_w),
        .dma_len        (dma_len_w),
        .dma_start      (dma_start_w),
        .dma_dir        (dma_dir_w),
        .dma_busy       (dma_busy_w),
        .dma_done       (dma_done_w),
        .ext_mem_addr   (ext_mem_addr),
        .ext_mem_wdata  (ext_mem_wdata),
        .ext_mem_wr     (ext_mem_wr),
        .ext_mem_req    (ext_mem_req),
        .ext_mem_gnt    (ext_mem_gnt),
        .ext_mem_rdata  (ext_mem_rdata),
        .ext_mem_rvalid (ext_mem_rvalid),
        .buf_addr       (dma_buf_addr),
        .buf_wdata      (dma_buf_wdata),
        .buf_wr         (dma_buf_wr),
        .buf_req        (dma_buf_req),
        .buf_gnt        (buf_gnt),
        .buf_rdata      (buf_rdata),
        .buf_rvalid     (buf_rvalid)
    );

    // Mux buffer port: DMA takes over when busy, otherwise host MMIO
    assign buf_addr  = dma_busy_w ? dma_buf_addr  : reg_buf_addr;
    assign buf_wdata = dma_busy_w ? dma_buf_wdata : reg_buf_wdata;
    assign buf_wr    = dma_busy_w ? dma_buf_wr    : reg_buf_wr;
    assign buf_req   = dma_busy_w ? dma_buf_req   : reg_buf_req;

    // Command fetch + decode + queue
    logic [MMIO_DATA_W-1:0]  desc_word;
    logic [3:0]              desc_word_idx;
    logic                    desc_word_valid, desc_done;
    logic                    fetch_busy;

    npu_cmd_fetch u_fetch (
        .clk            (clk),
        .rst_n          (rst_n),
        .doorbell       (doorbell),
        .cmd_rd_addr    (fetch_addr),
        .cmd_rd_req     (fetch_req),
        .cmd_rd_data    (fetch_rdata),
        .cmd_rd_valid   (fetch_rvalid),
        .desc_word      (desc_word),
        .desc_word_idx  (desc_word_idx),
        .desc_word_valid(desc_word_valid),
        .desc_done      (desc_done),
        .busy           (fetch_busy)
    );

    conv_cmd_t decode_cmd;
    logic      decode_valid, decode_ready, decode_err, decode_busy;

    npu_cmd_decode u_decode (
        .clk             (clk),
        .rst_n           (rst_n),
        .desc_word       (desc_word),
        .desc_word_idx   (desc_word_idx),
        .desc_word_valid (desc_word_valid),
        .desc_done       (desc_done),
        .cmd_out         (decode_cmd),
        .cmd_valid       (decode_valid),
        .cmd_ready       (decode_ready),
        .decode_err      (decode_err),
        .busy            (decode_busy)
    );

    conv_cmd_t queue_cmd;
    logic      queue_valid, queue_ready;
    logic      queue_full,  queue_empty;

    npu_queue u_queue (
        .clk      (clk),
        .rst_n    (rst_n),
        .wr_cmd   (decode_cmd),
        .wr_valid (decode_valid),
        .wr_ready (decode_ready),
        .rd_cmd   (queue_cmd),
        .rd_valid (queue_valid),
        .rd_ready (queue_ready),
        .full     (queue_full),
        .empty    (queue_empty)
    );

    // Core (scheduler + dispatch + backend + memory + completion)
    logic cmd_done_pulse;
    logic be_busy, be_active, be_stall;

    npu_core u_core (
        .clk             (clk),
        .rst_n           (rst_n),
        .queue_cmd       (queue_cmd),
        .queue_valid     (queue_valid),
        .queue_ready     (queue_ready),
        .buf_addr        (buf_addr),
        .buf_wdata       (buf_wdata),
        .buf_wr          (buf_wr),
        .buf_req         (buf_req),
        .buf_gnt         (buf_gnt),
        .buf_rdata       (buf_rdata),
        .buf_rvalid      (buf_rvalid),
        .cmd_done        (cmd_done_pulse),
        .backend_busy    (be_busy),
        .backend_active  (be_active),
        .backend_stall   (be_stall)
    );

    // Status aggregation
    npu_status u_status (
        .backend_busy  (be_busy | dma_busy_w),
        .queue_empty   (queue_empty),
        .queue_full    (queue_full),
        .cmd_pipe_busy (fetch_busy | decode_busy),
        .idle          (core_idle),
        .busy          (core_busy),
        .q_full        (queue_full_w)
    );

    // IRQ controller (DMA done OR'd with cmd_done)
    npu_irq_ctrl u_irq (
        .clk        (clk),
        .rst_n      (rst_n),
        .cmd_done   (cmd_done_pulse | dma_done_w),
        .err        (decode_err),
        .irq_enable (irq_enable_w),
        .irq_clear  (irq_clear_w),
        .irq_status (irq_status_w),
        .irq_out    (irq)
    );

    // Performance counters
    npu_perf_counters u_perf (
        .clk             (clk),
        .rst_n           (rst_n),
        .backend_active  (be_active),
        .backend_stall   (be_stall),
        .clear           (soft_reset),
        .cnt_cycles      (perf_cycles),
        .cnt_active      (perf_active),
        .cnt_stall       (perf_stall)
    );

endmodule : npu_shell
