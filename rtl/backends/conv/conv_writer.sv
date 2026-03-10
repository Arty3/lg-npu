// ============================================================================
// conv_writer.sv - Writes INT8 output back to activation buffer
// ============================================================================

module conv_writer
    import npu_types_pkg::*;
(
    input  logic                       clk,
    input  logic                       rst_n,

    // Output address from ctrl
    input  logic [ADDR_W-1:0]          out_addr,
    input  logic                       addr_valid,

    // Stream in (INT8 result from postproc)
    input  logic signed [DATA_W-1:0]   in_data,
    input  logic                       in_valid,
    output logic                       in_ready,

    // Memory write port (activation buffer)
    output logic [ADDR_W-1:0]          mem_addr,
    output logic [DATA_W-1:0]          mem_wdata,
    output logic                       mem_wr,
    output logic                       mem_req,
    input  logic                       mem_gnt,

    // Done pulse (one write completed)
    output logic                       write_done
);

    typedef enum logic [1:0]
	{
        S_IDLE,
        S_WRITE,
        S_ACK
    }   state_e;

    state_e state, state_next;

    logic [ADDR_W-1:0] addr_r;
    logic [DATA_W-1:0] data_r;

    assign in_ready   = (state == S_IDLE);
    assign mem_addr   = addr_r;
    assign mem_wdata  = data_r;
    assign mem_wr     = 1'b1;
    assign mem_req    = (state == S_WRITE);
    assign write_done = (state == S_ACK);

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            state <= S_IDLE;
        else
            state <= state_next;
    end

    always_comb begin
        state_next = state;
        case (state)
            S_IDLE: begin
                if (in_valid && addr_valid)
                    state_next = S_WRITE;
            end
            S_WRITE: begin
                if (mem_gnt)
                    state_next = S_ACK;
            end
            S_ACK: begin
                state_next = S_IDLE;
            end
            default: state_next = S_IDLE;
        endcase
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            addr_r <= '0;
            data_r <= '0;
        end else if (state == S_IDLE && in_valid && addr_valid) begin
            addr_r <= out_addr;
            data_r <= in_data;
        end
    end

endmodule : conv_writer
