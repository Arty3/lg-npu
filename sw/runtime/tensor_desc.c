/* tensor_desc.c - Tensor descriptor validation and layout conversion */

#include "../shared/annotations.h"
#include "../uapi/lgnpu_tensor.h"

#include <string.h>

enum npu_tensor_err npu_tensor_validate(const struct npu_tensor_desc* desc)
{
	if (UNLIKELY(!desc))
		return LGNPU_TENSOR_ERR_NULL_PTR;

	if (UNLIKELY(!desc->dim_n || !desc->dim_h ||
	             !desc->dim_w || !desc->dim_c))
		return LGNPU_TENSOR_ERR_ZERO_DIM;

	if (UNLIKELY(desc->dim_n != 1))
		return LGNPU_TENSOR_ERR_BATCH_UNSUP;

	if (UNLIKELY((uint32_t)desc->layout >= LGNPU_LAYOUT_COUNT))
		return LGNPU_TENSOR_ERR_LAYOUT_UNKNOWN;

	uint32_t hw, hwc;

	if (UNLIKELY(__builtin_mul_overflow((uint32_t)desc->dim_h, desc->dim_w, &hw) ||
	             __builtin_mul_overflow(hw, desc->dim_c, &hwc)))
		return LGNPU_TENSOR_ERR_OVERFLOW;

	return LGNPU_TENSOR_OK;
}

/* NCHW -> NHWC permutation.
 *   src index: ((n*C + c)*H + h)*W + w
 *   dst index: ((n*H + h)*W + w)*C + c
 *
 * Loop layout is about as vectorization friendly
 * as possible, compiler will do its best. */
HOT_CALL NO_NULL_ARGS
static void convert_nchw_to_nhwc(
	int8_t*       RESTRICT dst,
	const int8_t* RESTRICT src,
	uint16_t               N,
	uint16_t               H,
	uint16_t               W,
	uint16_t               C)
{
	const size_t HW  = (size_t)H * W;
	const size_t WC  = (size_t)W * C;
	const size_t HWC = HW * C;

	for (uint16_t n = 0; n < N; ++n)
	{
		int8_t*       dn = dst + n * HWC;
		const int8_t* sn = src + n * HWC;

		for (uint16_t c = 0; c < C; ++c)
		{
			int8_t*       dc = dn + c;
			const int8_t* sc = sn + (size_t)c * HW;

			for (uint16_t h = 0; h < H; ++h)
			{
				const int8_t* sh = sc + (size_t)h * W;
				int8_t*       dh = dc + h * WC;

				for (uint16_t w = 0; w < W; ++w)
					dh[w * C] = sh[w];
			}
		}
	}
}

enum npu_tensor_err npu_tensor_convert_to_nhwc(
	void*        RESTRICT         dst,
	const void*  RESTRICT         src,
	const struct npu_tensor_desc* desc,
	size_t                        buf_bytes)
{
	if (UNLIKELY(!dst || !src))
		return LGNPU_TENSOR_ERR_NULL_PTR;

	const enum npu_tensor_err err = npu_tensor_validate(desc);

	if (UNLIKELY(err != LGNPU_TENSOR_OK))
		return err;

	const size_t total = npu_tensor_byte_size(desc);

	if (UNLIKELY(buf_bytes < total))
		return LGNPU_TENSOR_ERR_BUF_TOO_SMALL;

	const int8_t* in  = (const int8_t*)src;
	int8_t*       out = (int8_t*)dst;

	if (desc->layout == LGNPU_LAYOUT_NHWC)
	{
		/* dst != src check is optimized out by restrict,
		 * use memmove to handle UB from memcpy and keep
		 * restrict since it's valuable for permutations. */
		memmove(out, in, total);
		return LGNPU_TENSOR_OK;
	}

	if (desc->layout == LGNPU_LAYOUT_NCHW)
	{
		/* On the other hand, here if dst == src
		 * this is quite bad. The memmove above
		 * is a best effort safety net. */
		convert_nchw_to_nhwc(
			out, in,
			desc->dim_n, desc->dim_h,
			desc->dim_w, desc->dim_c
		);
		return LGNPU_TENSOR_OK;
	}

	return LGNPU_TENSOR_ERR_LAYOUT_UNKNOWN;
}
