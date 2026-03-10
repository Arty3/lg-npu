// ============================================================================
// mem_req_rsp_if.sv - Generic memory request / response interface
// ============================================================================

interface mem_req_rsp_if #(
    parameter int ADDR_W = 16,
    parameter int DATA_W = 32
);

    // Request channel
    logic [ADDR_W-1:0]  req_addr;
    logic [DATA_W-1:0]  req_wdata;
    logic               req_wr;     // 1 = write, 0 = read
    logic               req_valid;
    logic               req_ready;

    // Response channel
    logic [DATA_W-1:0]  rsp_rdata;
    logic               rsp_valid;
    logic               rsp_ready;

    modport master (
        output req_addr,  req_wdata, req_wr, req_valid,
        input  req_ready,
        input  rsp_rdata, rsp_valid,
        output rsp_ready
    );

    modport slave (
        input  req_addr,  req_wdata, req_wr, req_valid,
        output req_ready,
        output rsp_rdata, rsp_valid,
        input  rsp_ready
    );

endinterface : mem_req_rsp_if
