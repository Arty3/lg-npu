// ============================================================================
// onehot_mux.sv - One-hot encoded multiplexer
// ============================================================================

module onehot_mux #(
    parameter int DATA_W  = 32,
    parameter int NUM_IN  = 2
) (
    input  logic [NUM_IN-1:0]               sel,
    input  logic [NUM_IN-1:0][DATA_W-1:0]   data_in,
    output logic [DATA_W-1:0]               data_out
);

    always_comb begin
        data_out = '0;
        for (int i = 0; i < NUM_IN; ++i) begin
            if (sel[i])
                data_out = data_in[i];
        end
    end

endmodule : onehot_mux
