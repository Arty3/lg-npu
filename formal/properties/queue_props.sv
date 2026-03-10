// ============================================================================
// queue_props.sv - Queue / FIFO contract assertions
// ============================================================================

module queue_props #(
    parameter int DEPTH = 4,
    parameter int PTR_W = $clog2(DEPTH)
) (
    input logic             clk,
    input logic             rst_n,
    input logic [PTR_W:0]   wr_ptr,
    input logic [PTR_W:0]   rd_ptr,
    input logic             full,
    input logic             empty,
    input logic             wr_valid,
    input logic             wr_ready,
    input logic             rd_valid,
    input logic             rd_ready
);

    // Occupancy derived from pointers
    logic [PTR_W:0] occupancy;
    assign occupancy = wr_ptr - rd_ptr;

    // Occupancy never exceeds depth
    a_occ_le_depth: assert property (
        @(posedge clk) disable iff (!rst_n)
        occupancy <= DEPTH
    );

    // Full flag is correct
    a_full_iff_depth: assert property (
        @(posedge clk) disable iff (!rst_n)
        full == (occupancy == DEPTH)
    );

    // Empty flag is correct
    a_empty_iff_zero: assert property (
        @(posedge clk) disable iff (!rst_n)
        empty == (occupancy == 0)
    );

    // wr_ready deasserted when full
    a_no_wr_ready_when_full: assert property (
        @(posedge clk) disable iff (!rst_n)
        full |-> !wr_ready
    );

    // rd_valid deasserted when empty
    a_no_rd_valid_when_empty: assert property (
        @(posedge clk) disable iff (!rst_n)
        empty |-> !rd_valid
    );

    // No write occurs when full (backpressure honoured)
    a_no_write_when_full: assert property (
        @(posedge clk) disable iff (!rst_n)
        full |-> !(wr_valid & wr_ready)
    );

    // No read occurs when empty
    a_no_read_when_empty: assert property (
        @(posedge clk) disable iff (!rst_n)
        empty |-> !(rd_valid & rd_ready)
    );

    // After reset, pointers are zero
    a_reset_ptrs: assert property (
        @(posedge clk)
        !rst_n |=> (wr_ptr == '0) && (rd_ptr == '0)
    );

endmodule : queue_props
