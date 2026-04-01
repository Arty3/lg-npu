/* lgnpu_drv.h - LG NPU driver internal header
 *
 * Private device structure, register accessors, and sub-module declarations.
 * See docs/spec/register_map.md and docs/spec/interrupts.md.
 */

#ifndef _LGNPU_DRV_H
#define _LGNPU_DRV_H

#include "lgnpu_annotate.h"

#include <linux/miscdevice.h>
#include <linux/completion.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/bits.h>
#include <linux/io.h>

struct platform_device;

/* Register offsets (mirrors sw/uapi/lgnpu_regs.h) */
#define LGNPU_REG_CTRL          0x00000U
#define LGNPU_REG_STATUS        0x00004U
#define LGNPU_REG_DOORBELL      0x00008U
#define LGNPU_REG_IRQ_STATUS    0x0000CU
#define LGNPU_REG_IRQ_ENABLE    0x00010U
#define LGNPU_REG_IRQ_CLEAR     0x00014U
#define LGNPU_REG_FEATURE_ID    0x00018U
#define LGNPU_REG_PERF_CYCLES   0x00020U
#define LGNPU_REG_PERF_ACTIVE   0x00024U
#define LGNPU_REG_PERF_STALL    0x00028U
#define LGNPU_REG_DMA_EXT_ADDR  0x00030U
#define LGNPU_REG_DMA_LOC_ADDR  0x00034U
#define LGNPU_REG_DMA_LEN       0x00038U
#define LGNPU_REG_DMA_CTRL      0x0003CU
#define LGNPU_REG_DMA_STATUS    0x00040U

/* Buffer / queue region bases */
#define LGNPU_CMD_QUEUE_BASE    0x01000U
#define LGNPU_WEIGHT_BUF_BASE   0x10000U
#define LGNPU_ACT_BUF_BASE      0x20000U
#define LGNPU_PSUM_BUF_BASE     0x30000U
#define LGNPU_MMIO_SIZE         0x40000U

#define LGNPU_CMD_ENTRY_BYTES   64U
#define LGNPU_CMD_WORDS         (LGNPU_CMD_ENTRY_BYTES / 4)

/* CTRL register bits */
#define LGNPU_CTRL_SOFT_RESET   BIT(0)
#define LGNPU_CTRL_ENABLE       BIT(1)

/* STATUS register bits */
#define LGNPU_STATUS_IDLE       BIT(0)
#define LGNPU_STATUS_BUSY       BIT(1)
#define LGNPU_STATUS_QUEUE_FULL BIT(2)

/* IRQ status bits */
#define LGNPU_STATUS_PENDING    BIT(0)

/* DMA control bits */
#define LGNPU_DMA_CTRL_START    BIT(0)
#define LGNPU_DMA_CTRL_DIR      BIT(1)

/* DMA status bits */
#define LGNPU_DMA_STATUS_BUSY   BIT(0)

/* DMA direction */
#define LGNPU_DMA_TO_DEVICE     0U
#define LGNPU_DMA_FROM_DEVICE   1U

/* Timeout / polling parameters */
#define LGNPU_DMA_POLL_ITERS    10000U
#define LGNPU_DMA_POLL_US       1U
#define LGNPU_CMD_TIMEOUT_MS    5000U

/* Driver name */
#define LGNPU_DRV_NAME          "lgnpu"

/* Private device structure */
struct lgnpu_device
{
    struct device*     dev;
    void __iomem*      regs;
    int                irq;
    struct miscdevice  miscdev;
    struct mutex       cmd_lock;   /* serializes command submission */
    struct completion  cmd_done;   /* signaled by IRQ on cmd completion */
    spinlock_t         irq_lock;   /* protects IRQ register access */
    u32                feature_id;
};

/* MMIO (npu_mmio.c / npu_mmio.h) */

NO_DISCARD COLD_CALL NO_NULL_ARGS
int lgnpu_mmio_init(
    struct lgnpu_device*    npu,
    struct platform_device* pdev
);

/* IRQ (npu_irq.c) */

NO_DISCARD COLD_CALL NO_NULL_ARGS
int lgnpu_irq_init(
    struct lgnpu_device*    npu,
    struct platform_device* pdev
);

HOT_CALL NO_NULL_ARGS
void lgnpu_irq_enable(
    struct lgnpu_device* npu
);

HOT_CALL NO_NULL_ARGS
void lgnpu_irq_disable(
    struct lgnpu_device* npu
);

/* DMA (npu_dma.c) */

NO_DISCARD HOT_CALL NO_NULL_ARGS
int lgnpu_dma_transfer(
    struct lgnpu_device* npu,
    const u32            ext_addr,
    const u32            loc_addr,
    const u32            len,
    const u32            dir
);

/* Command queue (npu_queue.c) */

NO_DISCARD HOT_CALL NO_NULL_ARGS
int lgnpu_cmd_submit(
    struct lgnpu_device* npu,
    const u32*           words
);

/* Reset / init (npu_reset.c) */

NO_DISCARD COLD_CALL NO_NULL_ARGS
int lgnpu_hw_reset(
    struct lgnpu_device* npu
);

NO_DISCARD COLD_CALL NO_NULL_ARGS
int lgnpu_hw_enable(
    struct lgnpu_device* npu
);

NO_DISCARD COLD_CALL NO_NULL_ARGS
int lgnpu_hw_init(
    struct lgnpu_device* npu
);

#endif /* _LGNPU_DRV_H */
