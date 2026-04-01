/* npu_mmio.c - MMIO resource mapping
 *
 * Platform resource initialization.  Register and buffer accessors are
 * inlined via npu_mmio.h.
 * See docs/spec/register_map.md for the full address map.
 */

#include "lgnpu_annotate.h"
#include "lgnpu_drv.h"

#include <linux/platform_device.h>

int lgnpu_mmio_init(struct lgnpu_device* npu, struct platform_device* pdev)
{
    npu->regs = devm_platform_ioremap_resource(pdev, 0);

    if (UNLIKELY(IS_ERR(npu->regs)))
    {
        dev_err(npu->dev, "failed to map MMIO region\n");
        return PTR_ERR(npu->regs);
    }

    return 0;
}
