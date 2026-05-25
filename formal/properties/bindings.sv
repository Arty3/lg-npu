// ============================================================================
// bindings.sv - Property bindings for simulation/formal runs
// ============================================================================

`ifdef FORMAL

bind fifo fifo_props #(
    .DATA_W(DATA_W),
    .DEPTH (DEPTH),
    .PTR_W (PTR_W)
) u_fifo_props (
    .clk      (clk),
    .rst_n    (rst_n),
    .wr_ptr   (wr_ptr),
    .rd_ptr   (rd_ptr),
    .full     (full),
    .empty    (empty),
    .wr_valid (wr_valid),
    .wr_ready (wr_ready),
    .rd_valid (rd_valid),
    .rd_ready (rd_ready),
    .wr_data  (wr_data),
    .rd_data  (rd_data)
);

bind npu_irq_ctrl irq_props u_irq_props (
    .clk        (clk),
    .rst_n      (rst_n),
    .cmd_done   (cmd_done),
    .err        (err),
    .irq_enable (irq_enable),
    .irq_clear  (irq_clear),
    .irq_status (irq_status),
    .irq_out    (irq_out)
);

bind npu_dma_frontend dma_props u_dma_props (
    .clk        (clk),
    .rst_n      (rst_n),
    .rd_busy    (rd_busy),
    .wr_busy    (wr_busy),
    .rd_ext_req (rd_ext_req),
    .wr_ext_req (wr_ext_req),
    .ext_mem_req(ext_mem_req),
    .ext_mem_wr (ext_mem_wr),
    .dma_done   (dma_done)
);

bind npu_core mem_req_rsp_props u_mem_req_rsp_props (
    .clk    (clk),
    .rst_n  (rst_n),
    .req    (be_act_rd_req),
    .gnt    (be_act_rd_gnt),
    .rvalid (be_act_rd_rvalid)
);

`elsif SIMULATION

bind fifo fifo_props #(
    .DATA_W(DATA_W),
    .DEPTH (DEPTH),
    .PTR_W (PTR_W)
) u_fifo_props (
    .clk      (clk),
    .rst_n    (rst_n),
    .wr_ptr   (wr_ptr),
    .rd_ptr   (rd_ptr),
    .full     (full),
    .empty    (empty),
    .wr_valid (wr_valid),
    .wr_ready (wr_ready),
    .rd_valid (rd_valid),
    .rd_ready (rd_ready),
    .wr_data  (wr_data),
    .rd_data  (rd_data)
);

bind npu_irq_ctrl irq_props u_irq_props (
    .clk        (clk),
    .rst_n      (rst_n),
    .cmd_done   (cmd_done),
    .err        (err),
    .irq_enable (irq_enable),
    .irq_clear  (irq_clear),
    .irq_status (irq_status),
    .irq_out    (irq_out)
);

bind npu_dma_frontend dma_props u_dma_props (
    .clk        (clk),
    .rst_n      (rst_n),
    .rd_busy    (rd_busy),
    .wr_busy    (wr_busy),
    .rd_ext_req (rd_ext_req),
    .wr_ext_req (wr_ext_req),
    .ext_mem_req(ext_mem_req),
    .ext_mem_wr (ext_mem_wr),
    .dma_done   (dma_done)
);

bind npu_core mem_req_rsp_props u_mem_req_rsp_props (
    .clk    (clk),
    .rst_n  (rst_n),
    .req    (be_act_rd_req),
    .gnt    (be_act_rd_gnt),
    .rvalid (be_act_rd_rvalid)
);

`endif
