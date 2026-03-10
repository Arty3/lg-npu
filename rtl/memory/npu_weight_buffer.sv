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

    // Simple arbitration: backend has priority, host waits
    logic                       sel_host;
    logic [SRAM_ADDR_W-1:0]     mux_addr;
    logic [DATA_W-1:0]          mux_wdata;
    logic                       mux_we, mux_req;
    logic [DATA_W-1:0]          mem_rdata;
    logic                       mem_rvalid;
    logic                       mem_gnt;

    assign sel_host  = ~be_req;
    assign mux_addr  = sel_host ? host_addr  : be_addr;
    assign mux_wdata = host_wdata;
    assign mux_we    = sel_host & host_we;
    assign mux_req   = sel_host ? host_req : be_req;

    assign host_gnt  = sel_host & mem_gnt;
    assign be_gnt    = ~sel_host & mem_gnt;

    npu_local_mem_wrap #(
        .DEPTH  (SRAM_DEPTH),
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

    // Route read data back to the requester that was active
    logic host_was_sel;
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) host_was_sel <= 1'b0;
        else        host_was_sel <= sel_host & mux_req;
    end

    assign host_rdata  = mem_rdata;
    assign host_rvalid = mem_rvalid & host_was_sel;
    assign be_rdata    = mem_rdata;
    assign be_rvalid   = mem_rvalid & ~host_was_sel;

endmodule : npu_weight_buffer
