// ============================================================================
// mem_macro_wrap.sv - Technology-portable single-port SRAM wrapper
//   Simulation: infers a simple register array.
//   FPGA/ASIC:  replace body with macro instantiation.
// ============================================================================

module mem_macro_wrap #(
    parameter int DEPTH    = 4096,
    parameter int DATA_W   = 8,
    parameter int MEM_IMPL = 0
) (
    input  logic                        clk,
    input  logic [$clog2(DEPTH)-1:0]    addr,
    input  logic [DATA_W-1:0]           wdata,
    input  logic                        we,
    input  logic                        en,
    output logic [DATA_W-1:0]           rdata
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
                if (en) begin
                    if (we)
                        mem[addr] <= wdata;
                    else
                        rdata <= mem[addr];
                end
            end
        end else if (MEM_IMPL == MEM_IMPL_FPGA) begin : g_fpga
            // Generic behavioral model for FPGA builds; replace with vendor BRAM
            // primitive instantiation in platform-specific forks as needed.
            always_ff @(posedge clk) begin
                if (en) begin
                    if (we)
                        mem[addr] <= wdata;
                    else
                        rdata <= mem[addr];
                end
            end
        end else begin : g_sky130
`ifdef ASIC_BUILD
            sky130_sram_1rw_blackbox #(
                .DEPTH  (DEPTH),
                .DATA_W (DATA_W)
            ) u_sky130_sram (
                .clk    (clk),
                .cs     (en),
                .we     (we),
                .addr   (addr),
                .din    (wdata),
                .dout   (rdata)
            );
`else
            always_ff @(posedge clk) begin
                if (en) begin
                    if (we)
                        mem[addr] <= wdata;
                    else
                        rdata <= mem[addr];
                end
            end
`endif
        end
    endgenerate

endmodule : mem_macro_wrap
