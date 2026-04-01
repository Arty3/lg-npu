/* npu_irq.c - Interrupt handling
 *
 * Single level-sensitive IRQ line, latched by cmd_done | error.
 * See docs/spec/interrupts.md for the full interrupt lifecycle.
 */

#include "lgnpu_annotate.h"
#include "lgnpu_drv.h"
#include "npu_mmio.h"

#include <linux/platform_device.h>
#include <linux/interrupt.h>

HOT_CALL NO_INLINE
static irqreturn_t lgnpu_irq_handler(int irq, void* data)
{
    struct lgnpu_device* npu = data;

    spin_lock(&npu->irq_lock);

    const u32 status = lgnpu_reg_read(npu, LGNPU_REG_IRQ_STATUS);

    if (UNLIKELY(!(status & LGNPU_IRQ_STATUS_PENDING)))
    {
        spin_unlock(&npu->irq_lock);
        return IRQ_NONE;
    }

    lgnpu_reg_write(npu, LGNPU_REG_IRQ_CLEAR, LGNPU_IRQ_STATUS_PENDING);
    spin_unlock(&npu->irq_lock);

    complete(&npu->cmd_done);
    return IRQ_HANDLED;
}

int lgnpu_irq_init(struct lgnpu_device* npu, struct platform_device* pdev)
{
    int irq;
    int ret;

    irq = platform_get_irq(pdev, 0);

    if (UNLIKELY(irq < 0))
        return irq;

    ret = devm_request_irq(
        npu->dev,
        (unsigned int)irq,
        lgnpu_irq_handler,
        IRQF_SHARED,
        LGNPU_DRV_NAME,
        npu
    );

    if (UNLIKELY(ret))
    {
        dev_err(npu->dev, "failed to request IRQ %d\n", irq);
        return ret;
    }

    npu->irq = irq;
    return 0;
}

void lgnpu_irq_enable(struct lgnpu_device* npu)
{
    unsigned long flags;

    spin_lock_irqsave(&npu->irq_lock, flags);
    lgnpu_reg_write(npu, LGNPU_REG_IRQ_ENABLE, 1);
    spin_unlock_irqrestore(&npu->irq_lock, flags);
}

void lgnpu_irq_disable(struct lgnpu_device* npu)
{
    unsigned long flags;

    spin_lock_irqsave(&npu->irq_lock, flags);
    lgnpu_reg_write(npu, LGNPU_REG_IRQ_ENABLE, 0);
    spin_unlock_irqrestore(&npu->irq_lock, flags);
}
