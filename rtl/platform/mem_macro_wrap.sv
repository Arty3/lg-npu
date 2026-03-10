// ============================================================================
// mem_macro_wrap.sv - Technology-portable single-port SRAM wrapper
//   Simulation: infers a simple register array.
//   FPGA/ASIC:  replace body with macro instantiation.
// ============================================================================

module mem_macro_wrap #(
    parameter int DEPTH  = 4096,
    parameter int DATA_W = 8
) (
    input  logic                        clk,
    input  logic [$clog2(DEPTH)-1:0]    addr,
    input  logic [DATA_W-1:0]           wdata,
    input  logic                         we,
    input  logic                         en,
    output logic [DATA_W-1:0]           rdata
);

    logic [DATA_W-1:0] mem [DEPTH];

    always_ff @(posedge clk) begin
        if (en) begin
            if (we)
                mem[addr] <= wdata;
            else
                rdata <= mem[addr];
        end
    end

endmodule : mem_macro_wrap
