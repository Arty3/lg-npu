// ============================================================================
// npu_tensor_pkg.sv - Tensor layout definitions
// ============================================================================

package npu_tensor_pkg;

    import npu_types_pkg::*;

    // Layout enum (only NHWC for now)
    typedef enum logic [1:0]
	{
        LAYOUT_NHWC = 2'b00
    }   tensor_layout_e;

    // Tensor descriptor - metadata passed alongside data
    typedef struct packed
	{
        logic [ADDR_W-1:0] base_addr;    // byte offset in local SRAM
        dim_t              dim_n;        // batch (always 1)
        dim_t              dim_h;        // height
        dim_t              dim_w;        // width
        dim_t              dim_c;        // channels
        tensor_layout_e    layout;       // NHWC
    }   tensor_desc_t;

endpackage : npu_tensor_pkg
