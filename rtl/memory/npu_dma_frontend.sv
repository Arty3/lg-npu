// npu_dma_frontend.sv - DMA control front-end
//   Orchestrates npu_dma_reader (ext->local) and npu_dma_writer (local->ext).
//   Direction bit: 0 = read (ext->local), 1 = write (local->ext).

module npu_dma_frontend
    import npu_types_pkg::*;
(
    input  logic                    clk,
    input  logic                    rst_n,

    // Configuration from register block
    input  logic [EXT_ADDR_W-1:0]   dma_ext_addr,
    input  logic [MMIO_ADDR_W-1:0]  dma_loc_addr,
    input  logic [15:0]             dma_len,
    input  logic                    dma_start,
    input  logic                    dma_dir,
    output logic                    dma_busy,
    output logic                    dma_done,

    // External memory port (directly wired to shell)
    output logic [EXT_ADDR_W-1:0]   ext_mem_addr,
    output logic [MMIO_DATA_W-1:0]  ext_mem_wdata,
    output logic                    ext_mem_wr,
    output logic                    ext_mem_req,
    input  logic                    ext_mem_gnt,
    input  logic [MMIO_DATA_W-1:0]  ext_mem_rdata,
    input  logic                    ext_mem_rvalid,

    // Local buffer port (muxed onto host path in shell)
    output logic [MMIO_ADDR_W-1:0]  buf_addr,
    output logic [MMIO_DATA_W-1:0]  buf_wdata,
    output logic                    buf_wr,
    output logic                    buf_req,
    input  logic                    buf_gnt,
    input  logic [MMIO_DATA_W-1:0]  buf_rdata,
    input  logic                    buf_rvalid
);

    // Reader signals (ext -> local)
    logic                    rd_start, rd_busy, rd_done;
    logic [EXT_ADDR_W-1:0]   rd_ext_addr;
    logic                    rd_ext_req;
    logic [MMIO_ADDR_W-1:0]  rd_buf_addr;
    logic [MMIO_DATA_W-1:0]  rd_buf_wdata;
    logic                    rd_buf_wr, rd_buf_req;

    // Writer signals (local -> ext)
    logic                    wr_start, wr_busy, wr_done;
    logic [EXT_ADDR_W-1:0]   wr_ext_addr;
    logic [MMIO_DATA_W-1:0]  wr_ext_wdata;
    logic                    wr_ext_wr, wr_ext_req;
    logic [MMIO_ADDR_W-1:0]  wr_buf_addr;
    logic                    wr_buf_req;

    assign rd_start = dma_start & ~dma_dir;
    assign wr_start = dma_start &  dma_dir;

    npu_dma_reader u_reader (
        .clk           (clk),
        .rst_n         (rst_n),
        .start         (rd_start),
        .ext_addr      (dma_ext_addr),
        .loc_addr      (dma_loc_addr),
        .len           (dma_len),
        .busy          (rd_busy),
        .done          (rd_done),
        .ext_mem_addr  (rd_ext_addr),
        .ext_mem_req   (rd_ext_req),
        .ext_mem_gnt   (ext_mem_gnt),
        .ext_mem_rdata (ext_mem_rdata),
        .ext_mem_rvalid(ext_mem_rvalid),
        .buf_addr      (rd_buf_addr),
        .buf_wdata     (rd_buf_wdata),
        .buf_wr        (rd_buf_wr),
        .buf_req       (rd_buf_req),
        .buf_gnt       (buf_gnt)
    );

    npu_dma_writer u_writer (
        .clk           (clk),
        .rst_n         (rst_n),
        .start         (wr_start),
        .ext_addr      (dma_ext_addr),
        .loc_addr      (dma_loc_addr),
        .len           (dma_len),
        .busy          (wr_busy),
        .done          (wr_done),
        .ext_mem_addr  (wr_ext_addr),
        .ext_mem_wdata (wr_ext_wdata),
        .ext_mem_wr    (wr_ext_wr),
        .ext_mem_req   (wr_ext_req),
        .ext_mem_gnt   (ext_mem_gnt),
        .buf_addr      (wr_buf_addr),
        .buf_req       (wr_buf_req),
        .buf_gnt       (buf_gnt),
        .buf_rdata     (buf_rdata),
        .buf_rvalid    (buf_rvalid)
    );

    // Mux outputs: only one engine active at a time
    assign dma_busy = rd_busy | wr_busy;
    assign dma_done = rd_done | wr_done;

    // External memory port mux
    assign ext_mem_addr  = rd_busy ? rd_ext_addr  : wr_ext_addr;
    assign ext_mem_wdata = wr_ext_wdata;
    assign ext_mem_wr    = wr_ext_wr;
    assign ext_mem_req   = rd_ext_req | wr_ext_req;

    // Local buffer port mux
    assign buf_addr  = rd_busy ? rd_buf_addr  : wr_buf_addr;
    assign buf_wdata = rd_buf_wdata;
    assign buf_wr    = rd_buf_wr;
    assign buf_req   = rd_buf_req | wr_buf_req;

endmodule : npu_dma_frontend
