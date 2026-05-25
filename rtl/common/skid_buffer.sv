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

    logic [DATA_W-1:0] out_data_r, skid_data_r;
    logic              out_valid_r, skid_valid_r;

    logic pop_out;
    logic push_in;
    logic bypass_to_out;
    logic refill_from_skid;
    logic spill_to_skid;

    assign out_data  = out_data_r;
    assign out_valid = out_valid_r;
    assign in_ready  = ~skid_valid_r;

    assign pop_out          = out_valid_r & out_ready;
    assign push_in          = in_valid & in_ready;
    assign refill_from_skid = (pop_out || ~out_valid_r) && skid_valid_r;
    assign bypass_to_out    = (pop_out || ~out_valid_r) && ~skid_valid_r && push_in;
    assign spill_to_skid    = out_valid_r && ~out_ready && push_in;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            out_data_r  <= '0;
            skid_data_r <= '0;
            out_valid_r <= 1'b0;
            skid_valid_r<= 1'b0;
        end else begin
            if (refill_from_skid) begin
                out_data_r   <= skid_data_r;
                out_valid_r  <= 1'b1;
                skid_valid_r <= 1'b0;
            end else if (bypass_to_out) begin
                out_data_r  <= in_data;
                out_valid_r <= 1'b1;
            end else if (pop_out) begin
                out_valid_r <= 1'b0;
            end

            if (spill_to_skid) begin
                skid_data_r  <= in_data;
                skid_valid_r <= 1'b1;
            end
        end
    end

endmodule : skid_buffer
