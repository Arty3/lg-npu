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

    // Priority: backend write > backend read > host
    logic                      sel_be_wr, sel_be_rd, sel_host;
    logic [SRAM_ADDR_W:0]      mux_addr;
    logic [DATA_W-1:0]         mux_wdata;
    logic                      mux_we, mux_req;
    logic [DATA_W-1:0]         mem_rdata;
    logic                      mem_rvalid, mem_gnt;

    assign sel_be_wr = be_wr_req;
    assign sel_be_rd = ~be_wr_req & be_rd_req;
    assign sel_host  = ~be_wr_req & ~be_rd_req;

    assign mux_addr  = sel_be_wr ? be_wr_addr :
                       sel_be_rd ? be_rd_addr : host_addr;
    assign mux_wdata = sel_be_wr ? be_wr_data : host_wdata;
    assign mux_we    = sel_be_wr ? 1'b1       : (sel_host & host_we);
    assign mux_req   = be_wr_req | be_rd_req  | host_req;

    assign be_wr_gnt = sel_be_wr & mem_gnt;
    assign be_rd_gnt = sel_be_rd & mem_gnt;
    assign host_gnt  = sel_host  & mem_gnt;

    npu_local_mem_wrap #(
        .DEPTH  (SRAM_DEPTH * 2),  // double-size for in + out
        .DATA_W (DATA_W)
    ) u_sram (
        .clk    (clk),
        .rst_n  (rst_n),
        .addr   (mux_addr),
        .wdata  (mux_wdata),
        .we     (mux_we),
        .req    (mux_req),
        .gnt    (mem_gnt),
        .rdata  (mem_rdata),
        .rvalid (mem_rvalid)
    );

    // Track who was reading
    logic [1:0] read_sel_r;  // 0=host, 1=be_rd
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) read_sel_r <= '0;
        else        read_sel_r <= {sel_be_rd & mux_req, sel_host & mux_req & ~host_we};
    end

    assign host_rdata   = mem_rdata;
    assign host_rvalid  = mem_rvalid & read_sel_r[0];
    assign be_rd_rdata  = mem_rdata;
    assign be_rd_rvalid = mem_rvalid & read_sel_r[1];

endmodule : npu_act_buffer
