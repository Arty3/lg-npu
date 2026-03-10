// ============================================================================
// fifo_props.sv - Generic FIFO assertions (bindable to fifo.sv)
// ============================================================================

module fifo_props #(
    parameter int DATA_W = 32,
    parameter int DEPTH  = 4,
    parameter int PTR_W  = $clog2(DEPTH)
) (
    input logic                 clk,
    input logic                 rst_n,
    input logic [PTR_W:0]       wr_ptr,
    input logic [PTR_W:0]       rd_ptr,
    input logic                 full,
    input logic                 empty,
    input logic                 wr_valid,
    input logic                 wr_ready,
    input logic                 rd_valid,
    input logic                 rd_ready,
    input logic [DATA_W-1:0]    wr_data,
    input logic [DATA_W-1:0]    rd_data
);

    logic [PTR_W:0] occupancy;
    assign occupancy = wr_ptr - rd_ptr;

    // Occupancy bounded
    a_occ_bounded: assert property (
        @(posedge clk) disable iff (!rst_n)
        occupancy <= DEPTH
    );

    // wr_ready iff not full
    a_wr_ready: assert property (
        @(posedge clk) disable iff (!rst_n)
        wr_ready == !full
    );

    // rd_valid iff not empty
    a_rd_valid: assert property (
        @(posedge clk) disable iff (!rst_n)
        rd_valid == !empty
    );

    // A write increments occupancy (when no simultaneous read)
    a_write_inc: assert property (
        @(posedge clk) disable iff (!rst_n)
        (wr_valid && wr_ready && !(rd_valid && rd_ready))
        |=> occupancy == $past(occupancy) + 1
    );

    // A read decrements occupancy (when no simultaneous write)
    a_read_dec: assert property (
        @(posedge clk) disable iff (!rst_n)
        (rd_valid && rd_ready && !(wr_valid && wr_ready))
        |=> occupancy == $past(occupancy) - 1
    );

    // Simultaneous read+write keeps occupancy stable
    a_rw_stable: assert property (
        @(posedge clk) disable iff (!rst_n)
        (wr_valid && wr_ready && rd_valid && rd_ready)
        |=> occupancy == $past(occupancy)
    );

    // Reset zeros pointers
    a_reset: assert property (
        @(posedge clk)
        !rst_n |=> (wr_ptr == '0) && (rd_ptr == '0)
    );

endmodule : fifo_props
