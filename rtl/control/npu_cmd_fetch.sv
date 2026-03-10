// ============================================================================
// npu_cmd_fetch.sv - Reads a command descriptor from the command queue window
//   When the host writes the doorbell register, this module reads the
//   descriptor words from the command-queue MMIO window and forwards the
//   raw data to npu_cmd_decode.
// ============================================================================

module npu_cmd_fetch
    import npu_types_pkg::*;
    import npu_addrmap_pkg::*;
(
    input  logic                    clk,
    input  logic                    rst_n,

    // Doorbell trigger (pulse from reg block)
    input  logic                    doorbell,

    // MMIO read port into the command queue window
    // (accesses the same MMIO bus, but the reg block routes these)
    output logic [MMIO_ADDR_W-1:0]  cmd_rd_addr,
    output logic                    cmd_rd_req,
    input  logic [MMIO_DATA_W-1:0]  cmd_rd_data,
    input  logic                    cmd_rd_valid,

    // Raw descriptor out (16 x 32-bit words)
    output logic [MMIO_DATA_W-1:0]  desc_word,
    output logic [3:0]              desc_word_idx,
    output logic                    desc_word_valid,
    output logic                    desc_done,      // All words transferred

    output logic                    busy
);

    localparam int NUM_WORDS = CMD_ENTRY_BYTES / (MMIO_DATA_W / 8); // 16

    typedef enum logic [1:0]
	{
        S_IDLE,
        S_READ,
        S_WAIT
    }   state_e;

    state_e     state, state_next;
    logic [3:0] word_cnt;

    assign busy = (state != S_IDLE);

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            state <= S_IDLE;
        else
            state <= state_next;
    end

    always_comb begin
        state_next = state;
        case (state)
            S_IDLE: if (doorbell)      state_next = S_READ;
            S_READ:                    state_next = S_WAIT;
            S_WAIT: begin
                if (cmd_rd_valid) begin
                    if (word_cnt == 4'(NUM_WORDS - 1))
                        state_next = S_IDLE;
                    else
                        state_next = S_READ;
                end
            end
            default: state_next = S_IDLE;
        endcase
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            word_cnt <= '0;
        end else if (state == S_IDLE && doorbell) begin
            word_cnt <= '0;
        end else if (state == S_WAIT && cmd_rd_valid) begin
            word_cnt <= word_cnt + 1;
        end
    end

    assign cmd_rd_addr     = CMD_QUEUE_BASE + MMIO_ADDR_W'({word_cnt, 2'b00});
    assign cmd_rd_req      = (state == S_READ);

    assign desc_word       = cmd_rd_data;
    assign desc_word_idx   = word_cnt;
    assign desc_word_valid = (state == S_WAIT) & cmd_rd_valid;
    assign desc_done       = desc_word_valid & (word_cnt == 4'(NUM_WORDS - 1));

endmodule : npu_cmd_fetch
