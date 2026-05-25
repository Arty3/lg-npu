// ============================================================================
// npu_act_buffer.sv - INT8 activation SRAM bank (input + output share space)
// ============================================================================

module npu_act_buffer
    import npu_types_pkg::*;
    import npu_cfg_pkg::*;
(
    input  logic                        clk,
    input  logic                        rst_n,

    // Port A - host MMIO
    input  logic [SRAM_ADDR_W:0]        host_addr,
    input  logic [DATA_W-1:0]           host_wdata,
    input  logic                        host_we,
    input  logic                        host_req,
    output logic                        host_gnt,
    output logic [DATA_W-1:0]           host_rdata,
    output logic                        host_rvalid,

    // Port B - conv backend read (input activations)
    input  logic [SRAM_ADDR_W:0]        be_rd_addr,
    input  logic                        be_rd_req,
    /* verilator lint_off UNOPTFLAT */
    output logic                        be_rd_gnt,
    /* verilator lint_on UNOPTFLAT */
    output logic [DATA_W-1:0]           be_rd_rdata,
    output logic                        be_rd_rvalid,

    // Port C - conv backend write (output activations)
    input  logic [SRAM_ADDR_W:0]        be_wr_addr,
    input  logic [DATA_W-1:0]           be_wr_data,
    input  logic                        be_wr_req,
    output logic                        be_wr_gnt
);

    // 1RW1R organization:
    //   Port A (RW): backend write has priority, else host/DMA RW.
    //   Port B (R):  dedicated backend read path.
    logic [SRAM_ADDR_W:0] a_addr;
    logic [DATA_W-1:0]    a_wdata;
    logic                 a_we, a_en;
    logic [DATA_W-1:0]    a_rdata;

    logic [DATA_W-1:0]    b_rdata;

    logic host_rd_accept_r;
    logic be_rd_accept_r;

    assign a_addr  = be_wr_req ? be_wr_addr : host_addr;
    assign a_wdata = be_wr_req ? be_wr_data : host_wdata;
    assign a_we    = be_wr_req ? 1'b1       : (host_req & host_we & ~be_wr_req);
    assign a_en    = be_wr_req | host_req;

    mem_macro_1rw1r_wrap #(
        .DEPTH  (SRAM_DEPTH * 2),  // double-size for in + out
        .DATA_W (DATA_W)
    ) u_sram (
        .clk     (clk),
        .a_addr  (a_addr),
        .a_wdata (a_wdata),
        .a_we    (a_we),
        .a_en    (a_en),
        .a_rdata (a_rdata),
        .b_addr  (be_rd_addr),
        .b_en    (be_rd_req),
        .b_rdata (b_rdata)
    );

    assign be_wr_gnt = be_wr_req;
    assign be_rd_gnt = be_rd_req;
    assign host_gnt  = host_req & ~be_wr_req;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            host_rd_accept_r <= 1'b0;
            be_rd_accept_r   <= 1'b0;
        end else begin
            host_rd_accept_r <= host_req & ~host_we & ~be_wr_req;
            be_rd_accept_r   <= be_rd_req;
        end
    end

    assign host_rdata   = a_rdata;
    assign host_rvalid  = host_rd_accept_r;
    assign be_rd_rdata  = b_rdata;
    assign be_rd_rvalid = be_rd_accept_r;

endmodule : npu_act_buffer
