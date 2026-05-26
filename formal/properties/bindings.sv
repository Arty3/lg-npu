// ============================================================================
// bindings.sv - Property bindings for simulation/formal runs
// ============================================================================

`ifdef FORMAL
    `define BIND_PROPS 1
`elsif SIMULATION
    `define BIND_PROPS 1
`endif

`ifdef BIND_PROPS

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

// Generic valid/ready protocol checks on the conv backend's main
// handshake points. Each catches the "valid dropped before ready"
// and "data changed while stalled" class of bugs.
bind conv_backend handshake_props #(.DATA_W(8)) u_hs_load_pair (
    .clk   (clk),
    .rst_n (rst_n),
    .valid (load_pair_valid),
    .ready (load_pair_ready),
    .data  (load_act)
);

bind conv_backend handshake_props #(.DATA_W(16)) u_hs_addr_gen (
    .clk   (clk),
    .rst_n (rst_n),
    .valid (addr_gen_valid),
    .ready (load_req_ready),
    .data  (act_addr)
);

bind conv_backend handshake_props #(.DATA_W(32)) u_hs_pe_out (
    .clk   (clk),
    .rst_n (rst_n),
    .valid (pe_valid_out),
    .ready (pe_ready_out),
    .data  (pe_acc_out)
);

bind conv_backend handshake_props #(.DATA_W(8)) u_hs_pp_out (
    .clk   (clk),
    .rst_n (rst_n),
    .valid (pp_out_valid),
    .ready (pp_out_ready),
    .data  (pp_out_data)
);

`endif

`undef BIND_PROPS
