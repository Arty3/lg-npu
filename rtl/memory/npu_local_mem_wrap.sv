// ============================================================================
// npu_local_mem_wrap.sv - Single-port SRAM bank with req/gnt interface
//   Wraps mem_macro_wrap and adds a single-cycle req/gnt + rvalid protocol.
// ============================================================================

module npu_local_mem_wrap #(
    parameter int DEPTH  = 4096,
    parameter int DATA_W = 8
) (
    input  logic                        clk,
    input  logic                        rst_n,

    input  logic [$clog2(DEPTH)-1:0]    addr,
    input  logic [DATA_W-1:0]           wdata,
    input  logic                        we,
    input  logic                        req,
    output logic                        gnt,
    output logic [DATA_W-1:0]           rdata,
    output logic                        rvalid
);

    // Grant immediately when requested (single-port, single-cycle)
    assign gnt = req;

    mem_macro_wrap #(
        .DEPTH  (DEPTH),
        .DATA_W (DATA_W)
    ) u_sram (
        .clk   (clk),
        .addr  (addr),
        .wdata (wdata),
        .we    (we),
        .en    (req),
        .rdata (rdata)
    );

    // Read data is valid one cycle after a read request
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            rvalid <= 1'b0;
        else
            rvalid <= req & ~we;
    end

endmodule : npu_local_mem_wrap
