/* npu_queue.c - Command queue submission
 *
 * Writes a 16-word (64-byte) descriptor to CMD_QUEUE_BASE, rings the
 * doorbell, and waits for the IRQ-driven completion signal.
 * See docs/spec/command_format.md for the descriptor layout.
 */

#include "lgnpu_annotate.h"
#include "lgnpu_drv.h"
#include "npu_mmio.h"

#include <linux/jiffies.h>

int lgnpu_cmd_submit(struct lgnpu_device* npu, const u32* words)
{
    mutex_lock(&npu->cmd_lock);

    const u32 status = lgnpu_reg_read(npu, LGNPU_REG_STATUS);

    /* Reject if hardware queue is full */
    if (UNLIKELY(status & LGNPU_STATUS_QUEUE_FULL))
    {
        mutex_unlock(&npu->cmd_lock);
        return -EBUSY;
    }

    /* Write 16-word descriptor into the command queue window */
    lgnpu_buf_write(npu, LGNPU_CMD_QUEUE_BASE, words, LGNPU_CMD_WORDS);

    /* Arm completion, enable IRQ, ring doorbell */
    reinit_completion(&npu->cmd_done);
    lgnpu_irq_enable(npu);
    lgnpu_reg_write(npu, LGNPU_REG_DOORBELL, 1);

    /* Wait for command completion or timeout */
    const long timeout = wait_for_completion_interruptible_timeout(
        &npu->cmd_done,
        msecs_to_jiffies(LGNPU_CMD_TIMEOUT_MS)
    );

    lgnpu_irq_disable(npu);
    mutex_unlock(&npu->cmd_lock);

    if (UNLIKELY(!timeout))
    {
        dev_err(npu->dev, "command submission timed out\n");
        return -ETIMEDOUT;
    }

    if (UNLIKELY(timeout < 0))
        return (int)timeout;

    return 0;
}
