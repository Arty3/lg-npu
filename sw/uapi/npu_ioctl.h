// npu_ioctl.h - NPU device operations API
//
// Provides the device context, MMIO helpers, and the public functions
// for controlling the NPU from host software.
//
// See docs/arch/programming_model.md for the interaction sequence.

#ifndef NPU_IOCTL_H
#define NPU_IOCTL_H

#include "npu_regs.h"
#include "npu_cmd.h"
#include <stdint.h>
#include <stdbool.h>

// Device context.  Holds the MMIO base pointer used by all operations.
struct npu_device
{
    volatile uint32_t* base;
};

// DMA transfer direction.
enum npu_dma_dir
{
    NPU_DMA_TO_DEVICE   = 0,  // external -> local SRAM
    NPU_DMA_FROM_DEVICE = 1,  // local SRAM -> external
};

// DMA transfer request.
struct npu_dma_req
{
    uint32_t         ext_addr;  // external (host-side) byte address
    uint16_t         loc_addr;  // local SRAM byte address
    uint16_t         len;       // transfer length in bytes
    enum npu_dma_dir dir;
};

// Snapshot of the STATUS register.
struct npu_status
{
    bool idle;
    bool busy;
    bool queue_full;
};

// Decoded FEATURE_ID register.
// Bit layout: [31:24] major, [23:16] minor, [15:12] backends,
//             [11:8] array rows, [7:4] array cols, [3:0] dtypes.
struct npu_device_info
{
    uint8_t version_major;
    uint8_t version_minor;
    uint8_t num_backends;
    uint8_t array_rows;
    uint8_t array_cols;
    uint8_t dtypes_supported;
};

// MMIO helpers (single 32-bit word).
static inline uint32_t npu_mmio_read(const struct npu_device* dev,
                                     uint32_t offset)
{
    return dev->base[offset / sizeof(uint32_t)];
}

static inline void npu_mmio_write(struct npu_device* dev,
                                  uint32_t offset, uint32_t value)
{
    dev->base[offset / sizeof(uint32_t)] = value;
}

// Device lifecycle
int  npu_open(struct npu_device* dev, volatile void* mmio_base);
void npu_close(struct npu_device* dev);

// Control
void npu_reset(struct npu_device* dev);
void npu_enable(struct npu_device* dev);

// Status queries
void npu_read_status(const struct npu_device* dev, struct npu_status* out);
void npu_read_info(const struct npu_device* dev, struct npu_device_info* out);
int  npu_poll_idle(const struct npu_device* dev, uint32_t timeout_cycles);

// IRQ management
void npu_irq_enable(struct npu_device* dev);
void npu_irq_disable(struct npu_device* dev);
void npu_irq_clear(struct npu_device* dev);
bool npu_irq_pending(const struct npu_device* dev);

// Performance counters
uint32_t npu_perf_read_cycles(const struct npu_device* dev);
uint32_t npu_perf_read_active(const struct npu_device* dev);
uint32_t npu_perf_read_stall(const struct npu_device* dev);

// Command submission
void npu_submit(struct npu_device* dev, const struct npu_cmd_desc* desc);

// DMA
int npu_dma_start(struct npu_device* dev, const struct npu_dma_req* req);
int npu_dma_poll(const struct npu_device* dev, uint32_t timeout_cycles);

#endif // NPU_IOCTL_H
