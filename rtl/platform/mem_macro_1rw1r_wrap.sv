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
            /*
             * Sky130 OpenRAM mapping for the 1RW1R path, built from the only
             * pre-built 32-bit OpenRAM macro in efabless/sky130_sram_macros:
             *   sky130_sram_2kbyte_1rw1r_32x512_8  (32 x 512, 4-bit byte mask)
             *
             * Each macro stores 4 lanes x 512 rows = 2048 bytes; larger logical
             * buffers tile multiple macros across the bank axis:
             *   DEPTH=4096, DATA_W=8 -> 2 * sky130_sram_2kbyte_1rw1r_32x512_8
             *   DEPTH=8192, DATA_W=8 -> 4 * sky130_sram_2kbyte_1rw1r_32x512_8
             * The byte mask gates per-lane writes inside the selected bank.
             */
            if (DATA_W != 8 || (DEPTH != 4096 && DEPTH != 8192)) begin : g_unsupported
`ifdef SIMULATION
                initial $error("mem_macro_1rw1r_wrap: no sky130 mapping for DEPTH=%0d DATA_W=%0d", DEPTH, DATA_W);
`endif
            end

            localparam int LANE_W   = 2;             /* 4 lanes per 32-bit row */
            localparam int ROW_W    = 9;             /* macro is 32x512        */
            localparam int BANKS    = (DEPTH == 8192) ? 4 : 2;
            localparam int BANK_SEL = $clog2(BANKS);

            logic [BANK_SEL-1:0] a_bank;
            logic [ROW_W-1:0]    a_row;
            logic [LANE_W-1:0]   a_lane;
            logic [BANK_SEL-1:0] a_bank_r;
            logic [LANE_W-1:0]   a_lane_r;
            logic [3:0]          a_wmask;
            logic [31:0]         a_din_packed;
            logic [31:0]         a_dout_packed [BANKS];

            logic [BANK_SEL-1:0] b_bank;
            logic [ROW_W-1:0]    b_row;
            logic [LANE_W-1:0]   b_lane;
            logic [BANK_SEL-1:0] b_bank_r;
            logic [LANE_W-1:0]   b_lane_r;
            logic [31:0]         b_dout_packed [BANKS];

            assign a_lane = a_addr[0                  +: LANE_W];
            assign a_row  = a_addr[LANE_W             +: ROW_W];
            assign a_bank = a_addr[LANE_W + ROW_W     +: BANK_SEL];
            assign b_lane = b_addr[0                  +: LANE_W];
            assign b_row  = b_addr[LANE_W             +: ROW_W];
            assign b_bank = b_addr[LANE_W + ROW_W     +: BANK_SEL];

            /* Replicate byte across all four lanes; wmask gates the write. */
            assign a_din_packed = {4{a_wdata}};
            always_comb begin
                a_wmask = 4'b0000;
                a_wmask[a_lane] = a_we;
            end

            always_ff @(posedge clk) begin
                if (a_en) begin
                    a_lane_r <= a_lane;
                    a_bank_r <= a_bank;
                end
                if (b_en) begin
                    b_lane_r <= b_lane;
                    b_bank_r <= b_bank;
                end
            end

            genvar gi;
            for (gi = 0; gi < BANKS; ++gi) begin : g_bank
                logic a_bank_en, b_bank_en;
                assign a_bank_en = a_en && (a_bank == BANK_SEL'(gi));
                assign b_bank_en = b_en && (b_bank == BANK_SEL'(gi));

                sky130_sram_2kbyte_1rw1r_32x512_8 u_sky130_sram (
                    .clk0   (clk),
                    .csb0   (~a_bank_en),
                    .web0   (~a_we),
                    .wmask0 (a_wmask),
                    .addr0  (a_row),
                    .din0   (a_din_packed),
                    .dout0  (a_dout_packed[gi]),
                    .clk1   (clk),
                    .csb1   (~b_bank_en),
                    .addr1  (b_row),
                    .dout1  (b_dout_packed[gi])
                );
            end

            assign a_rdata = a_dout_packed[a_bank_r][a_lane_r*8 +: 8];
            assign b_rdata = b_dout_packed[b_bank_r][b_lane_r*8 +: 8];
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
