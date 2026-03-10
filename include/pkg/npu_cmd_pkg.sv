// ============================================================================
// npu_cmd_pkg.sv - Command descriptor definitions
// ============================================================================

package npu_cmd_pkg;

    import npu_types_pkg::*;

    // Opcode enum (single opcode for now)
    typedef enum logic [3:0]
	{
        OP_CONV = 4'h1
    }   opcode_e;

    // Convolution command descriptor
    //   Submitted by the host through the command queue.
    typedef struct packed
	{
        opcode_e               opcode;

        logic [ADDR_W-1:0]     act_in_addr;
        logic [ADDR_W-1:0]     act_out_addr;
        logic [ADDR_W-1:0]     weight_addr;
        logic [ADDR_W-1:0]     bias_addr;

        // Spatial dimensions
        dim_t                  in_h;
        dim_t                  in_w;
        dim_t                  in_c;
        dim_t                  out_k;
        dim_t                  filt_r;
        dim_t                  filt_s;

        // Stride / padding
        dim_t                  stride_h;
        dim_t                  stride_w;
        dim_t                  pad_h;
        dim_t                  pad_w;

        // Quantization shift (right arithmetic shift for INT32->INT8)
        logic [4:0]            quant_shift;
    }   conv_cmd_t;

    // Generic command wrapper (reserved for future multi-backend dispatch)
    localparam int CMD_PAYLOAD_W = $bits(conv_cmd_t);

    typedef struct packed
	{
        opcode_e                   opcode;
        logic [CMD_PAYLOAD_W-1:0]  payload;
    }   cmd_t;

endpackage : npu_cmd_pkg
