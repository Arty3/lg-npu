// ============================================================================
// mmio_if.sv - Host <-> Shell MMIO interface
// ============================================================================

interface mmio_if;

    import npu_types_pkg::*;

    logic [MMIO_ADDR_W-1:0] addr;
    logic [MMIO_DATA_W-1:0] wdata;
    logic [MMIO_DATA_W-1:0] rdata;
    logic                   wr;      // 1 = write, 0 = read
    logic                   valid;
    logic                   ready;

    modport host (
        output addr, wdata, wr, valid,
        input  rdata, ready
    );

    modport device (
        input  addr, wdata, wr, valid,
        output rdata, ready
    );

endinterface : mmio_if
