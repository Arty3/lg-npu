// ============================================================================
// tensor_if.sv - Tensor metadata from dispatch to backend
// ============================================================================

interface tensor_if;

    import npu_types_pkg::*;
    import npu_tensor_pkg::*;

    tensor_desc_t in_desc;
    tensor_desc_t wt_desc;
    tensor_desc_t out_desc;
    logic         valid;
    logic         ready;

    modport source (
        output in_desc, wt_desc, out_desc, valid,
        input  ready
    );

    modport sink (
        input  in_desc, wt_desc, out_desc, valid,
        output ready
    );

endinterface : tensor_if
