// ============================================================================
// conv_backend.sv - Top-level convolution backend
//   Connects ctrl -> addr_gen -> loader -> PE array -> accum -> postproc -> writer
// ============================================================================

module conv_backend
    import npu_types_pkg::*;
    import npu_cmd_pkg::*;
(
    input  logic                clk,
    input  logic                rst_n,

    // Command from dispatch
    input  conv_cmd_t           cmd,
    input  logic                cmd_valid,
    output logic                cmd_ready,

    // Memory ports - activation buffer
    output logic [ADDR_W-1:0]   act_rd_addr,
    output logic                act_rd_req,
    input  logic                act_rd_gnt,
    input  logic [DATA_W-1:0]   act_rd_rdata,
    input  logic                act_rd_rvalid,

    // Memory ports - weight buffer
    output logic [ADDR_W-1:0]   wt_rd_addr,
    output logic                wt_rd_req,
    input  logic                wt_rd_gnt,
    input  logic [DATA_W-1:0]   wt_rd_rdata,
    input  logic                wt_rd_rvalid,

    // Memory ports - output write (activation buffer)
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

    // conv_ctrl outputs
    dim_t oh, ow, k, r, s, c_idx;
    logic [ADDR_W-1:0] act_base, wt_base, out_base, bias_base;
    /* verilator lint_off UNUSEDSIGNAL */
    dim_t in_h, in_w, in_c, out_k, filt_r, filt_s;
    dim_t stride_h, stride_w, pad_h, pad_w;
    logic [4:0] quant_shift;
    act_mode_e act_mode;
    dim_t out_h, out_w;
    /* verilator lint_on UNUSEDSIGNAL */
    logic iter_valid, iter_ready;
    logic acc_clr, last_inner;
    logic ctrl_done, ctrl_busy;

    conv_ctrl u_ctrl (
        .clk         (clk),
        .rst_n       (rst_n),
        .cmd         (cmd),
        .cmd_valid   (cmd_valid),
        .cmd_ready   (cmd_ready),
        .oh          (oh),
        .ow          (ow),
        .k           (k),
        .r           (r),
        .s           (s),
        .c           (c_idx),
        .act_base    (act_base),
        .wt_base     (wt_base),
        .out_base    (out_base),
        .bias_base   (bias_base),
        .in_h        (in_h),
        .in_w        (in_w),
        .in_c        (in_c),
        .out_k       (out_k),
        .filt_r      (filt_r),
        .filt_s      (filt_s),
        .stride_h    (stride_h),
        .stride_w    (stride_w),
        .pad_h       (pad_h),
        .pad_w       (pad_w),
        .quant_shift (quant_shift),
        .act_mode    (act_mode),
        .out_h       (out_h),
        .out_w       (out_w),
        .iter_valid  (iter_valid),
        .iter_ready  (iter_ready),
        .acc_clr     (acc_clr),
        .last_inner  (last_inner),
        .conv_done   (ctrl_done),
        .busy        (ctrl_busy)
    );

    // conv_addr_gen
    logic [ADDR_W-1:0] act_addr, wt_addr;
    logic zero_pad;
    logic addr_gen_ready, addr_gen_valid;

    // conv_loader
    logic signed [DATA_W-1:0] load_act, load_wt;
    logic load_pair_valid, load_pair_ready;
    logic load_req_ready;

    conv_addr_gen u_addr_gen (
        .clk      (clk),
        .rst_n    (rst_n),
        .in_valid (iter_valid),
        .in_ready (addr_gen_ready),
        .out_valid(addr_gen_valid),
        .out_ready(load_req_ready),
        .act_base (act_base),
        .wt_base  (wt_base),
        .in_h     (in_h),
        .in_w     (in_w),
        .in_c     (in_c),
        .filt_r   (filt_r),
        .filt_s   (filt_s),
        .oh       (oh),
        .ow       (ow),
        .k        (k),
        .r        (r),
        .s        (s),
        .c        (c_idx),
        .stride_h (stride_h),
        .stride_w (stride_w),
        .pad_h    (pad_h),
        .pad_w    (pad_w),
        .act_addr (act_addr),
        .wt_addr  (wt_addr),
        .zero_pad (zero_pad)
    );

    conv_loader u_loader (
        .clk            (clk),
        .rst_n          (rst_n),
        .act_addr       (act_addr),
        .wt_addr        (wt_addr),
        .zero_pad       (zero_pad),
        .load_valid     (addr_gen_valid),
        .load_ready     (load_req_ready),
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
        .act_out        (load_act),
        .wt_out         (load_wt),
        .pair_valid     (load_pair_valid),
        .pair_ready     (load_pair_ready)
    );

    assign iter_ready = addr_gen_ready;

    // conv_accum + conv_array
    logic signed [ACC_W-1:0] acc_feedback, acc_out_val, pe_acc_out;
    logic pe_valid_out, pe_ready_out;

    logic iter_accept;
    logic load_accept;
    logic pe_accept;

    assign iter_accept = iter_valid & iter_ready;
    assign load_accept = addr_gen_valid & load_req_ready;
    assign pe_accept   = load_pair_valid & load_pair_ready;

    // Metadata pipeline: iter_accept -> load_accept -> pe_accept.
    logic last_inner_iter_r, acc_clr_iter_r;
    logic [ADDR_W-1:0] wr_addr_iter_r;
    logic [ADDR_W-1:0] bias_addr_iter_r;

    logic last_inner_load_r, acc_clr_load_r;
    logic [ADDR_W-1:0] wr_addr_load_r;
    logic [ADDR_W-1:0] bias_addr_load_r;

    logic last_inner_d;
    logic [ADDR_W-1:0] wr_addr_d;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            last_inner_iter_r <= 1'b0;
            acc_clr_iter_r    <= 1'b0;
            wr_addr_iter_r    <= '0;
            bias_addr_iter_r  <= '0;
        end else if (iter_accept) begin
            last_inner_iter_r <= last_inner;
            acc_clr_iter_r    <= acc_clr;
            if (last_inner) begin
                wr_addr_iter_r   <= out_base + ADDR_W'(((oh * out_w) + ow) * out_k + k);
                bias_addr_iter_r <= bias_base + ADDR_W'(k);
            end
        end
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            last_inner_load_r <= 1'b0;
            acc_clr_load_r    <= 1'b0;
            wr_addr_load_r    <= '0;
            bias_addr_load_r  <= '0;
        end else if (load_accept) begin
            last_inner_load_r <= last_inner_iter_r;
            acc_clr_load_r    <= acc_clr_iter_r;
            wr_addr_load_r    <= wr_addr_iter_r;
            bias_addr_load_r  <= bias_addr_iter_r;
        end
    end

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            last_inner_d <= 1'b0;
            wr_addr_d    <= '0;
        end else if (pe_accept) begin
            last_inner_d <= last_inner_load_r;
            wr_addr_d    <= wr_addr_load_r;
        end
    end

    conv_accum u_accum (
        .clk          (clk),
        .rst_n        (rst_n),
        .clr          (acc_clr_load_r & pe_accept),
        .acc_en       (pe_valid_out & pe_ready_out),
        .mac_result   (pe_acc_out),
        .acc_out      (acc_out_val),
        .acc_feedback (acc_feedback)
    );

    conv_array u_array (
        .clk       (clk),
        .rst_n     (rst_n),
        .act_in    (load_act),
        .wt_in     (load_wt),
        .acc_in    (acc_feedback),
        .acc_out   (pe_acc_out),
        .valid_in  (load_pair_valid),
        .ready_out (load_pair_ready),
        .valid_out (pe_valid_out),
        .ready_in  (pe_ready_out)
    );

    // Bias fetch: issue read when the last-inner iteration is accepted by PE.
    assign bias_rd_addr = bias_addr_load_r;
    assign bias_rd_req  = last_inner_load_r & pe_accept;

    // pp_trigger fires when the PE outputs the result of a last-inner iteration.
    logic pp_trigger;
    logic pp_trigger_r;
    assign pp_trigger = last_inner_d & pe_valid_out & pe_ready_out;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            pp_trigger_r <= 1'b0;
        else
            pp_trigger_r <= pp_trigger;
    end

    // Bias data: latch when rvalid fires so it stays stable through postproc.
    logic signed [DATA_W-1:0] bias_val;
    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            bias_val <= '0;
        else if (bias_rd_rvalid)
            bias_val <= bias_rd_rdata;
    end

    // PE backpressure: stall when processing a last-inner result.
    // Keep write-address metadata and postproc data in lockstep.
    logic pp_in_ready;
    logic wr_addr_in_ready;
    assign pe_ready_out = last_inner_d ? (pp_in_ready & wr_addr_in_ready) : 1'b1;

    // conv_postproc
    logic signed [DATA_W-1:0] pp_out_data;
    logic pp_out_valid, pp_out_ready;

    postproc u_postproc (
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

    // conv_writer
    logic [ADDR_W-1:0]  wr_out_addr;
    logic               wr_addr_valid;

    skid_buffer #(
        .DATA_W (ADDR_W)
    ) u_wr_addr_q (
        .clk       (clk),
        .rst_n     (rst_n),
        .in_data   (wr_addr_d),
        .in_valid  (pp_trigger),
        .in_ready  (wr_addr_in_ready),
        .out_data  (wr_out_addr),
        .out_valid (wr_addr_valid),
        .out_ready (pp_out_valid & pp_out_ready)
    );

    /* verilator lint_off UNUSEDSIGNAL */
    logic wr_mem_wr;
    logic wr_done;
    /* verilator lint_on UNUSEDSIGNAL */

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

    // Gate done/busy so the backend waits for the write pipeline to drain.
    logic pipe_empty;
    assign pipe_empty = ~pp_trigger_r & ~pp_out_valid & ~wr_addr_valid
                      &  pp_out_ready;

    // ctrl_done is a 1-cycle pulse; latch it until the pipeline empties.
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

endmodule : conv_backend
