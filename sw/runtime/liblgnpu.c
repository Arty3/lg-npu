/* libnpu.c - Device lifecycle, status queries, IRQ, and perf counters */

#include "../shared/annotations.h"
#include "../uapi/lgnpu_ioctl.h"

#include "mmio_helpers.h"

#include <stddef.h>

int npu_open(struct npu_device* dev, volatile void* mmio_base)
{
    if (UNLIKELY(!dev || !mmio_base))
        return -1;

    dev->base = (volatile uint32_t*)mmio_base;
    return 0;
}

int npu_close(struct npu_device* dev)
{
    if (UNLIKELY(!dev))
        return -1;

    dev->base = NULL;
    return 0;
}

/* Control */

int npu_reset(struct npu_device* dev)
{
    if (UNLIKELY(!dev))
        return -1;

    npu_mmio_write(dev, LGNPU_REG_CTRL, LGNPU_CTRL_SOFT_RESET);
    return 0;
}

int npu_enable(struct npu_device* dev)
{
    if (UNLIKELY(!dev))
        return -1;

    npu_mmio_write(dev, LGNPU_REG_CTRL, LGNPU_CTRL_ENABLE);
    return 0;
}

/* Status queries */

int npu_read_status(const struct npu_device* dev, struct npu_status* out)
{
    if (UNLIKELY(!dev || !out))
        return -1;

    const uint32_t val = npu_mmio_read(dev, LGNPU_REG_STATUS);

    out->idle       = (val & LGNPU_STATUS_IDLE)       != 0;
    out->busy       = (val & LGNPU_STATUS_BUSY)       != 0;
    out->queue_full = (val & LGNPU_STATUS_QUEUE_FULL) != 0;
    return 0;
}

int npu_read_info(const struct npu_device* dev, struct npu_device_info* out)
{
    if (UNLIKELY(!dev || !out))
        return -1;

    const uint32_t val = npu_mmio_read(dev, LGNPU_REG_FEATURE_ID);

    out->version_major    = (uint8_t)((val >> 24) & 0xFFU);
    out->version_minor    = (uint8_t)((val >> 16) & 0xFFU);
    out->num_backends     = (uint8_t)((val >> 12) & 0x0FU);
    out->array_rows       = (uint8_t)((val >> 8)  & 0x0FU);
    out->array_cols       = (uint8_t)((val >> 4)  & 0x0FU);
    out->dtypes_supported = (uint8_t)( val        & 0x0FU);
    return 0;
}

int npu_poll_idle(const struct npu_device* dev, uint32_t timeout_cycles)
{
    if (UNLIKELY(!dev))
        return -1;

    for (uint32_t i = 0; i < timeout_cycles; ++i)
    {
        const uint32_t val = npu_mmio_read(dev, LGNPU_REG_STATUS);

        if ((val & LGNPU_STATUS_IDLE) && !(val & LGNPU_STATUS_BUSY))
            return 0;
    }

    return -1;
}

/* IRQ management */

int npu_irq_enable(struct npu_device* dev)
{
    if (UNLIKELY(!dev))
        return -1;

    npu_mmio_write(dev, LGNPU_REG_IRQ_ENABLE, 1);
    return 0;
}

int npu_irq_disable(struct npu_device* dev)
{
    if (UNLIKELY(!dev))
        return -1;

    npu_mmio_write(dev, LGNPU_REG_IRQ_ENABLE, 0);
    return 0;
}

int npu_irq_clear(struct npu_device* dev)
{
    if (UNLIKELY(!dev))
        return -1;

    npu_mmio_write(dev, LGNPU_REG_IRQ_CLEAR, 1);
    return 0;
}

int npu_irq_pending(const struct npu_device* dev)
{
    if (UNLIKELY(!dev))
        return 0;

    return (npu_mmio_read(dev, LGNPU_REG_IRQ_STATUS) & 1U) != 0;
}

/* Performance counters */

uint32_t npu_perf_read_cycles(const struct npu_device* dev)
{
    if (UNLIKELY(!dev))
        return 0;

    return npu_mmio_read(dev, LGNPU_REG_PERF_CYCLES);
}

uint32_t npu_perf_read_active(const struct npu_device* dev)
{
    if (UNLIKELY(!dev))
        return 0;

    return npu_mmio_read(dev, LGNPU_REG_PERF_ACTIVE);
}

uint32_t npu_perf_read_stall(const struct npu_device* dev)
{
    if (UNLIKELY(!dev))
        return 0;

    return npu_mmio_read(dev, LGNPU_REG_PERF_STALL);
}
