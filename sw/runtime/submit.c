// submit.c - Command submission and DMA transfer

#include "../uapi/npu_ioctl.h"

void npu_submit(struct npu_device* dev, const struct npu_cmd_desc* desc)
{
    volatile uint32_t* queue =
        dev->base + (NPU_CMD_QUEUE_BASE / sizeof(uint32_t));

    for (uint32_t i = 0; i < NPU_CMD_WORDS; ++i)
        queue[i] = desc->words[i];

    npu_mmio_write(dev, NPU_REG_DOORBELL, 1);
}

int npu_dma_start(struct npu_device* dev, const struct npu_dma_req* req)
{
    if (!dev || !req)
        return -1;

    npu_mmio_write(dev, NPU_REG_DMA_EXT_ADDR, req->ext_addr);
    npu_mmio_write(dev, NPU_REG_DMA_LOC_ADDR, req->loc_addr);
    npu_mmio_write(dev, NPU_REG_DMA_LEN, req->len);

    uint32_t ctrl = NPU_DMA_CTRL_START;
    if (req->dir == NPU_DMA_FROM_DEVICE)
        ctrl |= NPU_DMA_CTRL_DIR;

    npu_mmio_write(dev, NPU_REG_DMA_CTRL, ctrl);
    return 0;
}

int npu_dma_poll(const struct npu_device* dev, uint32_t timeout_cycles)
{
    for (uint32_t i = 0; i < timeout_cycles; ++i)
    {
        if (!(npu_mmio_read(dev, NPU_REG_DMA_STATUS) & NPU_DMA_STATUS_BUSY))
            return 0;
    }
    return -1;
}
