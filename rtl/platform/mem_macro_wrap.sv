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
            /*
             * Sky130 OpenRAM macro mapping for the single-port path.
             * Currently only the PSUM bank uses this wrapper:
             *   DEPTH=4096, DATA_W=32  -> 8 * sky130_sram_2kbyte_1rw1r_32x512_8
             * Other parameter combinations elaborate to an $error so a future
             * caller does not silently fall back to a behavioural model.
             */
            if (DEPTH != 4096 || DATA_W != 32) begin : g_unsupported
`ifdef SIMULATION
                initial $error("mem_macro_wrap: no sky130 mapping for DEPTH=%0d DATA_W=%0d", DEPTH, DATA_W);
`endif
            end

            localparam int BANKS    = 8;
            localparam int ROW_W    = 9;         /* macro is 32x512 */
            localparam int BANK_SEL = $clog2(BANKS);

            logic [BANK_SEL-1:0] bank_sel;
            logic [ROW_W-1:0]    row_addr;
            logic [BANK_SEL-1:0] bank_sel_r;
            logic [31:0]         bank_dout [BANKS];

            assign bank_sel = addr[ROW_W +: BANK_SEL];
            assign row_addr = addr[0       +: ROW_W];

            always_ff @(posedge clk) begin
                if (en)
                    bank_sel_r <= bank_sel;
            end

            genvar gi;
            for (gi = 0; gi < BANKS; ++gi) begin : g_bank
                logic bank_en;
                assign bank_en = en && (bank_sel == BANK_SEL'(gi));

                sky130_sram_2kbyte_1rw1r_32x512_8 u_sky130_sram (
                    .clk0   (clk),
                    .csb0   (~bank_en),
                    .web0   (~we),
                    .wmask0 (4'b1111),
                    .addr0  (row_addr),
                    .din0   (wdata),
                    .dout0  (bank_dout[gi]),
                    .clk1   (clk),
                    .csb1   (1'b1),
                    .addr1  ({ROW_W{1'b0}}),
                    .dout1  ()
                );
            end

            assign rdata = bank_dout[bank_sel_r];
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
