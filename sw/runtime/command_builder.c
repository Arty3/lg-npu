/* command_builder.c - Pack typed parameters into 16-word command descriptors
*
* Word positions match docs/spec/command_format.md.
*/

#include "../shared/annotations.h"
#include "../uapi/lgnpu_cmd.h"

#include <string.h>

NO_NULL_ARGS ALWAYS_INLINE
static void cmd_clear(struct npu_cmd_desc* desc)
{
    __builtin_memset(desc, 0, sizeof(*desc));
}

int npu_cmd_build_conv(
    struct npu_cmd_desc*          desc,
    const struct npu_conv_params* p)
{
    if (UNLIKELY(!desc || !p))
        return -1;

    cmd_clear(desc);

    desc->words[0]  = (uint32_t)LGNPU_OP_CONV;
    desc->words[1]  = p->act_in_addr;
    desc->words[2]  = p->act_out_addr;
    desc->words[3]  = p->weight_addr;
    desc->words[4]  = p->bias_addr;
    desc->words[5]  = p->in_h;
    desc->words[6]  = p->in_w;
    desc->words[7]  = p->in_c;
    desc->words[8]  = p->out_k;
    desc->words[9]  = p->filt_r;
    desc->words[10] = p->filt_s;
    desc->words[11] = p->stride_h;
    desc->words[12] = p->stride_w;
    desc->words[13] = p->pad_h;
    desc->words[14] = p->pad_w;
    desc->words[15] = (p->quant_shift & 0x1FU)
                    | (((uint32_t)p->act_mode & 0x3U) << 5);

    return 0;
}

int npu_cmd_build_gemm(
    struct npu_cmd_desc*          desc,
    const struct npu_gemm_params* p)
{
    if (UNLIKELY(!desc || !p))
        return -1;

    cmd_clear(desc);

    desc->words[0]  = (uint32_t)LGNPU_OP_GEMM;
    desc->words[1]  = p->a_addr;
    desc->words[2]  = p->c_addr;
    desc->words[3]  = p->b_addr;
    desc->words[4]  = p->bias_addr;
    desc->words[5]  = p->M;
    desc->words[6]  = p->N;
    desc->words[7]  = p->K;
    desc->words[15] = (p->quant_shift & 0x1FU)
                    | (((uint32_t)p->act_mode & 0x3U) << 5);

    return 0;
}

int npu_cmd_build_softmax(
    struct npu_cmd_desc*             desc,
    const struct npu_softmax_params* p)
{
    if (UNLIKELY(!desc || !p))
        return -1;

    cmd_clear(desc);

    desc->words[0] = (uint32_t)LGNPU_OP_SOFTMAX;
    desc->words[1] = p->in_addr;
    desc->words[2] = p->out_addr;
    desc->words[5] = p->num_rows;
    desc->words[6] = p->row_len;

    return 0;
}

int npu_cmd_build_vec(
    struct npu_cmd_desc*         desc,
    const struct npu_vec_params* p)
{
    if (UNLIKELY(!desc || !p))
        return -1;

    cmd_clear(desc);

    desc->words[0]  = (uint32_t)LGNPU_OP_VEC;
    desc->words[1]  = p->a_addr;
    desc->words[2]  = p->out_addr;
    desc->words[3]  = p->b_addr;
    desc->words[5]  = p->length;
    desc->words[6]  = p->vec_op;
    desc->words[15] = (p->quant_shift & 0x1FU)
                    | (((uint32_t)p->act_mode & 0x3U) << 5);

    return 0;
}

int npu_cmd_build_lnorm(
    struct npu_cmd_desc*           desc,
    const struct npu_lnorm_params* p)
{
    if (UNLIKELY(!desc || !p))
        return -1;

    cmd_clear(desc);

    desc->words[0]  = (uint32_t)LGNPU_OP_LNORM;
    desc->words[1]  = p->in_addr;
    desc->words[2]  = p->out_addr;
    desc->words[5]  = p->num_rows;
    desc->words[6]  = p->row_len;
    desc->words[15] = p->norm_shift & 0x1FU;

    return 0;
}

int npu_cmd_build_pool(
    struct npu_cmd_desc*          desc,
    const struct npu_pool_params* p)
{
    if (UNLIKELY(!desc || !p))
        return -1;

    cmd_clear(desc);

    desc->words[0]  = (uint32_t)LGNPU_OP_POOL;
    desc->words[1]  = p->in_addr;
    desc->words[2]  = p->out_addr;
    desc->words[5]  = p->in_h;
    desc->words[6]  = p->in_w;
    desc->words[7]  = p->in_c;
    desc->words[9]  = p->pool_r;
    desc->words[10] = p->pool_s;
    desc->words[11] = p->stride_h;
    desc->words[12] = p->stride_w;
    desc->words[13] = p->pad_h;
    desc->words[14] = p->pad_w;
    desc->words[15] = p->pool_mode & 0x1U;

    return 0;
}
