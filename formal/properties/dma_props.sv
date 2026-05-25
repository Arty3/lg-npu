// ============================================================================
// dma_props.sv - DMA reader/writer coordination assertions
// ============================================================================

module dma_props (
    input logic clk,
    input logic rst_n,
    input logic rd_busy,
    input logic wr_busy,
    input logic rd_ext_req,
    input logic wr_ext_req,
    input logic ext_mem_req,
    input logic ext_mem_wr,
    input logic dma_done
);

`ifdef FORMAL

    // Frontend policy: only one direction active at a time.
    a_single_direction: assert property (
        @(posedge clk) disable iff (!rst_n)
        !(rd_busy && wr_busy)
    );

    // External request must come from active engine only.
    a_req_matches_active_engine: assert property (
        @(posedge clk) disable iff (!rst_n)
        ext_mem_req |-> ((rd_busy && rd_ext_req && !ext_mem_wr) ||
                         (wr_busy && wr_ext_req &&  ext_mem_wr))
    );

    // Completion should only happen when at least one engine was active.
    a_done_requires_activity: assert property (
        @(posedge clk) disable iff (!rst_n)
        dma_done |-> $past(rd_busy || wr_busy)
    );

`elsif SIMULATION

    a_single_direction: assert property (
        @(posedge clk) disable iff (!rst_n)
        !(rd_busy && wr_busy)
    );

`endif

endmodule : dma_props
