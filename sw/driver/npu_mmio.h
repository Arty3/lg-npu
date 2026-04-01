/* npu_mmio.h - Inline MMIO register and buffer accessors
 *
 * Single-instruction wrappers around readl/writel and memcpy_toio/memcpy_fromio.
 * Inlined to eliminate call overhead on hot paths (DMA polling, IRQ handler, etc.).
 * See docs/spec/register_map.md for the full address map.
 */

#ifndef _NPU_MMIO_H
#define _NPU_MMIO_H

#include "lgnpu_annotate.h"
#include "lgnpu_drv.h"

#include <linux/types.h>
#include <linux/io.h>

NO_DISCARD HOT_CALL NO_NULL_ARGS ALWAYS_INLINE
static inline u32 lgnpu_reg_read(
    const struct lgnpu_device* npu,
    const u32                  offset)
{
    return readl(npu->regs + offset);
}

HOT_CALL NO_NULL_ARGS ALWAYS_INLINE
static inline void lgnpu_reg_write(
    struct lgnpu_device* npu,
    const u32            offset,
    const u32            val)
{
    writel(val, npu->regs + offset);
}

HOT_CALL NO_NULL_ARGS
static inline void lgnpu_buf_write(
    struct lgnpu_device* npu,
    const u32            offset,
    const u32*           data,
    const u32            count)
{
    memcpy_toio(npu->regs + offset, data, (size_t)count * sizeof(u32));
}

HOT_CALL NO_NULL_ARGS
static inline void lgnpu_buf_read(
    const struct lgnpu_device* npu,
    const u32                  offset,
    u32*                       data,
    const u32                  count)
{
    memcpy_fromio(data, npu->regs + offset, (size_t)count * sizeof(u32));
}

#endif /* _NPU_MMIO_H */
