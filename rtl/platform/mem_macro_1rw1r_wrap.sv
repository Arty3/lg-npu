// ============================================================================
// mem_macro_1rw1r_wrap.sv - Technology-portable 1RW1R SRAM wrapper
//   Port A: read/write
//   Port B: read-only
// ============================================================================

module mem_macro_1rw1r_wrap #(
    parameter int DEPTH  = 4096,
    parameter int DATA_W = 8,
    parameter int MEM_IMPL = 0
) (
    input  logic                        clk,

    // Port A (1RW)
    input  logic [$clog2(DEPTH)-1:0]    a_addr,
    input  logic [DATA_W-1:0]           a_wdata,
    input  logic                        a_we,
    input  logic                        a_en,
    output logic [DATA_W-1:0]           a_rdata,

    // Port B (1R)
    input  logic [$clog2(DEPTH)-1:0]    b_addr,
    input  logic                        b_en,
    output logic [DATA_W-1:0]           b_rdata
);

    localparam int MEM_IMPL_FLOPS  = 0;
    localparam int MEM_IMPL_FPGA   = 1;

    logic [DATA_W-1:0] mem [DEPTH];

    generate
        if (MEM_IMPL == MEM_IMPL_FLOPS) begin : g_flops
`ifdef SIMULATION
            initial begin
                for (int i = 0; i < DEPTH; ++i)
                    mem[i] = '0;
            end
`endif

            always_ff @(posedge clk) begin
                if (a_en) begin
                    if (a_we)
                        mem[a_addr] <= a_wdata;
                    else
                        a_rdata <= mem[a_addr];
                end
                if (b_en)
                    b_rdata <= mem[b_addr];
            end
        end else if (MEM_IMPL == MEM_IMPL_FPGA) begin : g_fpga
            always_ff @(posedge clk) begin
                if (a_en) begin
                    if (a_we)
                        mem[a_addr] <= a_wdata;
                    else
                        a_rdata <= mem[a_addr];
                end
                if (b_en)
                    b_rdata <= mem[b_addr];
            end
        end else begin : g_sky130
`ifdef ASIC_BUILD
            sky130_sram_1rw1r_blackbox #(
                .DEPTH  (DEPTH),
                .DATA_W (DATA_W)
            ) u_sky130_sram (
                .clk    (clk),
                .a_cs   (a_en),
                .a_we   (a_we),
                .a_addr (a_addr),
                .a_din  (a_wdata),
                .a_dout (a_rdata),
                .b_cs   (b_en),
                .b_addr (b_addr),
                .b_dout (b_rdata)
            );
`else
            always_ff @(posedge clk) begin
                if (a_en) begin
                    if (a_we)
                        mem[a_addr] <= a_wdata;
                    else
                        a_rdata <= mem[a_addr];
                end
                if (b_en)
                    b_rdata <= mem[b_addr];
            end
`endif
        end
    endgenerate

endmodule : mem_macro_1rw1r_wrap
