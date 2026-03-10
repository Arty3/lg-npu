// ============================================================================
// npu_cfg_pkg.sv - Design-time configuration parameters
// ============================================================================

package npu_cfg_pkg;

    import npu_types_pkg::*;

    // PE array geometry (rows x cols)
    localparam int ARRAY_ROWS = 1;
    localparam int ARRAY_COLS = 1;

    // Command queue
    localparam int CMD_QUEUE_DEPTH = 4;

    // Local SRAM bank sizes (in bytes)
    //   Each bank is SRAM_DEPTH x data-width bytes.
    localparam int SRAM_DEPTH  = 4096;  // entries per bank
    localparam int SRAM_ADDR_W = $clog2(SRAM_DEPTH);

    /* verilator lint_off UNUSEDPARAM */
    localparam int WEIGHT_BUF_BYTES = SRAM_DEPTH * 1;  //  4 KB (INT8)
    localparam int ACT_BUF_BYTES    = SRAM_DEPTH * 2;  //  8 KB (INT8, in+out)
    localparam int PSUM_BUF_BYTES   = SRAM_DEPTH * 4;  // 16 KB (INT32)
    /* verilator lint_on UNUSEDPARAM */

    // Feature-ID constants
    localparam logic [7:0] NPU_VERSION_MAJOR = 8'd0;
    localparam logic [7:0] NPU_VERSION_MINOR = 8'd1;
    localparam int         NUM_BACKENDS      = 1;
    localparam logic [3:0] DTYPES_SUPPORTED  = 4'b0001; // bit 0 = INT8

endpackage : npu_cfg_pkg
