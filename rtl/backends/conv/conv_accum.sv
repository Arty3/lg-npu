// ============================================================================
// conv_accum.sv - Accumulator for partial sums
//   Holds one INT32 accumulator per active output element.
//   For the single-PE configuration this is a single register.
// ============================================================================

module conv_accum
    import npu_types_pkg::*;
(
    input  logic                      clk,
    input  logic                      rst_n,

    // Control
    input  logic                      clr,          // Clear accumulator (new output pixel)
    input  logic                      acc_en,       // Accumulate enable

    // Data from PE array
    input  logic signed [ACC_W-1:0]   mac_result,

    // Accumulated value out
    output logic signed [ACC_W-1:0]   acc_out,
    output logic signed [ACC_W-1:0]   acc_feedback  // Fed back to PE as acc_in
);

    logic signed [ACC_W-1:0] acc_r;

    assign acc_out      = acc_r;
    assign acc_feedback = clr ? '0 : acc_r;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            acc_r <= '0;
        end else if (clr) begin
            acc_r <= '0;
        end else if (acc_en) begin
            acc_r <= mac_result;
        end
    end

endmodule : conv_accum
