// ============================================================================
// arbiter_rr.sv - Round-robin arbiter
// ============================================================================

module arbiter_rr #(
    parameter int NUM_REQ = 2
) (
    input  logic                    clk,
    input  logic                    rst_n,
    input  logic [NUM_REQ-1:0]      req,
    output logic [NUM_REQ-1:0]      grant
);

    logic [$clog2(NUM_REQ)-1:0] last_grant;

    initial begin
        if (NUM_REQ != (1 << $clog2(NUM_REQ)))
            $fatal(1, "arbiter_rr NUM_REQ must be power-of-two, got %0d", NUM_REQ);
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            last_grant <= '0;
        end else if (|grant) begin
            for (int i = 0; i < NUM_REQ; ++i) begin
                if (grant[i])
                    last_grant <= i[$clog2(NUM_REQ)-1:0];
            end
        end
    end

    always_comb begin
        grant = '0;
        // Search from last_grant+1, wrap around
        for (int i = 0; i < NUM_REQ; ++i) begin
            automatic int idx;
            idx = int'(last_grant) + 1 + i;
            if (idx >= NUM_REQ)
                idx = idx - NUM_REQ;
            if (req[idx] && grant == '0)
                grant[idx] = 1'b1;
        end
    end

endmodule : arbiter_rr
