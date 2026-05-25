// ============================================================================
// mem_req_rsp_props.sv - Memory request/response pairing assertions
// ============================================================================

module mem_req_rsp_props (
    input logic clk,
    input logic rst_n,
    input logic req,
    input logic gnt,
    input logic rvalid
);

    logic outstanding_r;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            outstanding_r <= 1'b0;
        else begin
            if (req && gnt)
                outstanding_r <= 1'b1;
            if (rvalid)
                outstanding_r <= 1'b0;
        end
    end

`ifdef FORMAL

    a_rsp_requires_outstanding: assert property (
        @(posedge clk) disable iff (!rst_n)
        rvalid |-> outstanding_r
    );

`elsif SIMULATION

    a_rsp_requires_outstanding: assert property (
        @(posedge clk) disable iff (!rst_n)
        rvalid |-> outstanding_r
    );

`endif

endmodule : mem_req_rsp_props
