// npu_tensor.h - Tensor layout types, descriptors, and runtime conversion API

#ifndef NPU_TENSOR_H
#define NPU_TENSOR_H

#include "../shared/annotations.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Tensor element data type.
// Only INT8 is supported in the current hardware revision.
enum npu_dtype
{
    NPU_DTYPE_INT8 = 0,
};

// Tensor memory layout.
//
// The NPU hardware operates exclusively in NHWC (channel-last). All
// data must be in NHWC order when it reaches device SRAM.
//
// The runtime accepts tensors in any layout listed here and converts
// them to NHWC before loading.
//
//   NHWC  - canonical internal layout (no conversion needed)
//   NCHW  - common framework default (PyTorch, etc.)
enum npu_tensor_layout
{
    NPU_LAYOUT_NHWC  = 0,
    NPU_LAYOUT_NCHW  = 1,
    NPU_LAYOUT_COUNT
};

// Tensor descriptor.
//
// Carries the metadata needed to address and convert a tensor.
// Dimensions always use the NHWC naming convention regardless of
// the declared layout:
//   dim_n  - batch       (must be 1 in v0.1)
//   dim_h  - height      (or rows for 2-D matrices)
//   dim_w  - width       (or columns for 2-D matrices)
//   dim_c  - channels    (innermost in NHWC)
struct npu_tensor_desc
{
    uint16_t                base_addr;
    uint16_t                dim_n;
    uint16_t                dim_h;
    uint16_t                dim_w;
    uint16_t                dim_c;
    enum npu_tensor_layout  layout;
    enum npu_dtype          dtype;
};

// Validation error codes returned by npu_tensor_validate().
enum npu_tensor_err
{
    NPU_TENSOR_OK                 = 0,
    NPU_TENSOR_ERR_NULL_PTR       = 1,
    NPU_TENSOR_ERR_ZERO_DIM       = 2,
    NPU_TENSOR_ERR_BATCH_UNSUP    = 3,
    NPU_TENSOR_ERR_LAYOUT_UNKNOWN = 4,
    NPU_TENSOR_ERR_OVERFLOW       = 5,
    NPU_TENSOR_ERR_BUF_TOO_SMALL  = 6,
};

// Validate a tensor descriptor.
// Checks: non-null, positive dimensions, batch == 1, known layout,
// and that N*H*W*C does not overflow a uint32_t.
NODISCARD enum npu_tensor_err
npu_tensor_validate(const struct npu_tensor_desc* desc);

// Total number of elements (N * H * W * C).
// Caller must ensure desc is valid (call npu_tensor_validate first).
PURE_CALL NODISCARD size_t
npu_tensor_element_count(const struct npu_tensor_desc* desc);

// Total byte size of the tensor data (element_count * element_size).
// For INT8 this equals element_count.
PURE_CALL NODISCARD size_t
npu_tensor_byte_size(const struct npu_tensor_desc* desc);

// Compute the linear byte offset for element [n][h][w][c] in NHWC
// order, relative to base_addr.
PURE_CALL NODISCARD size_t
npu_tensor_nhwc_offset(const struct npu_tensor_desc* desc,
                       uint16_t n, uint16_t h, uint16_t w, uint16_t c);

// Convert tensor data from its declared layout to NHWC (canonical).
//
//   dst       - destination buffer filled in NHWC order
//   src       - source buffer in the layout given by desc->layout
//   desc      - tensor descriptor (layout field selects the conversion)
//   buf_bytes - size of both src and dst buffers in bytes
//
// If desc->layout is already NHWC the data is copied unchanged.
// Returns NPU_TENSOR_OK on success.
NODISCARD enum npu_tensor_err
npu_tensor_convert_to_nhwc(void* RESTRICT dst,
                           const void* RESTRICT src,
                           const struct npu_tensor_desc* desc,
                           size_t buf_bytes);

// Returns true when the layout is the canonical internal layout (NHWC).
NODISCARD static INLINE bool
npu_tensor_is_canonical(enum npu_tensor_layout layout)
{
    return layout == NPU_LAYOUT_NHWC;
}

#endif // NPU_TENSOR_H
