/* test_command.c - Command descriptor construction tests
*
* Verifies that each npu_cmd_build_* function packs fields into the
* correct word positions per docs/spec/command_format.md.
*/

#include "../../uapi/lgnpu_cmd.h"
#include "../test_harness.h"

#include <string.h>

/* Null pointer handling */
static void test_conv_null_desc(void)
{
    TEST_BEGIN("cmd_build_conv: null desc");

    struct npu_conv_params p;
    __builtin_memset(&p, 0, sizeof(p));

    ASSERT_EQ(npu_cmd_build_conv(NULL, &p), -1);

    TEST_PASS();
}

static void test_conv_null_params(void)
{
    TEST_BEGIN("cmd_build_conv: null params");

    struct npu_cmd_desc desc;
    ASSERT_EQ(npu_cmd_build_conv(&desc, NULL), -1);

    TEST_PASS();
}

/* Conv: verify every field lands in the right word */
static void test_conv_field_packing(void)
{
    TEST_BEGIN("cmd_build_conv: field packing");

    struct npu_conv_params p = {
        .act_in_addr  = 0x1000,
        .act_out_addr = 0x2000,
        .weight_addr  = 0x3000,
        .bias_addr    = 0x4000,
        .in_h         = 28,
        .in_w         = 28,
        .in_c         = 3,
        .out_k        = 16,
        .filt_r       = 3,
        .filt_s       = 3,
        .stride_h     = 1,
        .stride_w     = 1,
        .pad_h        = 1,
        .pad_w        = 1,
        .quant_shift  = 7,
        .act_mode     = LGNPU_ACT_RELU,
    };

    struct npu_cmd_desc desc;
    ASSERT_EQ(npu_cmd_build_conv(&desc, &p), 0);

    ASSERT_EQ(desc.words[0],  (uint32_t)LGNPU_OP_CONV);
    ASSERT_EQ(desc.words[1],  0x1000u);
    ASSERT_EQ(desc.words[2],  0x2000u);
    ASSERT_EQ(desc.words[3],  0x3000u);
    ASSERT_EQ(desc.words[4],  0x4000u);
    ASSERT_EQ(desc.words[5],  28u);
    ASSERT_EQ(desc.words[6],  28u);
    ASSERT_EQ(desc.words[7],  3u);
    ASSERT_EQ(desc.words[8],  16u);
    ASSERT_EQ(desc.words[9],  3u);
    ASSERT_EQ(desc.words[10], 3u);
    ASSERT_EQ(desc.words[11], 1u);
    ASSERT_EQ(desc.words[12], 1u);
    ASSERT_EQ(desc.words[13], 1u);
    ASSERT_EQ(desc.words[14], 1u);
    /* word[15] = (quant_shift & 0x1F) | (act_mode << 5) = 7 | (1 << 5) = 39 */
    ASSERT_EQ(desc.words[15], 7u | (1u << 5));

    TEST_PASS();
}

/* Conv: descriptor is zeroed before packing */
static void test_conv_zeroes_desc(void)
{
    TEST_BEGIN("cmd_build_conv: descriptor zeroed first");

    struct npu_cmd_desc desc;
    __builtin_memset(&desc, 0xFF, sizeof(desc));

    struct npu_conv_params p;
    __builtin_memset(&p, 0, sizeof(p));
    p.act_in_addr = 0x100;

    ASSERT_EQ(npu_cmd_build_conv(&desc, &p), 0);

    /* Unused words should be zero */
    for (uint32_t i = 2; i < LGNPU_CMD_WORDS - 1; ++i)
        if (i != 1) ASSERT_EQ(desc.words[i], 0u);

    TEST_PASS();
}

/* GEMM */
static void test_gemm_field_packing(void)
{
    TEST_BEGIN("cmd_build_gemm: field packing");

    struct npu_gemm_params p = {
        .a_addr      = 0x100,
        .c_addr      = 0x200,
        .b_addr      = 0x300,
        .bias_addr   = 0x400,
        .M           = 64,
        .N           = 32,
        .K           = 16,
        .quant_shift = 4,
        .act_mode    = LGNPU_ACT_LEAKY_RELU,
    };

    struct npu_cmd_desc desc;
    ASSERT_EQ(npu_cmd_build_gemm(&desc, &p), 0);

    ASSERT_EQ(desc.words[0], (uint32_t)LGNPU_OP_GEMM);
    ASSERT_EQ(desc.words[1], 0x100u);
    ASSERT_EQ(desc.words[2], 0x200u);
    ASSERT_EQ(desc.words[3], 0x300u);
    ASSERT_EQ(desc.words[4], 0x400u);
    ASSERT_EQ(desc.words[5], 64u);
    ASSERT_EQ(desc.words[6], 32u);
    ASSERT_EQ(desc.words[7], 16u);
    ASSERT_EQ(desc.words[15], 4u | (2u << 5));

    TEST_PASS();
}

static void test_gemm_null(void)
{
    TEST_BEGIN("cmd_build_gemm: null pointers");

    struct npu_gemm_params p;
    __builtin_memset(&p, 0, sizeof(p));

    ASSERT_EQ(npu_cmd_build_gemm(NULL, &p), -1);
    struct npu_cmd_desc desc;
    ASSERT_EQ(npu_cmd_build_gemm(&desc, NULL), -1);

    TEST_PASS();
}

/* Softmax */
static void test_softmax_field_packing(void)
{
    TEST_BEGIN("cmd_build_softmax: field packing");

    struct npu_softmax_params p = {
        .in_addr  = 0x500,
        .out_addr = 0x600,
        .num_rows = 8,
        .row_len  = 256,
    };

    struct npu_cmd_desc desc;
    ASSERT_EQ(npu_cmd_build_softmax(&desc, &p), 0);

    ASSERT_EQ(desc.words[0], (uint32_t)LGNPU_OP_SOFTMAX);
    ASSERT_EQ(desc.words[1], 0x500u);
    ASSERT_EQ(desc.words[2], 0x600u);
    ASSERT_EQ(desc.words[5], 8u);
    ASSERT_EQ(desc.words[6], 256u);

    TEST_PASS();
}

static void test_softmax_null(void)
{
    TEST_BEGIN("cmd_build_softmax: null pointers");

    struct npu_softmax_params p;
    __builtin_memset(&p, 0, sizeof(p));

    ASSERT_EQ(npu_cmd_build_softmax(NULL, &p), -1);
    struct npu_cmd_desc desc;
    ASSERT_EQ(npu_cmd_build_softmax(&desc, NULL), -1);

    TEST_PASS();
}

/* Vec */
static void test_vec_field_packing(void)
{
    TEST_BEGIN("cmd_build_vec: field packing");

    struct npu_vec_params p = {
        .a_addr      = 0x100,
        .out_addr    = 0x200,
        .b_addr      = 0x300,
        .length      = 128,
        .vec_op      = LGNPU_VEC_MUL,
        .quant_shift = 3,
        .act_mode    = LGNPU_ACT_RELU,
    };

    struct npu_cmd_desc desc;
    ASSERT_EQ(npu_cmd_build_vec(&desc, &p), 0);

    ASSERT_EQ(desc.words[0], (uint32_t)LGNPU_OP_VEC);
    ASSERT_EQ(desc.words[1], 0x100u);
    ASSERT_EQ(desc.words[2], 0x200u);
    ASSERT_EQ(desc.words[3], 0x300u);
    ASSERT_EQ(desc.words[5], 128u);
    ASSERT_EQ(desc.words[6], (uint32_t)LGNPU_VEC_MUL);
    ASSERT_EQ(desc.words[15], 3u | (1u << 5));

    TEST_PASS();
}

static void test_vec_null(void)
{
    TEST_BEGIN("cmd_build_vec: null pointers");

    struct npu_vec_params p;
    __builtin_memset(&p, 0, sizeof(p));

    ASSERT_EQ(npu_cmd_build_vec(NULL, &p), -1);
    struct npu_cmd_desc desc;
    ASSERT_EQ(npu_cmd_build_vec(&desc, NULL), -1);

    TEST_PASS();
}

/* Lnorm */
static void test_lnorm_field_packing(void)
{
    TEST_BEGIN("cmd_build_lnorm: field packing");

    struct npu_lnorm_params p = {
        .in_addr    = 0x100,
        .out_addr   = 0x200,
        .num_rows   = 4,
        .row_len    = 64,
        .norm_shift = 10,
    };

    struct npu_cmd_desc desc;
    ASSERT_EQ(npu_cmd_build_lnorm(&desc, &p), 0);

    ASSERT_EQ(desc.words[0],  (uint32_t)LGNPU_OP_LNORM);
    ASSERT_EQ(desc.words[1],  0x100u);
    ASSERT_EQ(desc.words[2],  0x200u);
    ASSERT_EQ(desc.words[5],  4u);
    ASSERT_EQ(desc.words[6],  64u);
    ASSERT_EQ(desc.words[15], 10u);

    TEST_PASS();
}

static void test_lnorm_null(void)
{
    TEST_BEGIN("cmd_build_lnorm: null pointers");

    struct npu_lnorm_params p;
    __builtin_memset(&p, 0, sizeof(p));

    ASSERT_EQ(npu_cmd_build_lnorm(NULL, &p), -1);
    struct npu_cmd_desc desc;
    ASSERT_EQ(npu_cmd_build_lnorm(&desc, NULL), -1);

    TEST_PASS();
}

/* Pool */
static void test_pool_field_packing(void)
{
    TEST_BEGIN("cmd_build_pool: field packing");

    struct npu_pool_params p = {
        .in_addr   = 0x100,
        .out_addr  = 0x200,
        .in_h      = 8,
        .in_w      = 8,
        .in_c      = 16,
        .pool_r    = 2,
        .pool_s    = 2,
        .stride_h  = 2,
        .stride_w  = 2,
        .pad_h     = 0,
        .pad_w     = 0,
        .pool_mode = LGNPU_POOL_AVG,
    };

    struct npu_cmd_desc desc;
    ASSERT_EQ(npu_cmd_build_pool(&desc, &p), 0);

    ASSERT_EQ(desc.words[0],  (uint32_t)LGNPU_OP_POOL);
    ASSERT_EQ(desc.words[1],  0x100u);
    ASSERT_EQ(desc.words[2],  0x200u);
    ASSERT_EQ(desc.words[5],  8u);
    ASSERT_EQ(desc.words[6],  8u);
    ASSERT_EQ(desc.words[7],  16u);
    ASSERT_EQ(desc.words[9],  2u);
    ASSERT_EQ(desc.words[10], 2u);
    ASSERT_EQ(desc.words[11], 2u);
    ASSERT_EQ(desc.words[12], 2u);
    ASSERT_EQ(desc.words[13], 0u);
    ASSERT_EQ(desc.words[14], 0u);
    ASSERT_EQ(desc.words[15], 1u);  /* AVG = 1 */

    TEST_PASS();
}

static void test_pool_null(void)
{
    TEST_BEGIN("cmd_build_pool: null pointers");

    struct npu_pool_params p;
    __builtin_memset(&p, 0, sizeof(p));

    ASSERT_EQ(npu_cmd_build_pool(NULL, &p), -1);
    struct npu_cmd_desc desc;
    ASSERT_EQ(npu_cmd_build_pool(&desc, NULL), -1);

    TEST_PASS();
}

/* Quant shift masking (top bits must be stripped) */
static void test_quant_shift_mask(void)
{
    TEST_BEGIN("cmd_build_conv: quant_shift masked to 5 bits");

    struct npu_conv_params p;
    __builtin_memset(&p, 0, sizeof(p));

    p.quant_shift = 0xFF;  /* only low 5 bits = 31 */
    p.act_mode    = 0;

    struct npu_cmd_desc desc;
    ASSERT_EQ(npu_cmd_build_conv(&desc, &p), 0);
    ASSERT_EQ(desc.words[15], 31u);

    TEST_PASS();
}

/* Act mode masking */
static void test_act_mode_mask(void)
{
    TEST_BEGIN("cmd_build_conv: act_mode masked to 2 bits");

    struct npu_conv_params p;
    __builtin_memset(&p, 0, sizeof(p));

    p.quant_shift = 0;
    p.act_mode    = 0xFF;  /* only low 2 bits = 3 */

    struct npu_cmd_desc desc;
    ASSERT_EQ(npu_cmd_build_conv(&desc, &p), 0);
    ASSERT_EQ(desc.words[15], 3u << 5);

    TEST_PASS();
}

int main(void)
{
    SUITE_BEGIN("Command Construction");

    test_conv_null_desc();
    test_conv_null_params();
    test_conv_field_packing();
    test_conv_zeroes_desc();

    test_gemm_field_packing();
    test_gemm_null();

    test_softmax_field_packing();
    test_softmax_null();

    test_vec_field_packing();
    test_vec_null();

    test_lnorm_field_packing();
    test_lnorm_null();

    test_pool_field_packing();
    test_pool_null();

    test_quant_shift_mask();
    test_act_mode_mask();

    SUITE_END();

    return SUITE_RESULT();
}
