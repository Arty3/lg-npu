/* test_layout.c - Layout conversion tests (NCHW -> NHWC)
 *
 * Verifies npu_tensor_convert_to_nhwc with small, medium, and random tensors.
 * All randomized tests use fixed seeds for reproducible CI results.
 */

#include "../../uapi/lgnpu_tensor.h"
#include "../test_harness.h"

#include <stdlib.h>
#include <string.h>

/* Software reference: NCHW -> NHWC permutation */
static inline void ref_nchw_to_nhwc(
    int8_t*       dst,
    const int8_t* src,
    uint16_t      H,
    uint16_t      W,
    uint16_t      C)
{
    for (uint16_t h = 0; h < H; ++h)
        for (uint16_t w = 0; w < W; ++w)
            for (uint16_t c = 0; c < C; ++c)
                dst[((size_t)h * W + w) * C + c] = \
                    src[((size_t)c * H + h) * W + w];
}

/* Small: 1x2x3x2 (12 elements) */
static void test_small_nchw_to_nhwc(void)
{
    TEST_BEGIN("layout: small NCHW->NHWC 1x2x3x2");

    struct npu_tensor_desc d = {
        .base_addr = 0,
        .dim_n     = 1,
        .dim_h     = 2,
        .dim_w     = 3,
        .dim_c     = 2,
        .layout    = LGNPU_LAYOUT_NCHW,
        .dtype     = LGNPU_DTYPE_INT8
    };

    /* NCHW data: channel 0 = [0..5], channel 1 = [6..11] */
    const int8_t src[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

    int8_t dst[12];
    int8_t ref[12];

    ref_nchw_to_nhwc(ref, src, 2, 3, 2);

    enum npu_tensor_err err = npu_tensor_convert_to_nhwc(dst, src, &d, 12);

    ASSERT_EQ(err, LGNPU_TENSOR_OK);
    ASSERT_MEM_EQ(dst, ref, 12);

    TEST_PASS();
}

/* NHWC passthrough (memcpy path) */
static void test_nhwc_passthrough(void)
{
    TEST_BEGIN("layout: NHWC passthrough (no conversion)");

    struct npu_tensor_desc d = {
        .base_addr = 0,
        .dim_n     = 1,
        .dim_h     = 4,
        .dim_w     = 4,
        .dim_c     = 3,
        .layout    = LGNPU_LAYOUT_NHWC,
        .dtype     = LGNPU_DTYPE_INT8
    };

    int8_t src[48];
    int8_t dst[48];

    for (int i = 0; i < 48; ++i)
        src[i] = (int8_t)i;

    enum npu_tensor_err err = npu_tensor_convert_to_nhwc(dst, src, &d, 48);

    ASSERT_EQ(err, LGNPU_TENSOR_OK);
    ASSERT_MEM_EQ(dst, src, 48);

    TEST_PASS();
}

/* Medium: 1x16x16x8 (2048 elements) */
static void test_medium_nchw_to_nhwc(void)
{
    TEST_BEGIN("layout: medium NCHW->NHWC 1x16x16x8");

    struct npu_tensor_desc d = {
        .base_addr = 0,
        .dim_n     = 1,
        .dim_h     = 16,
        .dim_w     = 16,
        .dim_c     = 8,
        .layout    = LGNPU_LAYOUT_NCHW,
        .dtype     = LGNPU_DTYPE_INT8
    };

    const size_t sz = 2048;

    int8_t* src = (int8_t*)malloc(sz);
    int8_t* dst = (int8_t*)malloc(sz);
    int8_t* ref = (int8_t*)malloc(sz);

    ASSERT_TRUE(src && dst && ref);

    /* Fill with deterministic pattern */
    for (size_t i = 0; i < sz; ++i)
        src[i] = (int8_t)(i & 0x7F);

    ref_nchw_to_nhwc(ref, src, 16, 16, 8);

    enum npu_tensor_err err = npu_tensor_convert_to_nhwc(dst, src, &d, sz);

    ASSERT_EQ(err, LGNPU_TENSOR_OK);
    ASSERT_MEM_EQ(dst, ref, sz);

    free(src);
    free(dst);
    free(ref);

    TEST_PASS();
}

/* Random tensors with fixed seed */
static void test_random_tensors(void)
{
    TEST_BEGIN("layout: 20 random NCHW->NHWC (seed=42)");

    srand(42);

    for (int t = 0; t < 20; ++t)
    {
        const uint16_t H = (uint16_t)(1 + rand() % 32);
        const uint16_t W = (uint16_t)(1 + rand() % 32);
        const uint16_t C = (uint16_t)(1 + rand() % 16);
        const size_t   sz = (size_t)H * W * C;

        struct npu_tensor_desc d = {
            .base_addr = 0,
            .dim_n     = 1,
            .dim_h     = H,
            .dim_w     = W,
            .dim_c     = C,
            .layout    = LGNPU_LAYOUT_NCHW,
            .dtype     = LGNPU_DTYPE_INT8
        };

        int8_t* src = (int8_t*)malloc(sz);
        int8_t* dst = (int8_t*)malloc(sz);
        int8_t* ref = (int8_t*)malloc(sz);

        ASSERT_TRUE(src && dst && ref);

        for (size_t i = 0; i < sz; ++i)
            src[i] = (int8_t)(rand() & 0xFF);

        ref_nchw_to_nhwc(ref, src, H, W, C);

        enum npu_tensor_err err = npu_tensor_convert_to_nhwc(dst, src, &d, sz);
        if (err != LGNPU_TENSOR_OK || memcmp(dst, ref, sz) != 0)
        {
            free(src);
            free(dst);
            free(ref);

            TEST_FAIL("random tensor mismatch");
            return;
        }

        free(src);
        free(dst);
        free(ref);
    }

    TEST_PASS();
}

/* Error: null dst */
static void test_null_dst(void)
{
    TEST_BEGIN("layout: null dst pointer");

    struct npu_tensor_desc d = {
        .base_addr = 0,
        .dim_n     = 1,
        .dim_h     = 2,
        .dim_w     = 2,
        .dim_c     = 1,
        .layout    = LGNPU_LAYOUT_NHWC,
        .dtype     = LGNPU_DTYPE_INT8
    };

    int8_t src[4];

    ASSERT_EQ(
        npu_tensor_convert_to_nhwc(NULL, src, &d, 4),
        LGNPU_TENSOR_ERR_NULL_PTR
    );

    TEST_PASS();
}

/* Error: null src */
static void test_null_src(void)
{
    TEST_BEGIN("layout: null src pointer");

    struct npu_tensor_desc d = {
        .base_addr = 0,
        .dim_n     = 1,
        .dim_h     = 2,
        .dim_w     = 2,
        .dim_c     = 1,
        .layout    = LGNPU_LAYOUT_NHWC,
        .dtype     = LGNPU_DTYPE_INT8
    };

    int8_t dst[4];

    ASSERT_EQ(
        npu_tensor_convert_to_nhwc(dst, NULL, &d, 4),
        LGNPU_TENSOR_ERR_NULL_PTR
    );

    TEST_PASS();
}

/* Error: buffer too small */
static void test_buf_too_small(void)
{
    TEST_BEGIN("layout: buffer too small");

    struct npu_tensor_desc d = {
        .base_addr = 0,
        .dim_n     = 1,
        .dim_h     = 4,
        .dim_w     = 4,
        .dim_c     = 3,
        .layout    = LGNPU_LAYOUT_NHWC,
        .dtype     = LGNPU_DTYPE_INT8
    };

    int8_t src[48];
    int8_t dst[48];

    /* Need 48 bytes, provide 47 */
    ASSERT_EQ(
        npu_tensor_convert_to_nhwc(dst, src, &d, 47),
        LGNPU_TENSOR_ERR_BUF_TOO_SMALL
    );

    TEST_PASS();
}

/* Single element tensor */
static void test_single_element(void)
{
    TEST_BEGIN("layout: single element 1x1x1x1 NCHW");

    struct npu_tensor_desc d = {
        .base_addr = 0,
        .dim_n     = 1,
        .dim_h     = 1,
        .dim_w     = 1,
        .dim_c     = 1,
        .layout    = LGNPU_LAYOUT_NCHW,
        .dtype     = LGNPU_DTYPE_INT8
    };

    int8_t src = 42;
    int8_t dst = 0;

    ASSERT_EQ(
        npu_tensor_convert_to_nhwc(&dst, &src, &d, 1),
        LGNPU_TENSOR_OK
    );

    ASSERT_EQ(dst, (int8_t)42);

    TEST_PASS();
}

/* 1D vector (H=1, W=N, C=1) */
static void test_1d_vector(void)
{
    TEST_BEGIN("layout: 1D vector 1x1x8x1 NCHW");

    struct npu_tensor_desc d = {
        .base_addr = 0,
        .dim_n     = 1,
        .dim_h     = 1,
        .dim_w     = 8,
        .dim_c     = 1,
        .layout    = LGNPU_LAYOUT_NCHW,
        .dtype     = LGNPU_DTYPE_INT8
    };

    int8_t src[8];
    int8_t dst[8];

    for (int i = 0; i < 8; ++i)
        src[i] = (int8_t)(i * 10);

    ASSERT_EQ(npu_tensor_convert_to_nhwc(dst, src, &d, 8), LGNPU_TENSOR_OK);
    /* C=1: NCHW and NHWC order are identical */
    ASSERT_MEM_EQ(dst, src, 8);

    TEST_PASS();
}

int main(void)
{
    SUITE_BEGIN("Layout Conversion");

    test_small_nchw_to_nhwc();
    test_nhwc_passthrough();
    test_medium_nchw_to_nhwc();
    test_random_tensors();
    test_null_dst();
    test_null_src();
    test_buf_too_small();
    test_single_element();
    test_1d_vector();

    SUITE_END();

    return SUITE_RESULT();
}
