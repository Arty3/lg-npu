// ============================================================================
// sky130_sram_blackboxes.sv - Real Sky130 OpenRAM SRAM macro declarations
//
// These blackbox declarations match the OpenRAM-generated module signatures
// shipped with the open `sky130_sram_macros` collection:
//
//     https://github.com/efabless/sky130_sram_macros
//
// Bring-up flow:
//   1. Clone sky130_sram_macros (or copy the .v / .lef / .lib / .gds files)
//      into asic/openlane2/macros/.
//   2. Wire EXTRA_VERILOG_MODELS / EXTRA_LEFS / EXTRA_LIBS / EXTRA_GDS_FILES
//      in asic/openlane2/config.json to those paths.
//   3. For LVS sign-off, define USE_POWER_PINS at the synthesis step and
//      plumb vccd1 / vssd1 from the top of the chip down through the
//      wrappers (currently TODO; the macro accepts the pins under the
//      same ifdef so the synth tool will report them as missing).
//
// Larger logical buffers are built by tiling this single 2 KB macro inside
// `mem_macro_wrap` / `mem_macro_1rw1r_wrap`.
// ============================================================================

/* verilator lint_off DECLFILENAME */
/* verilator lint_off UNUSEDSIGNAL */

// 2 KB 1RW + 1R, 32-bit wide, 512 rows, 4-bit byte mask.
//   Total bits = 512 * 32 = 16384  (2 KiB).
// This is the only pre-built OpenRAM macro in efabless/sky130_sram_macros that
// matches the 32-bit datapath used by every on-chip buffer; bigger buffers are
// composed by tiling N copies in the wrappers above.
module sky130_sram_2kbyte_1rw1r_32x512_8 (
`ifdef USE_POWER_PINS
    inout         vccd1,
    inout         vssd1,
`endif
    // Port 0 : 1RW
    input         clk0,
    input         csb0,    // active-low chip select
    input         web0,    // active-low write enable
    input  [3:0]  wmask0,  // byte write mask (active-high per byte)
    input  [8:0]  addr0,
    input  [31:0] din0,
    output [31:0] dout0,

    // Port 1 : 1R
    input         clk1,
    input         csb1,    // active-low chip select
    input  [8:0]  addr1,
    output [31:0] dout1
);
    // Empty body - real implementation provided by the macro's GDS/LEF/LIB
    // during sign-off and by a behavioural .v during functional sim.
endmodule : sky130_sram_2kbyte_1rw1r_32x512_8

/* verilator lint_on UNUSEDSIGNAL */
/* verilator lint_on DECLFILENAME */
