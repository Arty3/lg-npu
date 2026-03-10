// ============================================================================
// npu_queue.sv - Command queue FIFO
// ============================================================================

module npu_queue
    import npu_types_pkg::*;
    import npu_cmd_pkg::*;
    import npu_cfg_pkg::*;
(
    input  logic      clk,
    input  logic      rst_n,

    // Push side (from decode)
    input  conv_cmd_t wr_cmd,
    input  logic      wr_valid,
    output logic      wr_ready,

    // Pop side (to scheduler)
    output conv_cmd_t rd_cmd,
    output logic      rd_valid,
    input  logic      rd_ready,

    // Status
    output logic      full,
    output logic      empty
);

    localparam int W = $bits(conv_cmd_t);

    logic [W-1:0] wr_data_flat, rd_data_flat;

    assign wr_data_flat = wr_cmd;
    assign rd_cmd       = conv_cmd_t'(rd_data_flat);

    fifo #(
        .DATA_W (W),
        .DEPTH  (CMD_QUEUE_DEPTH)
    ) u_fifo (
        .clk      (clk),
        .rst_n    (rst_n),
        .wr_data  (wr_data_flat),
        .wr_valid (wr_valid),
        .wr_ready (wr_ready),
        .rd_data  (rd_data_flat),
        .rd_valid (rd_valid),
        .rd_ready (rd_ready),
        .full     (full),
        .empty    (empty)
    );

endmodule : npu_queue
