/* npu_cmd.h - NPU command opcodes, descriptor layout, and builder API
 *
 * Mirrors include/pkg/npu_cmd_pkg.sv. Keep both files in sync.
 */

#ifndef _LGNPU_CMD_H
#define _LGNPU_CMD_H

#include "../shared/annotations.h"
#include "lgnpu_regs.h"

#include <stdint.h>

/* Number of 32-bit words per command descriptor. */
#define LGNPU_CMD_WORDS (LGNPU_CMD_ENTRY_BYTES / sizeof(uint32_t))

/* Command opcodes (must match npu_cmd_pkg::opcode_e) */
enum npu_opcode
{
	LGNPU_OP_CONV    = 0x1,
	LGNPU_OP_GEMM    = 0x2,
	LGNPU_OP_SOFTMAX = 0x3,
	LGNPU_OP_VEC     = 0x4,
	LGNPU_OP_LNORM   = 0x5,
	LGNPU_OP_POOL    = 0x6,
};

/* Activation-function selector (must match npu_types_pkg::act_mode_e) */
enum npu_act_mode
{
	LGNPU_ACT_NONE       = 0,
	LGNPU_ACT_RELU       = 1,
	LGNPU_ACT_LEAKY_RELU = 2,
};

/* Vec sub-operation selector */
enum npu_vec_op
{
	LGNPU_VEC_ADD = 0,
	LGNPU_VEC_MUL = 1,
};

/* Pooling mode selector */
enum npu_pool_mode
{
	LGNPU_POOL_MAX = 0,
	LGNPU_POOL_AVG = 1,
};

/* 16-word (64-byte) command descriptor written to CMD_QUEUE_BASE.
 * Field positions match the hardware packing in npu_cmd_pkg.sv. */
struct npu_cmd_desc
{
	uint32_t words[LGNPU_CMD_WORDS];
};

STATIC_ASSERT(
    sizeof(struct npu_cmd_desc) == LGNPU_CMD_ENTRY_BYTES,
    "npu_cmd_desc must be 64 bytes"
);

/* Per-opcode parameter structs.
 * These carry typed fields that the command builder packs into the
 * 16-word descriptor. */

struct npu_conv_params
{
	uint16_t act_in_addr;
	uint16_t act_out_addr;
	uint16_t weight_addr;
	uint16_t bias_addr;
	uint16_t in_h;
	uint16_t in_w;
	uint16_t in_c;
	uint16_t out_k;
	uint16_t filt_r;
	uint16_t filt_s;
	uint16_t stride_h;
	uint16_t stride_w;
	uint16_t pad_h;
	uint16_t pad_w;
	uint8_t  quant_shift;
	uint8_t  act_mode;
};

struct npu_gemm_params
{
	uint16_t a_addr;
	uint16_t c_addr;
	uint16_t b_addr;
	uint16_t bias_addr;
	uint16_t M;
	uint16_t N;
	uint16_t K;
	uint8_t  quant_shift;
	uint8_t  act_mode;
};

struct npu_softmax_params
{
	uint16_t in_addr;
	uint16_t out_addr;
	uint16_t num_rows;
	uint16_t row_len;
};

struct npu_vec_params
{
	uint16_t a_addr;
	uint16_t out_addr;
	uint16_t b_addr;
	uint16_t length;
	uint8_t  vec_op;
	uint8_t  quant_shift;
	uint8_t  act_mode;
};

struct npu_lnorm_params
{
	uint16_t in_addr;
	uint16_t out_addr;
	uint16_t num_rows;
	uint16_t row_len;
	uint8_t  norm_shift;
};

struct npu_pool_params
{
	uint16_t in_addr;
	uint16_t out_addr;
	uint16_t in_h;
	uint16_t in_w;
	uint16_t in_c;
	uint16_t pool_r;
	uint16_t pool_s;
	uint16_t stride_h;
	uint16_t stride_w;
	uint16_t pad_h;
	uint16_t pad_w;
	uint8_t  pool_mode;
};

/* Command builder functions.
 * Each zeroes the descriptor, sets the opcode, and packs the
 * parameter fields into the correct word positions. */

NO_DISCARD API_CALL
int npu_cmd_build_conv(
	struct npu_cmd_desc*          desc,
	const struct npu_conv_params* p
);

NO_DISCARD API_CALL
int npu_cmd_build_gemm(
	struct npu_cmd_desc*          desc,
	const struct npu_gemm_params* p
);

NO_DISCARD API_CALL
int npu_cmd_build_softmax(
	struct npu_cmd_desc*             desc,
	const struct npu_softmax_params* p
);

NO_DISCARD API_CALL
int npu_cmd_build_vec(
	struct npu_cmd_desc*         desc,
	const struct npu_vec_params* p
);

NO_DISCARD API_CALL
int npu_cmd_build_lnorm(
	struct npu_cmd_desc*           desc,
	const struct npu_lnorm_params* p
);

NO_DISCARD API_CALL
int npu_cmd_build_pool(
	struct npu_cmd_desc*          desc,
	const struct npu_pool_params* p
);

#endif /* _LGNPU_CMD_H */
