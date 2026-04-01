/* npu_dma.c - DMA transfer operations
 *
 * Programs the 5-register DMA engine and polls for completion.
 * DMA completion also sets IRQ_STATUS.PENDING (OR'd with cmd_done);
 * the pending flag is cleared after polling to prevent stale IRQ.
 * See docs/spec/register_map.md for DMA register layout.
 */

#include "lgnpu_annotate.h"
#include "lgnpu_drv.h"
#include "npu_mmio.h"

#include <linux/delay.h>

int lgnpu_dma_transfer(
    struct lgnpu_device* npu,
    const u32            ext_addr,
    const u32            loc_addr,
    const u32            len,
    const u32            dir)
{
    u32 ctl;

    mutex_lock(&npu->cmd_lock);

    lgnpu_reg_write(npu, LGNPU_REG_DMA_EXT_ADDR, ext_addr);
    lgnpu_reg_write(npu, LGNPU_REG_DMA_LOC_ADDR, loc_addr);
    lgnpu_reg_write(npu, LGNPU_REG_DMA_LEN, len);

    ctl = LGNPU_DMA_CTRL_START;

    if (dir == LGNPU_DMA_FROM_DEVICE)
        ctl |= LGNPU_DMA_CTRL_DIR;

    lgnpu_reg_write(npu, LGNPU_REG_DMA_CTRL, ctl);

    /* Poll for DMA completion */
    for (u32 i = 0; i < LGNPU_DMA_POLL_ITERS; ++i)
    {
        if (!(lgnpu_reg_read(npu, LGNPU_REG_DMA_STATUS) & LGNPU_DMA_STATUS_BUSY))
        {
            /* DMA completion sets IRQ_STATUS.PENDING; clear it so a
             * subsequent cmd_submit does not see a stale interrupt. */
            lgnpu_reg_write(npu, LGNPU_REG_IRQ_CLEAR, LGNPU_STATUS_PENDING);
            mutex_unlock(&npu->cmd_lock);
            return 0;
        }
        udelay(LGNPU_DMA_POLL_US);
    }

    mutex_unlock(&npu->cmd_lock);
    dev_err(npu->dev, "DMA transfer timed out\n");
    return -ETIMEDOUT;
}
