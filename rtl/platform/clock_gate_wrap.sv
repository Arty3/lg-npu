// ============================================================================
// clock_gate_wrap.sv - Technology-portable clock gate
//   Simulation: simple AND gate (latch-free for sim convenience).
//   ASIC:       replace with ICG cell.
// ============================================================================

module clock_gate_wrap (
    input  logic clk,
    input  logic en,
    output logic gated_clk
);

    // Latch-based clock gate (safe for ASIC; benign for simulation)
    logic en_latched;

    always_latch begin
        if (!clk)
            en_latched = en;
    end

    assign gated_clk = clk & en_latched;

endmodule : clock_gate_wrap
