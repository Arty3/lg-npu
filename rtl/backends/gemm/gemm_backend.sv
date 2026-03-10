// ============================================================================
// gemm_backend.sv - Top-level GEMM backend
//   Connects gemm_ctrl -> gemm_addr_gen -> loader -> PE array -> accum
//   -> postproc -> writer.  Reuses conv_loader, conv_array, conv_accum,
//   conv_postproc, and conv_writer as generic building blocks.
// ============================================================================

module gemm_backend
    import npu_types_pkg::*;
    import npu_cmd_pkg::*;
(
    input  logic                clk,
    input  logic                rst_n,

    // Command from dispatch
    input  conv_cmd_t           cmd,
    input  logic                cmd_valid,
    output logic                cmd_ready,

    // Memory ports - activation buffer (matrix A read)
    output logic [ADDR_W-1:0]   act_rd_addr,
    output logic                act_rd_req,
    input  logic                act_rd_gnt,
    input  logic [DATA_W-1:0]   act_rd_rdata,
    input  logic                act_rd_rvalid,

    // Memory ports - weight buffer (matrix B read)
    output logic [ADDR_W-1:0]   wt_rd_addr,
    output logic                wt_rd_req,
    input  logic                wt_rd_gnt,
    input  logic [DATA_W-1:0]   wt_rd_rdata,
    input  logic                wt_rd_rvalid,

    // Memory ports - output write (activation buffer, matrix C)
    output logic [ADDR_W-1:0]   out_wr_addr,
    output logic [DATA_W-1:0]   out_wr_data,
    output logic                out_wr_req,
    input  logic                out_wr_gnt,

    // Memory port - bias read
    output logic [ADDR_W-1:0]   bias_rd_addr,
    output logic                bias_rd_req,
    /* verilator lint_off UNUSEDSIGNAL */
    input  logic                bias_rd_gnt,
    /* verilator lint_on UNUSEDSIGNAL */
    input  logic [DATA_W-1:0]   bias_rd_rdata,
    input  logic                bias_rd_rvalid,

    // Status
    output logic                done,
    output logic                busy
);

    // gemm_ctrl outputs
    dim_t m_idx, n_idx, k_idx;
    logic [ADDR_W-1:0] a_base, b_base, c_base, bias_base;
    /* verilator lint_off UNUSEDSIGNAL */
    dim_t m_dim, n_dim, k_dim;
    /* verilator lint_on UNUSEDSIGNAL */
    logic [4:0] quant_shift;
    act_mode_e act_mode;
    logic iter_valid, iter_ready;
    logic acc_clr, last_inner;
    logic ctrl_done, ctrl_busy;

    gemm_ctrl u_ctrl (
        .clk         (clk),
        .rst_n       (rst_n),
        .cmd         (cmd),
        .cmd_valid   (cmd_valid),
        .cmd_ready   (cmd_ready),
        .m           (m_idx),
        .n           (n_idx),
        .k_idx       (k_idx),
        .a_base      (a_base),
        .b_base      (b_base),
        .c_base      (c_base),
        .bias_base   (bias_base),
        .m_dim       (m_dim),
        .n_dim       (n_dim),
        .k_dim       (k_dim),
        .quant_shift (quant_shift),
        .act_mode    (act_mode),
        .iter_valid  (iter_valid),
        .iter_ready  (iter_ready),
        .acc_clr     (acc_clr),
        .last_inner  (last_inner),
        .gemm_done   (ctrl_done),
        .busy        (ctrl_busy)
    );

    // gemm_addr_gen
    logic [ADDR_W-1:0] a_addr, b_addr;

    gemm_addr_gen u_addr_gen (
        .a_base (a_base),
        .b_base (b_base),
        .n_dim  (n_dim),
        .k_dim  (k_dim),
        .m      (m_idx),
        .n      (n_idx),
        .k_idx  (k_idx),
        .a_addr (a_addr),
        .b_addr (b_addr)
    );

    // conv_loader (reused -- zero_pad tied to 0 for GEMM)
    logic signed [DATA_W-1:0] load_a, load_b;
    logic load_pair_valid, load_pair_ready;

    conv_loader u_loader (
        .clk            (clk),
        .rst_n          (rst_n),
        .act_addr       (a_addr),
        .wt_addr        (b_addr),
        .zero_pad       (1'b0),
        .load_valid     (iter_valid),
        .load_ready     (iter_ready),
        .act_mem_addr   (act_rd_addr),
        .act_mem_req    (act_rd_req),
        .act_mem_gnt    (act_rd_gnt),
        .act_mem_rdata  (act_rd_rdata),
        .act_mem_rvalid (act_rd_rvalid),
        .wt_mem_addr    (wt_rd_addr),
        .wt_mem_req     (wt_rd_req),
        .wt_mem_gnt     (wt_rd_gnt),
        .wt_mem_rdata   (wt_rd_rdata),
        .wt_mem_rvalid  (wt_rd_rvalid),
        .act_out        (load_a),
        .wt_out         (load_b),
        .pair_valid     (load_pair_valid),
        .pair_ready     (load_pair_ready)
    );

    // conv_accum + conv_array (reused)
    logic signed [ACC_W-1:0] acc_feedback, acc_out_val, pe_acc_out;
    logic pe_valid_out, pe_ready_out;

    // Pipeline-align control signals from gemm_ctrl.
    // Same timing contract as conv_backend: capture at iter-accept time.
    logic iter_accept;
    assign iter_accept = iter_valid & iter_ready;

    logic pe_accept;
    assign pe_accept = load_pair_valid & load_pair_ready;

    // Stage 1: capture at loader-accept time
    logic last_inner_q, acc_clr_q;
    logic [ADDR_W-1:0] wr_addr_q;
    logic [ADDR_W-1:0] bias_addr_q;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            last_inner_q <= 1'b0;
            acc_clr_q    <= 1'b0;
            wr_addr_q    <= '0;
            bias_addr_q  <= '0;
        end else if (iter_accept) begin
            last_inner_q <= last_inner;
            acc_clr_q    <= acc_clr;
            if (last_inner) begin
                wr_addr_q   <= c_base + ADDR_W'(m_idx * n_dim + n_idx);
                bias_addr_q <= bias_base + ADDR_W'(n_idx);
            end
        end
    end

    // Stage 2: capture at PE-accept time
    logic last_inner_d;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            last_inner_d <= 1'b0;
        else if (pe_accept)
            last_inner_d <= last_inner_q;
    end

    conv_accum u_accum (
        .clk          (clk),
        .rst_n        (rst_n),
        .clr          (acc_clr_q & pe_accept),
        .acc_en       (pe_valid_out & pe_ready_out),
        .mac_result   (pe_acc_out),
        .acc_out      (acc_out_val),
        .acc_feedback (acc_feedback)
    );

    conv_array u_array (
        .clk       (clk),
        .rst_n     (rst_n),
        .act_in    (load_a),
        .wt_in     (load_b),
        .acc_in    (acc_feedback),
        .acc_out   (pe_acc_out),
        .valid_in  (load_pair_valid),
        .ready_out (load_pair_ready),
        .valid_out (pe_valid_out),
        .ready_in  (pe_ready_out)
    );

    // Bias fetch: issue read when the last-inner iteration is accepted by PE
    assign bias_rd_addr = bias_addr_q;
    assign bias_rd_req  = last_inner_q & pe_accept;

    // pp_trigger fires when the PE outputs the result of a last-inner iteration
    logic pp_trigger;
    logic pp_trigger_r;
    assign pp_trigger = last_inner_d & pe_valid_out & pe_ready_out;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            pp_trigger_r <= 1'b0;
        else
            pp_trigger_r <= pp_trigger;
    end

    // Bias data: latch when rvalid fires
    logic signed [DATA_W-1:0] bias_val;
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            bias_val <= '0;
        else if (bias_rd_rvalid)
            bias_val <= bias_rd_rdata;
    end

    // PE backpressure: stall when processing a last-inner result
    logic pp_in_ready;
    assign pe_ready_out = last_inner_d ? pp_in_ready : 1'b1;

    // conv_postproc (reused)
    logic signed [DATA_W-1:0] pp_out_data;
    logic pp_out_valid, pp_out_ready;

    conv_postproc u_postproc (
        .clk         (clk),
        .rst_n       (rst_n),
        .bias_val    (bias_val),
        .quant_shift (quant_shift),
        .act_mode    (act_mode),
        .in_data     (acc_out_val),
        .in_valid    (pp_trigger_r),
        .in_ready    (pp_in_ready),
        .out_data    (pp_out_data),
        .out_valid   (pp_out_valid),
        .out_ready   (pp_out_ready)
    );

    // conv_writer (reused)
    logic [ADDR_W-1:0]  wr_out_addr;
    logic               wr_addr_valid;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            wr_out_addr   <= '0;
            wr_addr_valid <= 1'b0;
        end else if (pp_trigger) begin
            wr_out_addr   <= wr_addr_q;
            wr_addr_valid <= 1'b1;
        end else if (pp_out_valid && pp_out_ready) begin
            wr_addr_valid <= 1'b0;
        end
    end

    /* verilator lint_off UNUSEDSIGNAL */
    logic wr_mem_wr;
    /* verilator lint_on UNUSEDSIGNAL */
    logic wr_done;

    conv_writer u_writer (
        .clk        (clk),
        .rst_n      (rst_n),
        .out_addr   (wr_out_addr),
        .addr_valid (wr_addr_valid),
        .in_data    (pp_out_data),
        .in_valid   (pp_out_valid),
        .in_ready   (pp_out_ready),
        .mem_addr   (out_wr_addr),
        .mem_wdata  (out_wr_data),
        .mem_wr     (wr_mem_wr),
        .mem_req    (out_wr_req),
        .mem_gnt    (out_wr_gnt),
        .write_done (wr_done)
    );

    // Gate done/busy so the backend waits for the write pipeline to drain
    logic  pipe_empty;
    assign pipe_empty = ~pp_trigger_r & ~pp_out_valid & ~wr_addr_valid
                      &  pp_out_ready & ~wr_done;

    // ctrl_done is a 1-cycle pulse; latch it until the pipeline empties
    logic ctrl_finished;
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            ctrl_finished <= 1'b0;
        else if (done)
            ctrl_finished <= 1'b0;
        else if (ctrl_done)
            ctrl_finished <= 1'b1;
    end

    assign done = (ctrl_done | ctrl_finished) & pipe_empty;
    assign busy =  ctrl_busy | ~pipe_empty | ctrl_finished;

endmodule : gemm_backend
