/* npu_reset.c - Hardware reset and initialization
 *
 * Soft reset is a self-clearing single-cycle pulse.
 * Boot sequence: reset -> enable -> read FEATURE_ID.
 * See docs/spec/reset_boot_flow.md for the full sequence.
 */

#include "lgnpu_annotate.h"
#include "lgnpu_drv.h"
#include "npu_mmio.h"

#include <linux/delay.h>

int lgnpu_hw_reset(struct lgnpu_device* npu)
{
    lgnpu_reg_write(npu, LGNPU_REG_CTRL, LGNPU_CTRL_SOFT_RESET);

    /* Self-clearing pulse; small delay for propagation */
    udelay(1);
    return 0;
}

int lgnpu_hw_enable(struct lgnpu_device* npu)
{
    lgnpu_reg_write(npu, LGNPU_REG_CTRL, LGNPU_CTRL_ENABLE);
    return 0;
}

int lgnpu_hw_init(struct lgnpu_device* npu)
{
    int ret;

    ret = lgnpu_hw_reset(npu);

    if (UNLIKELY(ret))
        return ret;

    ret = lgnpu_hw_enable(npu);

    if (UNLIKELY(ret))
        return ret;

    npu->feature_id = lgnpu_reg_read(npu, LGNPU_REG_FEATURE_ID);

    if (UNLIKELY(!npu->feature_id))
    {
        dev_err(npu->dev, "FEATURE_ID reads as zero, device not responding\n");
        return -ENODEV;
    }

    return 0;
}
