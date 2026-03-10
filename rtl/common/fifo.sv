// ============================================================================
// fifo.sv - Synchronous FIFO with valid/ready handshake
// ============================================================================

module fifo #(
    parameter int DATA_W = 32,
    parameter int DEPTH  = 4
) (
    input  logic                clk,
    input  logic                rst_n,

    // Write (push) side
    input  logic [DATA_W-1:0]   wr_data,
    input  logic                wr_valid,
    output logic                wr_ready,

    // Read (pop) side
    output logic [DATA_W-1:0]   rd_data,
    output logic                rd_valid,
    input  logic                rd_ready,

    // Status
    output logic                full,
    output logic                empty
);

    localparam int PTR_W = $clog2(DEPTH);

    logic [DATA_W-1:0]  mem [DEPTH];
    logic [PTR_W:0]     wr_ptr, rd_ptr;
    logic               do_wr, do_rd;

    assign full     = (wr_ptr[PTR_W] != rd_ptr[PTR_W]) &&
                      (wr_ptr[PTR_W-1:0] == rd_ptr[PTR_W-1:0]);
    assign empty    = (wr_ptr == rd_ptr);
    assign wr_ready = ~full;
    assign rd_valid = ~empty;
    assign rd_data  = mem[rd_ptr[PTR_W-1:0]];

    assign do_wr = wr_valid & wr_ready;
    assign do_rd = rd_valid & rd_ready;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            wr_ptr <= '0;
            rd_ptr <= '0;
        end else begin
            if (do_wr) begin
                mem[wr_ptr[PTR_W-1:0]] <= wr_data;
                wr_ptr <= wr_ptr + 1'b1;
            end
            if (do_rd)
                rd_ptr <= rd_ptr + 1'b1;
        end
    end

endmodule : fifo
