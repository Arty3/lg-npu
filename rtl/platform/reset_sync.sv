// ============================================================================
// reset_sync.sv - Asynchronous-assert, synchronous-deassert reset synchronizer
// ============================================================================

module reset_sync #(
    parameter int STAGES = 2
) (
    input  logic clk,
    input  logic rst_async_n,
    output logic rst_sync_n
);

    logic [STAGES-1:0] sync_chain;

    always_ff @(posedge clk or negedge rst_async_n) begin
        if (!rst_async_n)
            sync_chain <= '0;
        else
            sync_chain <= {sync_chain[STAGES-2:0], 1'b1};
    end

    assign rst_sync_n = sync_chain[STAGES-1];

endmodule : reset_sync
