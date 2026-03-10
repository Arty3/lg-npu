// ============================================================================
// vec_backend.sv - Top-level element-wise vector backend
//   Connects vec_ctrl.  Does not use bias ports (tied off).
//   Source A from activation buffer, source B from weight buffer.
// ============================================================================

module vec_backend
    import npu_types_pkg::*;
    import npu_cmd_pkg::*;
(
    input  logic                clk,
    input  logic                rst_n,

    // Command from dispatch
    input  conv_cmd_t           cmd,
    input  logic                cmd_valid,
    output logic                cmd_ready,

    // Memory ports - activation buffer read (source A)
    output logic [ADDR_W-1:0]   act_rd_addr,
    output logic                act_rd_req,
    input  logic                act_rd_gnt,
    input  logic [DATA_W-1:0]   act_rd_rdata,
    input  logic                act_rd_rvalid,

    // Memory ports - weight buffer read (source B)
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

    // Memory port - bias read (unused)
    output logic [ADDR_W-1:0]   bias_rd_addr,
    output logic                bias_rd_req,
    /* verilator lint_off UNUSEDSIGNAL */
    input  logic                bias_rd_gnt,
    input  logic [DATA_W-1:0]   bias_rd_rdata,
    input  logic                bias_rd_rvalid,
    /* verilator lint_on UNUSEDSIGNAL */

    // Status
    output logic                done,
    output logic                busy
);

    // Tie off unused bias port
    assign bias_rd_addr = '0;
    assign bias_rd_req  = 1'b0;

    vec_ctrl u_ctrl (
        .clk           (clk),
        .rst_n         (rst_n),
        .cmd           (cmd),
        .cmd_valid     (cmd_valid),
        .cmd_ready     (cmd_ready),
        .act_rd_addr   (act_rd_addr),
        .act_rd_req    (act_rd_req),
        .act_rd_gnt    (act_rd_gnt),
        .act_rd_rdata  (act_rd_rdata),
        .act_rd_rvalid (act_rd_rvalid),
        .wt_rd_addr    (wt_rd_addr),
        .wt_rd_req     (wt_rd_req),
        .wt_rd_gnt     (wt_rd_gnt),
        .wt_rd_rdata   (wt_rd_rdata),
        .wt_rd_rvalid  (wt_rd_rvalid),
        .out_wr_addr   (out_wr_addr),
        .out_wr_data   (out_wr_data),
        .out_wr_req    (out_wr_req),
        .out_wr_gnt    (out_wr_gnt),
        .done          (done),
        .busy          (busy)
    );

endmodule : vec_backend
