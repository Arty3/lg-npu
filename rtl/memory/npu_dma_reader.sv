// npu_dma_reader.sv - DMA read engine (external memory -> local SRAM)

module npu_dma_reader
    import npu_types_pkg::*;
(
    input  logic                    clk,
    input  logic                    rst_n,

    // Control
    input  logic                    start,
    input  logic [EXT_ADDR_W-1:0]   ext_addr,
    input  logic [MMIO_ADDR_W-1:0]  loc_addr,
    input  logic [15:0]             len,
    output logic                    busy,
    output logic                    done,

    // External memory read port
    output logic [EXT_ADDR_W-1:0]   ext_mem_addr,
    output logic                    ext_mem_req,
    input  logic                    ext_mem_gnt,
    input  logic [MMIO_DATA_W-1:0]  ext_mem_rdata,
    input  logic                    ext_mem_rvalid,

    // Local buffer write port
    output logic [MMIO_ADDR_W-1:0]  buf_addr,
    output logic [MMIO_DATA_W-1:0]  buf_wdata,
    output logic                    buf_wr,
    output logic                    buf_req,
    input  logic                    buf_gnt
);

    typedef enum logic [2:0]
    {
        S_IDLE,
        S_EXT_RD,
        S_EXT_WAIT,
        S_LOC_WR,
        S_DONE
    }   state_e;

    state_e                 state_r, state_next;
    logic [EXT_ADDR_W-1:0]  ext_addr_r;
    logic [MMIO_ADDR_W-1:0] loc_addr_r;
    logic [15:0]            len_r;
    logic [15:0]            cnt_r;
    logic [MMIO_DATA_W-1:0] data_r;

    always_ff @(posedge clk or negedge rst_n)
    begin
        if (!rst_n)
        begin
            state_r    <= S_IDLE;
            ext_addr_r <= '0;
            loc_addr_r <= '0;
            len_r      <= '0;
            cnt_r      <= '0;
            data_r     <= '0;
        end
        else
        begin
            state_r <= state_next;
            case (state_r)
                S_IDLE:
                begin
                    if (start)
                    begin
                        ext_addr_r <= ext_addr;
                        loc_addr_r <= loc_addr;
                        len_r      <= len;
                        cnt_r      <= '0;
                    end
                end
                S_EXT_RD:
                begin
                    // Wait for grant; nothing to latch here
                end
                S_EXT_WAIT:
                begin
                    if (ext_mem_rvalid)
                        data_r <= ext_mem_rdata;
                end
                S_LOC_WR:
                begin
                    if (buf_gnt)
                    begin
                        cnt_r      <= cnt_r + 16'd1;
                        ext_addr_r <= ext_addr_r + 32'd1;
                        loc_addr_r <= loc_addr_r + 20'd1;
                    end
                end
                default: ;
            endcase
        end
    end

    always_comb
    begin
        state_next   = state_r;
        ext_mem_addr = '0;
        ext_mem_req  = 1'b0;
        buf_addr     = '0;
        buf_wdata    = '0;
        buf_wr       = 1'b0;
        buf_req      = 1'b0;
        busy         = 1'b0;
        done         = 1'b0;

        case (state_r)
            S_IDLE:
            begin
                if (start)
                    state_next = S_EXT_RD;
            end
            S_EXT_RD:
            begin
                busy         = 1'b1;
                ext_mem_addr = ext_addr_r;
                ext_mem_req  = 1'b1;
                if (ext_mem_gnt)
                    state_next = S_EXT_WAIT;
            end
            S_EXT_WAIT:
            begin
                busy = 1'b1;
                if (ext_mem_rvalid)
                    state_next = S_LOC_WR;
            end
            S_LOC_WR:
            begin
                busy      = 1'b1;
                buf_addr  = loc_addr_r;
                buf_wdata = data_r;
                buf_wr    = 1'b1;
                buf_req   = 1'b1;
                if (buf_gnt)
                begin
                    if (cnt_r + 16'd1 >= len_r)
                        state_next = S_DONE;
                    else
                        state_next = S_EXT_RD;
                end
            end
            S_DONE:
            begin
                done       = 1'b1;
                state_next = S_IDLE;
            end
            default: state_next = S_IDLE;
        endcase
    end

endmodule : npu_dma_reader
