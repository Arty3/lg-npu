/* npu_mmio.c - MMIO resource mapping and register access
 *
 * Provides register read/write and bulk buffer accessors.
 * See docs/spec/register_map.md for the full address map.
 */

#include "lgnpu_drv.h"

#include <linux/platform_device.h>
#include <linux/io.h>

u32 lgnpu_reg_read(const struct lgnpu_device* npu, const u32 offset)
{
    return readl(npu->regs + offset);
}

void lgnpu_reg_write(struct lgnpu_device* npu, const u32 offset, const u32 val)
{
    writel(val, npu->regs + offset);
}

void lgnpu_buf_write(
    struct lgnpu_device* npu,
    const u32            offset,
    const u32*           data,
    const u32            count)
{
    memcpy_toio(npu->regs + offset, data, (size_t)count * sizeof(u32));
}

void lgnpu_buf_read(
    const struct lgnpu_device* npu,
    const u32                  offset,
    u32*                       data,
    const u32                  count)
{
    memcpy_fromio(data, npu->regs + offset, (size_t)count * sizeof(u32));
}

int lgnpu_mmio_init(struct lgnpu_device* npu, struct platform_device* pdev)
{
    npu->regs = devm_platform_ioremap_resource(pdev, 0);

    if (IS_ERR(npu->regs))
    {
        dev_err(npu->dev, "failed to map MMIO region\n");
        return PTR_ERR(npu->regs);
    }

    return 0;
}
