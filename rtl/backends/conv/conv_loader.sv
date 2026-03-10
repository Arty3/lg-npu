// ============================================================================
// conv_loader.sv - Issues read requests for weight and activation data
// ============================================================================

module conv_loader
    import npu_types_pkg::*;
(
    input  logic                       clk,
    input  logic                       rst_n,

    // Address / control from conv_ctrl via conv_addr_gen
    input  logic [ADDR_W-1:0]          act_addr,
    input  logic [ADDR_W-1:0]          wt_addr,
    input  logic                       zero_pad,
    input  logic                       load_valid,
    output logic                       load_ready,

    // Memory ports (activation buffer)
    output logic [ADDR_W-1:0]          act_mem_addr,
    output logic                       act_mem_req,
    input  logic                       act_mem_gnt,
    input  logic [DATA_W-1:0]          act_mem_rdata,
    input  logic                       act_mem_rvalid,

    // Memory ports (weight buffer)
    output logic [ADDR_W-1:0]          wt_mem_addr,
    output logic                       wt_mem_req,
    input  logic                       wt_mem_gnt,
    input  logic [DATA_W-1:0]          wt_mem_rdata,
    input  logic                       wt_mem_rvalid,

    // Downstream pair to PE array
    output logic signed [DATA_W-1:0]   act_out,
    output logic signed [DATA_W-1:0]   wt_out,
    output logic                       pair_valid,
    input  logic                       pair_ready
);

    // FSM states
    typedef enum logic [1:0]
	{
        S_IDLE,
        S_REQ,
        S_WAIT,
        S_DONE
    }   state_e;

    state_e state, state_next;

    logic                       act_latched, wt_latched;
    logic signed [DATA_W-1:0]   act_r, wt_r;
    logic                       is_zero_pad_r;

    assign load_ready   = (state == S_IDLE);
    // Latch addresses at acceptance time (S_IDLE -> S_REQ) because
    // conv_ctrl advances its loop counters on the same cycle as iter_accept,
    // which changes addr_gen outputs before S_REQ issues memory requests.
    logic [ADDR_W-1:0] act_addr_r, wt_addr_r;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            act_addr_r <= '0;
            wt_addr_r  <= '0;
        end else if (state == S_IDLE && load_valid) begin
            act_addr_r <= act_addr;
            wt_addr_r  <= wt_addr;
        end
    end

    assign act_mem_addr = act_addr_r;
    assign wt_mem_addr  = wt_addr_r;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            state <= S_IDLE;
        else
            state <= state_next;
    end

    always_comb begin
        state_next  = state;
        act_mem_req = 1'b0;
        wt_mem_req  = 1'b0;

        case (state)
            S_IDLE: begin
                if (load_valid)
                    state_next = S_REQ;
            end
            S_REQ: begin
                if (is_zero_pad_r) begin
                    // No memory read needed, proceed directly
                    state_next = S_DONE;
                end else begin
                    act_mem_req = ~act_latched;
                    wt_mem_req  = ~wt_latched;
                    if ((act_latched || act_mem_gnt) &&
                        (wt_latched  || wt_mem_gnt))
                        state_next = S_WAIT;
                end
            end
            S_WAIT: begin
                if (act_latched && wt_latched)
                    state_next = S_DONE;
            end
            S_DONE: begin
                if (pair_ready)
                    state_next = S_IDLE;
            end
        endcase
    end

    // Latch read data as it arrives
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            act_r         <= '0;
            wt_r          <= '0;
            act_latched   <= 1'b0;
            wt_latched    <= 1'b0;
            is_zero_pad_r <= 1'b0;
        end else begin
            case (state)
                S_IDLE: begin
                    if (load_valid) begin
                        act_latched   <= 1'b0;
                        wt_latched    <= 1'b0;
                        is_zero_pad_r <= zero_pad;
                        if (zero_pad) begin
                            act_r       <= '0;
                            wt_r        <= '0;
                            act_latched <= 1'b1;
                            wt_latched  <= 1'b1;
                        end
                    end
                end
                S_REQ, S_WAIT: begin
                    if (act_mem_rvalid && !act_latched) begin
                        act_r       <= act_mem_rdata;
                        act_latched <= 1'b1;
                    end
                    if (wt_mem_rvalid && !wt_latched) begin
                        wt_r        <= wt_mem_rdata;
                        wt_latched  <= 1'b1;
                    end
                end
                S_DONE: begin
                    // held until pair_ready
                end
            endcase
        end
    end

    assign act_out    = act_r;
    assign wt_out     = wt_r;
    assign pair_valid = (state == S_DONE);

endmodule : conv_loader
