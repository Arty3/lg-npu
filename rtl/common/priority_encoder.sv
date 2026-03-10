// ============================================================================
// priority_encoder.sv - Leading-one priority encoder
// ============================================================================

module priority_encoder #(
    parameter int WIDTH = 8
) (
    input  logic [WIDTH-1:0]           in,
    output logic [$clog2(WIDTH)-1:0]   out,
    output logic                       valid
);

    always_comb begin
        out   = '0;
        valid = 1'b0;
        for (int i = 0; i < WIDTH; ++i) begin
            if (in[i] && !valid) begin
                out   = i[$clog2(WIDTH)-1:0];
                valid = 1'b1;
            end
        end
    end

endmodule : priority_encoder
