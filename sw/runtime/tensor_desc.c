// tensor_desc.c - Tensor descriptor validation and layout conversion

#include "../uapi/npu_tensor.h"
#include "../shared/annotations.h"
#include <string.h>

enum npu_tensor_err
npu_tensor_validate(const struct npu_tensor_desc* desc)
{
    if (!desc)
    {
        return NPU_TENSOR_ERR_NULL_PTR;
    }

    if (desc->dim_n == 0 || desc->dim_h == 0 ||
        desc->dim_w == 0 || desc->dim_c == 0)
    {
        return NPU_TENSOR_ERR_ZERO_DIM;
    }

    if (desc->dim_n != 1)
    {
        return NPU_TENSOR_ERR_BATCH_UNSUP;
    }

    if ((unsigned)desc->layout >= NPU_LAYOUT_COUNT)
    {
        return NPU_TENSOR_ERR_LAYOUT_UNKNOWN;
    }

    // Overflow check: H*W, then H*W*C, all in uint32_t.
    uint32_t hw  = (uint32_t)desc->dim_h * desc->dim_w;
    uint32_t hwc = hw * desc->dim_c;

    if (hw / desc->dim_h != desc->dim_w)
    {
        return NPU_TENSOR_ERR_OVERFLOW;
    }
    if (hwc / hw != desc->dim_c)
    {
        return NPU_TENSOR_ERR_OVERFLOW;
    }

    return NPU_TENSOR_OK;
}

size_t
npu_tensor_element_count(const struct npu_tensor_desc* desc)
{
    return (size_t)desc->dim_n * desc->dim_h * desc->dim_w * desc->dim_c;
}

size_t
npu_tensor_byte_size(const struct npu_tensor_desc* desc)
{
    // INT8: one byte per element.
    return npu_tensor_element_count(desc);
}

size_t
npu_tensor_nhwc_offset(const struct npu_tensor_desc* desc,
                       uint16_t n, uint16_t h, uint16_t w, uint16_t c)
{
    return (size_t)desc->base_addr +
           ((((size_t)n * desc->dim_h + h) * desc->dim_w + w) *
            desc->dim_c + c);
}

// NCHW -> NHWC permutation.
//   src index: ((n*C + c)*H + h)*W + w
//   dst index: ((n*H + h)*W + w)*C + c
static void
convert_nchw_to_nhwc(int8_t* RESTRICT dst,
                     const int8_t* RESTRICT src,
                     uint16_t N, uint16_t H, uint16_t W, uint16_t C)
{
    for (uint16_t n = 0; n < N; ++n)
    {
        for (uint16_t c = 0; c < C; ++c)
        {
            for (uint16_t h = 0; h < H; ++h)
            {
                for (uint16_t w = 0; w < W; ++w)
                {
                    size_t si = (((size_t)n * C + c) * H + h) * W + w;
                    size_t di = (((size_t)n * H + h) * W + w) * C + c;
                    dst[di] = src[si];
                }
            }
        }
    }
}

enum npu_tensor_err
npu_tensor_convert_to_nhwc(void* RESTRICT dst,
                           const void* RESTRICT src,
                           const struct npu_tensor_desc* desc,
                           size_t buf_bytes)
{
    enum npu_tensor_err err = npu_tensor_validate(desc);
    if (err != NPU_TENSOR_OK)
    {
        return err;
    }

    size_t total = npu_tensor_byte_size(desc);
    if (buf_bytes < total)
    {
        return NPU_TENSOR_ERR_BUF_TOO_SMALL;
    }

    const int8_t* in  = (const int8_t*)src;
    int8_t*       out = (int8_t*)dst;

    switch (desc->layout)
    {
    case NPU_LAYOUT_NHWC:
        memcpy(out, in, total);
        break;

    case NPU_LAYOUT_NCHW:
        convert_nchw_to_nhwc(out, in,
                             desc->dim_n, desc->dim_h,
                             desc->dim_w, desc->dim_c);
        break;

    default:
        return NPU_TENSOR_ERR_LAYOUT_UNKNOWN;
    }

    return NPU_TENSOR_OK;
}
