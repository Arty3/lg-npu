/* mmio_helpers.h - NPU device MMIO raw operations (single 32-bit word)
 * See docs/arch/programming_model.md for the interaction sequence.
 */

#ifndef _MMIO_HELPERS_H
#define _MMIO_HELPERS_H

#include "../shared/annotations.h"
#include "../uapi/lgnpu_ioctl.h"

#include <stdint.h>

/* Do NOT mark as pure call, it fights volatile base addr for dev. */

NO_DISCARD ALWAYS_INLINE NO_NULL_ARGS
static uint32_t npu_mmio_read(const struct npu_device* dev, uint32_t offset)
{
    return dev->base[offset / sizeof(uint32_t)];
}

ALWAYS_INLINE NO_NULL_ARGS
static void npu_mmio_write(struct npu_device* dev, uint32_t offset, uint32_t value)
{
    dev->base[offset / sizeof(uint32_t)] = value;
}

#endif /* _MMIO_HELPERS_H */
