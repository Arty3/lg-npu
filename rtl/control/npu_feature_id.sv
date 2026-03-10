// ============================================================================
// npu_feature_id.sv - Read-only feature identification register
// ============================================================================

module npu_feature_id
    import npu_types_pkg::*;
    import npu_cfg_pkg::*;
(
    output logic [31:0] feature_id
);

    // Bit layout:
    //   [31:24] version major
    //   [23:16] version minor
    //   [15:12] number of backends
    //   [11:8]  array rows
    //   [7:4]   array cols
    //   [3:0]   supported dtypes (bit 0 = INT8)
    assign feature_id = {
        NPU_VERSION_MAJOR,                        // [31:24]
        NPU_VERSION_MINOR,                        // [23:16]
        4'(NUM_BACKENDS),                         // [15:12]
        4'(ARRAY_ROWS),                           // [11:8]
        4'(ARRAY_COLS),                           // [7:4]
        4'b0001                                   // [3:0] INT8 only
    };

endmodule : npu_feature_id
