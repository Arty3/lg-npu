// libnpu.c - Device lifecycle, status queries, IRQ, and perf counters

#include "../uapi/npu_ioctl.h"
#include <stddef.h>

int npu_open(struct npu_device* dev, volatile void* mmio_base)
{
    if (!dev || !mmio_base)
        return -1;

    dev->base = (volatile uint32_t*)mmio_base;
    return 0;
}

void npu_close(struct npu_device* dev)
{
    if (dev)
        dev->base = NULL;
}

// Control

void npu_reset(struct npu_device* dev)
{
    npu_mmio_write(dev, NPU_REG_CTRL, NPU_CTRL_SOFT_RESET);
}

void npu_enable(struct npu_device* dev)
{
    npu_mmio_write(dev, NPU_REG_CTRL, NPU_CTRL_ENABLE);
}

// Status queries

void npu_read_status(const struct npu_device* dev, struct npu_status* out)
{
    uint32_t val = npu_mmio_read(dev, NPU_REG_STATUS);
    out->idle       = (val & NPU_STATUS_IDLE)       != 0;
    out->busy       = (val & NPU_STATUS_BUSY)       != 0;
    out->queue_full = (val & NPU_STATUS_QUEUE_FULL) != 0;
}

void npu_read_info(const struct npu_device* dev, struct npu_device_info* out)
{
    uint32_t val = npu_mmio_read(dev, NPU_REG_FEATURE_ID);
    out->version_major    = (uint8_t)((val >> 24) & 0xFF);
    out->version_minor    = (uint8_t)((val >> 16) & 0xFF);
    out->num_backends     = (uint8_t)((val >> 12) & 0x0F);
    out->array_rows       = (uint8_t)((val >> 8)  & 0x0F);
    out->array_cols       = (uint8_t)((val >> 4)  & 0x0F);
    out->dtypes_supported = (uint8_t)(val & 0x0F);
}

int npu_poll_idle(const struct npu_device* dev, uint32_t timeout_cycles)
{
    for (uint32_t i = 0; i < timeout_cycles; ++i)
    {
        uint32_t val = npu_mmio_read(dev, NPU_REG_STATUS);
        if ((val & NPU_STATUS_IDLE) && !(val & NPU_STATUS_BUSY))
            return 0;
    }
    return -1;
}

// IRQ management

void npu_irq_enable(struct npu_device* dev)
{
    npu_mmio_write(dev, NPU_REG_IRQ_ENABLE, 1);
}

void npu_irq_disable(struct npu_device* dev)
{
    npu_mmio_write(dev, NPU_REG_IRQ_ENABLE, 0);
}

void npu_irq_clear(struct npu_device* dev)
{
    npu_mmio_write(dev, NPU_REG_IRQ_CLEAR, 1);
}

bool npu_irq_pending(const struct npu_device* dev)
{
    return (npu_mmio_read(dev, NPU_REG_IRQ_STATUS) & 1U) != 0;
}

// Performance counters

uint32_t npu_perf_read_cycles(const struct npu_device* dev)
{
    return npu_mmio_read(dev, NPU_REG_PERF_CYCLES);
}

uint32_t npu_perf_read_active(const struct npu_device* dev)
{
    return npu_mmio_read(dev, NPU_REG_PERF_ACTIVE);
}

uint32_t npu_perf_read_stall(const struct npu_device* dev)
{
    return npu_mmio_read(dev, NPU_REG_PERF_STALL);
}
