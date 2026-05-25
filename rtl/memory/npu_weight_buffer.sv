// ============================================================================
// npu_weight_buffer.sv - INT8 weight SRAM bank
// ============================================================================

module npu_weight_buffer
    import npu_types_pkg::*;
    import npu_cfg_pkg::*;
(
    input  logic                        clk,
    input  logic                        rst_n,

    // Port A - host MMIO writes / reads
    input  logic [SRAM_ADDR_W-1:0]      host_addr,
    input  logic [DATA_W-1:0]           host_wdata,
    input  logic                        host_we,
    input  logic                        host_req,
    output logic                        host_gnt,
    output logic [DATA_W-1:0]           host_rdata,
    output logic                        host_rvalid,

    // Port B - conv backend reads
    input  logic [SRAM_ADDR_W-1:0]      be_addr,
    input  logic                        be_req,
    /* verilator lint_off UNOPTFLAT */
    output logic                        be_gnt,
    /* verilator lint_on UNOPTFLAT */
    output logic [DATA_W-1:0]           be_rdata,
    output logic                        be_rvalid
);

    logic [DATA_W-1:0] host_rdata_i;
    logic [DATA_W-1:0] be_rdata_i;

    logic host_rd_r;
    logic be_rd_r;

    // 1RW1R: host/DMA uses RW port, backend uses dedicated read port.
    mem_macro_1rw1r_wrap #(
        .DEPTH  (SRAM_DEPTH),
        .DATA_W (DATA_W)
    ) u_sram (
        .clk     (clk),
        .a_addr  (host_addr),
        .a_wdata (host_wdata),
        .a_we    (host_we),
        .a_en    (host_req),
        .a_rdata (host_rdata_i),
        .b_addr  (be_addr),
        .b_en    (be_req),
        .b_rdata (be_rdata_i)
    );

    assign host_gnt = host_req;
    assign be_gnt   = be_req;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            host_rd_r <= 1'b0;
            be_rd_r   <= 1'b0;
        end else begin
            host_rd_r <= host_req & ~host_we;
            be_rd_r   <= be_req;
        end
    end

    assign host_rdata  = host_rdata_i;
    assign host_rvalid = host_rd_r;
    assign be_rdata    = be_rdata_i;
    assign be_rvalid   = be_rd_r;

endmodule : npu_weight_buffer
