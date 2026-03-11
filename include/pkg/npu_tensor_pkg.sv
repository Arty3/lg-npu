// ============================================================================
// npu_tensor_pkg.sv - Tensor layout definitions
//   Canonical internal layout: NHWC (channel-last).
//   The runtime (sw/) converts from external layouts (e.g. NCHW) to NHWC
//   before data reaches device SRAM.  See sw/uapi/npu_tensor.h for the
//   full set of runtime-supported layouts.
// ============================================================================

package npu_tensor_pkg;

    import npu_types_pkg::*;

    // Tensor layout selector.
    // Hardware accepts LAYOUT_NHWC only.  LAYOUT_NCHW is reserved so the
    // enum encoding stays consistent with the C-side npu_tensor_layout.
    typedef enum logic [1:0]
	{
        LAYOUT_NHWC = 2'b00,
        LAYOUT_NCHW = 2'b01   // runtime-only; rejected by hardware
    }   tensor_layout_e;

    // Tensor descriptor - metadata passed alongside data
    typedef struct packed
	{
        logic [ADDR_W-1:0] base_addr;    // byte offset in local SRAM
        dim_t              dim_n;        // batch (always 1)
        dim_t              dim_h;        // height
        dim_t              dim_w;        // width
        dim_t              dim_c;        // channels
        tensor_layout_e    layout;       // must be LAYOUT_NHWC at hardware
    }   tensor_desc_t;

endpackage : npu_tensor_pkg
