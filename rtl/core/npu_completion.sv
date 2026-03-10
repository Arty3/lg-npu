// ============================================================================
// npu_completion.sv - Tracks backend done and signals IRQ controller
// ============================================================================

module npu_completion (
    // From backend
    input  logic backend_done,   // Pulse

    // To IRQ controller
    output logic cmd_done        // Pulse
);

    // In the current scope (single in-order command) this is a simple wire.
    assign cmd_done = backend_done;

endmodule : npu_completion
