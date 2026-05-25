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

`ifdef ASIC_BUILD
    // Map to a recognized Sky130 integrated clock-gating cell for CTS/power.
    sky130_fd_sc_hd__dlclkp_1 u_icg (
        .CLK (clk),
        .GATE(en),
        .GCLK(gated_clk)
    );
`else
    // Functional fallback for simulation/lint.
    logic en_latched;

    always_latch begin
        if (!clk)
            en_latched = en;
    end

    assign gated_clk = clk & en_latched;
`endif

endmodule : clock_gate_wrap
