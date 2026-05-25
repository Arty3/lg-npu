// ============================================================================
// handshake_props.sv - Generic valid/ready protocol assertions
// ============================================================================

module handshake_props #(
    parameter int DATA_W = 32
) (
    input logic               clk,
    input logic               rst_n,
    input logic               valid,
    input logic               ready,
    input logic [DATA_W-1:0]  data
);

`ifdef FORMAL

    // Source must keep valid asserted until transfer.
    a_valid_stable_until_ready: assert property (
        @(posedge clk) disable iff (!rst_n)
        valid && !ready |=> valid
    );

    // Payload must remain stable while stalled.
    a_data_stable_while_stalled: assert property (
        @(posedge clk) disable iff (!rst_n)
        valid && !ready |=> $stable(data)
    );

`elsif SIMULATION

    a_valid_stable_until_ready: assert property (
        @(posedge clk) disable iff (!rst_n)
        valid && !ready |=> valid
    );

`endif

endmodule : handshake_props

