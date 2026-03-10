// ============================================================================
// skid_buffer.sv - Skid buffer for registered-ready handshake decoupling
// ============================================================================

module skid_buffer #(
    parameter int DATA_W = 32
) (
    input  logic                clk,
    input  logic                rst_n,

    // Upstream
    input  logic [DATA_W-1:0]   in_data,
    input  logic                in_valid,
    output logic                in_ready,

    // Downstream
    output logic [DATA_W-1:0]   out_data,
    output logic                out_valid,
    input  logic                out_ready
);

    logic [DATA_W-1:0]  buf_data;
    logic               buf_valid;

    assign in_ready  = ~buf_valid;
    assign out_valid = in_valid | buf_valid;
    assign out_data  = buf_valid ? buf_data : in_data;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            buf_valid <= 1'b0;
            buf_data  <= '0;
        end else begin
            if (in_valid && in_ready && out_valid && !out_ready) begin
                buf_data  <= in_data;
                buf_valid <= 1'b1;
            end else if (buf_valid && out_ready) begin
                buf_valid <= 1'b0;
            end
        end
    end

endmodule : skid_buffer
