// ============================================================================
// counter.sv - Generic up-counter with clear and enable
// ============================================================================

module counter #(
    parameter int WIDTH = 32
) (
    input  logic             clk,
    input  logic             rst_n,
    input  logic             clr,
    input  logic             en,
    output logic [WIDTH-1:0] count
);

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            count <= '0;
        else if (clr)
            count <= '0;
        else if (en)
            count <= count + 1'b1;
    end

endmodule : counter
