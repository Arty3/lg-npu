// ============================================================================
// sky130_sram_blackboxes.sv - Placeholder Sky130 SRAM macro declarations
//   Replace these wrappers with exact sky130_sram_* macro instances chosen
//   by memory compiler output and macro naming in your flow.
// ============================================================================

/* verilator lint_off DECLFILENAME */

module sky130_sram_1rw_blackbox #(
    parameter int DEPTH  = 4096,
    parameter int DATA_W = 8,
    parameter int ADDR_W = $clog2(DEPTH)
) (
    input  logic               clk,
    input  logic               cs,
    input  logic               we,
    input  logic [ADDR_W-1:0]  addr,
    input  logic [DATA_W-1:0]  din,
    output logic [DATA_W-1:0]  dout
);

endmodule : sky130_sram_1rw_blackbox

module sky130_sram_1rw1r_blackbox #(
    parameter int DEPTH = 4096,
    parameter int DATA_W = 8,
    parameter int ADDR_W = $clog2(DEPTH)
) (
    input  logic               clk,

    // Port A: 1RW
    input  logic               a_cs,
    input  logic               a_we,
    input  logic [ADDR_W-1:0]  a_addr,
    input  logic [DATA_W-1:0]  a_din,
    output logic [DATA_W-1:0]  a_dout,

    // Port B: 1R
    input  logic               b_cs,
    input  logic [ADDR_W-1:0]  b_addr,
    output logic [DATA_W-1:0]  b_dout
);

endmodule : sky130_sram_1rw1r_blackbox

/* verilator lint_on DECLFILENAME */
