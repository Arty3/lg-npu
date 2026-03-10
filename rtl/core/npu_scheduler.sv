// ============================================================================
// npu_scheduler.sv - In-order command scheduler (single command at a time)
// ============================================================================

module npu_scheduler
    import npu_types_pkg::*;
    import npu_cmd_pkg::*;
(
    input  logic      clk,
    input  logic      rst_n,

    // From command queue
    input  conv_cmd_t queue_cmd,
    input  logic      queue_valid,
    output logic      queue_ready,

    // To dispatch
    output conv_cmd_t dispatch_cmd,
    output logic      dispatch_valid,
    input  logic      dispatch_ready,

    // Feedback: backend done
    input  logic      backend_done
);

    typedef enum logic [1:0]
	{
        S_IDLE,
        S_ISSUE,
        S_WAIT
    }   state_e;

    state_e    state, state_next;
    conv_cmd_t cmd_r;

    assign queue_ready    = (state == S_IDLE);
    assign dispatch_cmd   = cmd_r;
    assign dispatch_valid = (state == S_ISSUE);

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            state <= S_IDLE;
        else
            state <= state_next;
    end

    always_comb begin
        state_next = state;
        case (state)
            S_IDLE:  if (queue_valid)    state_next = S_ISSUE;
            S_ISSUE: if (dispatch_ready) state_next = S_WAIT;
            S_WAIT:  if (backend_done)   state_next = S_IDLE;
            default:                     state_next = S_IDLE;
        endcase
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            cmd_r <= '0;
        end else if (state == S_IDLE && queue_valid) begin
            cmd_r <= queue_cmd;
        end
    end

endmodule : npu_scheduler
