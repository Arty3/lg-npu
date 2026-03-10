// ============================================================================
// npu_reg_block.sv - MMIO register file and buffer window decoder
// ============================================================================

module npu_reg_block
    import npu_types_pkg::*;
    import npu_addrmap_pkg::*;
    import npu_perf_pkg::*;
(
    input  logic                    clk,
    input  logic                    rst_n,

    // MMIO bus (from shell)
    input  logic [MMIO_ADDR_W-1:0]  mmio_addr,
    input  logic [MMIO_DATA_W-1:0]  mmio_wdata,
    input  logic                    mmio_wr,
    input  logic                    mmio_valid,
    output logic                    mmio_ready,
    output logic [MMIO_DATA_W-1:0]  mmio_rdata,

    // Control outputs
    output logic                    soft_reset,
    output logic                    npu_enable,
    output logic                    doorbell,      // Pulse on doorbell write

    // IRQ interface
    input  logic                    irq_status,
    output logic                    irq_enable,
    output logic                    irq_clear,     // Pulse

    // Feature ID
    input  logic [31:0]             feature_id,

    // Perf counters
    input  logic [PERF_CNT_W-1:0]   perf_cycles,
    input  logic [PERF_CNT_W-1:0]   perf_active,
    input  logic [PERF_CNT_W-1:0]   perf_stall,

    // Status from core
    input  logic                    core_idle,
    input  logic                    core_busy,
    input  logic                    queue_full,

    // Buffer window passthrough
    output logic [MMIO_ADDR_W-1:0]  buf_addr,
    output logic [MMIO_DATA_W-1:0]  buf_wdata,
    output logic                    buf_wr,
    output logic                    buf_req,
    input  logic                    buf_gnt,
    input  logic [MMIO_DATA_W-1:0]  buf_rdata,
    /* verilator lint_off UNUSEDSIGNAL */
    input  logic                    buf_rvalid,
    /* verilator lint_on UNUSEDSIGNAL */

    // Command queue window read (for cmd_fetch)
    input  logic [MMIO_ADDR_W-1:0]  fetch_addr,
    input  logic                    fetch_req,
    output logic [MMIO_DATA_W-1:0]  fetch_rdata,
    output logic                    fetch_rvalid
);

    // Address decode
    logic is_reg, is_buf, is_cmd_queue;

    assign is_reg       = (mmio_addr < CMD_QUEUE_BASE);
    assign is_cmd_queue = (mmio_addr >= CMD_QUEUE_BASE) && (mmio_addr < WEIGHT_BUF_BASE);
    assign is_buf       = (mmio_addr >= WEIGHT_BUF_BASE);

    // Control / status registers
    logic [MMIO_DATA_W-1:0] ctrl_reg;
    logic [MMIO_DATA_W-1:0] irq_en_reg;

    assign soft_reset = ctrl_reg[CTRL_SOFT_RESET_BIT];
    assign npu_enable = ctrl_reg[CTRL_ENABLE_BIT];
    assign irq_enable = irq_en_reg[0];

    // Doorbell and IRQ clear are write-pulses
    logic doorbell_r, irq_clear_r;
    assign doorbell  = doorbell_r;
    assign irq_clear = irq_clear_r;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            ctrl_reg    <= '0;
            irq_en_reg  <= '0;
            doorbell_r  <= 1'b0;
            irq_clear_r <= 1'b0;
        end else begin
            doorbell_r  <= 1'b0;
            irq_clear_r <= 1'b0;

            if (mmio_valid && mmio_wr && is_reg) begin
                case (mmio_addr)
                    REG_CTRL:       ctrl_reg    <= mmio_wdata;
                    REG_DOORBELL:   doorbell_r  <= 1'b1;
                    REG_IRQ_ENABLE: irq_en_reg  <= mmio_wdata;
                    REG_IRQ_CLEAR:  irq_clear_r <= 1'b1;
                    default: ;
                endcase
            end
        end
    end

    // Command queue window (host writes, fetch reads)
    logic [MMIO_DATA_W-1:0] cmd_mem [16];

    // Host writes into command queue window
    always_ff @(posedge clk) begin
        if (mmio_valid && mmio_wr && is_cmd_queue) begin
            cmd_mem[4'((mmio_addr - CMD_QUEUE_BASE) >> 2)] <= mmio_wdata;
        end
    end

    // Fetch reads from command queue window
    logic                   fetch_rvalid_r;
    logic [MMIO_DATA_W-1:0] fetch_rdata_r;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            fetch_rvalid_r <= 1'b0;
            fetch_rdata_r  <= '0;
        end else begin
            fetch_rvalid_r <= fetch_req;
            if (fetch_req)
                fetch_rdata_r <= cmd_mem[4'((fetch_addr - CMD_QUEUE_BASE) >> 2)];
        end
    end

    assign fetch_rdata  = fetch_rdata_r;
    assign fetch_rvalid = fetch_rvalid_r;

    // Register read mux
    logic [MMIO_DATA_W-1:0] reg_rdata;

    always_comb begin
        reg_rdata = '0;
        case (mmio_addr)
            REG_CTRL: reg_rdata = ctrl_reg;
            REG_STATUS: begin
                reg_rdata = '0;
                reg_rdata[STATUS_IDLE_BIT]   = core_idle;
                reg_rdata[STATUS_BUSY_BIT]   = core_busy;
                reg_rdata[STATUS_QUEUE_FULL] = queue_full;
            end
            REG_DOORBELL:    reg_rdata = '0;
            REG_IRQ_STATUS:  reg_rdata = {31'b0, irq_status};
            REG_IRQ_ENABLE:  reg_rdata = irq_en_reg;
            REG_IRQ_CLEAR:   reg_rdata = '0;
            REG_FEATURE_ID:  reg_rdata = feature_id;
            REG_PERF_CYCLES: reg_rdata = perf_cycles;
            REG_PERF_ACTIVE: reg_rdata = perf_active;
            REG_PERF_STALL:  reg_rdata = perf_stall;
            default:         reg_rdata = '0;
        endcase
    end

    // Buffer window passthrough
    assign buf_addr  = mmio_addr;
    assign buf_wdata = mmio_wdata;
    assign buf_wr    = mmio_wr;
    assign buf_req   = mmio_valid & is_buf;

    // MMIO response mux
    logic is_buf_r;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            is_buf_r <= 1'b0;
        else if (mmio_valid)
            is_buf_r <= is_buf;
    end

    assign mmio_ready = is_buf ? buf_gnt : 1'b1;
    assign mmio_rdata = is_buf_r ? buf_rdata : reg_rdata;

endmodule : npu_reg_block
