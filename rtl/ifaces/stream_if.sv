// ============================================================================
// stream_if.sv - Point-to-point data stream with valid/ready handshake
// ============================================================================

interface stream_if #(
    parameter int DATA_W = 32
);

    logic [DATA_W-1:0] data;
    logic              valid;
    logic              ready;

    modport source (
        output data, valid,
        input  ready
    );

    modport sink (
        input  data, valid,
        output ready
    );

endinterface : stream_if
