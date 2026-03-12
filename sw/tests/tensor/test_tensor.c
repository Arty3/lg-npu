/* test_tensor.c - Tensor descriptor validation tests */

#include "../../uapi/lgnpu_tensor.h"
#include "../test_harness.h"

static void test_null_ptr(void)
{
    TEST_BEGIN("validate: null pointer");

    ASSERT_EQ(
        npu_tensor_validate(NULL),
        LGNPU_TENSOR_ERR_NULL_PTR
    );

    TEST_PASS();
}

static void test_zero_dim_n(void)
{
    TEST_BEGIN("validate: zero dim_n");

    struct npu_tensor_desc d = {
        .base_addr = 0,
        .dim_n     = 0,
        .dim_h     = 8,
        .dim_w     = 8,
        .dim_c     = 3,
        .layout    = LGNPU_LAYOUT_NHWC,
        .dtype     = LGNPU_DTYPE_INT8
    };

    ASSERT_EQ(
        npu_tensor_validate(&d),
        LGNPU_TENSOR_ERR_ZERO_DIM
    );

    TEST_PASS();
}

static void test_zero_dim_h(void)
{
    TEST_BEGIN("validate: zero dim_h");

    struct npu_tensor_desc d = {
        .base_addr = 0,
        .dim_n     = 1,
        .dim_h     = 0,
        .dim_w     = 8,
        .dim_c     = 3,
        .layout    = LGNPU_LAYOUT_NHWC,
        .dtype     = LGNPU_DTYPE_INT8
    };

    ASSERT_EQ(
        npu_tensor_validate(&d),
        LGNPU_TENSOR_ERR_ZERO_DIM
    );

    TEST_PASS();
}

static void test_zero_dim_w(void)
{
    TEST_BEGIN("validate: zero dim_w");

    struct npu_tensor_desc d = {
        .base_addr = 0,
        .dim_n     = 1,
        .dim_h     = 8,
        .dim_w     = 0,
        .dim_c     = 3,
        .layout    = LGNPU_LAYOUT_NHWC,
        .dtype     = LGNPU_DTYPE_INT8
    };

    ASSERT_EQ(
        npu_tensor_validate(&d),
        LGNPU_TENSOR_ERR_ZERO_DIM
    );

    TEST_PASS();
}

static void test_zero_dim_c(void)
{
    TEST_BEGIN("validate: zero dim_c");

    struct npu_tensor_desc d = {
        .base_addr = 0,
        .dim_n     = 1,
        .dim_h     = 8,
        .dim_w     = 8,
        .dim_c     = 0,
        .layout    = LGNPU_LAYOUT_NHWC,
        .dtype     = LGNPU_DTYPE_INT8
    };

    ASSERT_EQ(
        npu_tensor_validate(&d),
        LGNPU_TENSOR_ERR_ZERO_DIM
    );

    TEST_PASS();
}

static void test_batch_unsupported(void)
{
    TEST_BEGIN("validate: batch > 1 unsupported");

    struct npu_tensor_desc d = {
        .base_addr = 0,
        .dim_n     = 2,
        .dim_h     = 8,
        .dim_w     = 8,
        .dim_c     = 3,
        .layout    = LGNPU_LAYOUT_NHWC,
        .dtype     = LGNPU_DTYPE_INT8
    };

    ASSERT_EQ(
        npu_tensor_validate(&d),
        LGNPU_TENSOR_ERR_BATCH_UNSUP
    );

    TEST_PASS();
}

static void test_large_batch(void)
{
    TEST_BEGIN("validate: batch = 255 unsupported");

    struct npu_tensor_desc d = {
        .base_addr = 0,
        .dim_n     = 255,
        .dim_h     = 1,
        .dim_w     = 1,
        .dim_c     = 1,
        .layout    = LGNPU_LAYOUT_NHWC,
        .dtype     = LGNPU_DTYPE_INT8
    };

    ASSERT_EQ(
        npu_tensor_validate(&d),
        LGNPU_TENSOR_ERR_BATCH_UNSUP
    );

    TEST_PASS();
}

static void test_invalid_layout(void)
{
    TEST_BEGIN("validate: invalid layout enum");

    struct npu_tensor_desc d = {
        .base_addr = 0,
        .dim_n     = 1,
        .dim_h     = 8,
        .dim_w     = 8,
        .dim_c     = 3,
        .layout    = LGNPU_LAYOUT_COUNT,
        .dtype     = LGNPU_DTYPE_INT8
    };

    ASSERT_EQ(
        npu_tensor_validate(&d),
        LGNPU_TENSOR_ERR_LAYOUT_UNKNOWN
    );

    TEST_PASS();
}

static void test_layout_out_of_range(void)
{
    TEST_BEGIN("validate: layout = 99");

    struct npu_tensor_desc d = {
        .base_addr = 0,
        .dim_n     = 1,
        .dim_h     = 4,
        .dim_w     = 4,
        .dim_c     = 1,
        .layout    = (enum npu_tensor_layout)99,
        .dtype     = LGNPU_DTYPE_INT8
    };

    ASSERT_EQ(
        npu_tensor_validate(&d),
        LGNPU_TENSOR_ERR_LAYOUT_UNKNOWN
    );

    TEST_PASS();
}

static void test_overflow_h_times_w(void)
{
    TEST_BEGIN("validate: overflow H*W");

    struct npu_tensor_desc d = {
        .base_addr = 0,
        .dim_n     = 1,
        .dim_h     = 65535,
        .dim_w     = 65535,
        .dim_c     = 2,
        .layout    = LGNPU_LAYOUT_NHWC,
        .dtype     = LGNPU_DTYPE_INT8
    };

    ASSERT_EQ(
        npu_tensor_validate(&d),
        LGNPU_TENSOR_ERR_OVERFLOW
    );

    TEST_PASS();
}

static void test_overflow_hw_times_c(void)
{
    TEST_BEGIN("validate: overflow H*W*C");

    /* H*W = 65535*256 = 16,776,960 which fits uint32.
     * H*W*C = 16,776,960 * 512 = 8,589,803,520 which overflows uint32. */
    struct npu_tensor_desc d = {
        .base_addr = 0,
        .dim_n     = 1,
        .dim_h     = 65535,
        .dim_w     = 256,
        .dim_c     = 512,
        .layout    = LGNPU_LAYOUT_NHWC,
        .dtype     = LGNPU_DTYPE_INT8
    };

    ASSERT_EQ(
        npu_tensor_validate(&d),
        LGNPU_TENSOR_ERR_OVERFLOW
    );

    TEST_PASS();
}

static void test_valid_small(void)
{
    TEST_BEGIN("validate: valid small tensor 1x4x4x3 NHWC");

    struct npu_tensor_desc d = {
        .base_addr = 0x100,
        .dim_n     = 1,
        .dim_h     = 4,
        .dim_w     = 4,
        .dim_c     = 3,
        .layout    = LGNPU_LAYOUT_NHWC,
        .dtype     = LGNPU_DTYPE_INT8
    };

    ASSERT_EQ(npu_tensor_validate(&d), LGNPU_TENSOR_OK);

    TEST_PASS();
}

static void test_valid_nchw(void)
{
    TEST_BEGIN("validate: valid tensor 1x32x32x16 NCHW");

    struct npu_tensor_desc d = {
        .base_addr = 0,
        .dim_n     = 1,
        .dim_h     = 32,
        .dim_w     = 32,
        .dim_c     = 16,
        .layout    = LGNPU_LAYOUT_NCHW,
        .dtype     = LGNPU_DTYPE_INT8
    };

    ASSERT_EQ(npu_tensor_validate(&d), LGNPU_TENSOR_OK);

    TEST_PASS();
}

static void test_valid_1x1x1x1(void)
{
    TEST_BEGIN("validate: minimal valid 1x1x1x1");

    struct npu_tensor_desc d = {
        .base_addr = 0,
        .dim_n     = 1,
        .dim_h     = 1,
        .dim_w     = 1,
        .dim_c     = 1,
        .layout    = LGNPU_LAYOUT_NHWC,
        .dtype     = LGNPU_DTYPE_INT8
    };

    ASSERT_EQ(npu_tensor_validate(&d), LGNPU_TENSOR_OK);

    TEST_PASS();
}

static void test_element_count(void)
{
    TEST_BEGIN("element_count: 1x4x4x3 = 48");

    struct npu_tensor_desc d = {
        .base_addr = 0,
        .dim_n     = 1,
        .dim_h     = 4,
        .dim_w     = 4,
        .dim_c     = 3,
        .layout    = LGNPU_LAYOUT_NHWC,
        .dtype     = LGNPU_DTYPE_INT8
    };

    ASSERT_EQ(npu_tensor_element_count(&d), 48u);

    TEST_PASS();
}

static void test_byte_size(void)
{
    TEST_BEGIN("byte_size: 1x8x8x16 = 1024");

    struct npu_tensor_desc d = {
        .base_addr = 0,
        .dim_n     = 1,
        .dim_h     = 8,
        .dim_w     = 8,
        .dim_c     = 16,
        .layout    = LGNPU_LAYOUT_NHWC,
        .dtype     = LGNPU_DTYPE_INT8
    };

    ASSERT_EQ(npu_tensor_byte_size(&d), 1024u);

    TEST_PASS();
}

static void test_nhwc_offset(void)
{
    TEST_BEGIN("nhwc_offset: [0][1][2][1] in 1x4x4x3 base=0x100");

    struct npu_tensor_desc d = {
        .base_addr = 0x100,
        .dim_n     = 1,
        .dim_h     = 4,
        .dim_w     = 4,
        .dim_c     = 3,
        .layout    = LGNPU_LAYOUT_NHWC,
        .dtype     = LGNPU_DTYPE_INT8
    };

    /* offset = 0x100 + ((0*4+1)*4+2)*3 + 1 = 0x100 + 19 = 0x113 */
    ASSERT_EQ(npu_tensor_nhwc_offset(&d, 0, 1, 2, 1), (size_t)0x113);

    TEST_PASS();
}

static void test_is_canonical(void)
{
    TEST_BEGIN("is_canonical: NHWC=1, NCHW=0");

    ASSERT_TRUE(npu_tensor_is_canonical(LGNPU_LAYOUT_NHWC));
    ASSERT_TRUE(!npu_tensor_is_canonical(LGNPU_LAYOUT_NCHW));

    TEST_PASS();
}

int main(void)
{
    SUITE_BEGIN("Tensor Validation");

    test_null_ptr();
    test_zero_dim_n();
    test_zero_dim_h();
    test_zero_dim_w();
    test_zero_dim_c();
    test_batch_unsupported();
    test_large_batch();
    test_invalid_layout();
    test_layout_out_of_range();
    test_overflow_h_times_w();
    test_overflow_hw_times_c();
    test_valid_small();
    test_valid_nchw();
    test_valid_1x1x1x1();
    test_element_count();
    test_byte_size();
    test_nhwc_offset();
    test_is_canonical();

    SUITE_END();

    return SUITE_RESULT();
}
