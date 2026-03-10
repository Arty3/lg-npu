// ============================================================================
// cmd_if.sv - Internal command interface (decode -> queue -> core)
// ============================================================================

interface cmd_if;

    import npu_cmd_pkg::*;

    conv_cmd_t cmd;
    logic      valid;
    logic      ready;

    modport source (
        output cmd, valid,
        input  ready
    );

    modport sink (
        input  cmd, valid,
        output ready
    );

endinterface : cmd_if
