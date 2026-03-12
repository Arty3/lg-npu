/* npu_ioctl.h - NPU device operations API
 *
 * Provides the device context and the public functions
 * for controlling the NPU from host software.
 *
 * See docs/arch/programming_model.md for the interaction sequence.
 */

#ifndef _LGNPU_IOCTL_H
#define _LGNPU_IOCTL_H

#include "../shared/annotations.h"

#include "lgnpu_regs.h"
#include "lgnpu_cmd.h"

#include <stdint.h>

/* Device context.
 * Holds the MMIO base pointer used by all operations. */
struct npu_device
{
	volatile uint32_t* base;
};

/* DMA transfer direction. */
enum npu_dma_dir
{
	LGNPU_DMA_TO_DEVICE   = 0,  /* external -> local SRAM */
	LGNPU_DMA_FROM_DEVICE = 1,  /* local SRAM -> external */
};

/* DMA transfer request. */
struct npu_dma_req
{
	uint32_t         ext_addr;  /* external (host-side) byte address */
	uint16_t         loc_addr;  /* local SRAM byte address */
	uint16_t         len;       /* transfer length in bytes */
	enum npu_dma_dir dir;
};

/* Snapshot of the STATUS register. */
struct npu_status
{
	int idle;
	int busy;
	int queue_full;
};

/* Decoded FEATURE_ID register.
 * Bit layout: [31:24] major, [23:16] minor, [15:12] backends,
 *             [11:8] array rows, [7:4] array cols, [3:0] dtypes. */
struct npu_device_info
{
	uint8_t version_major;
	uint8_t version_minor;
	uint8_t num_backends;
	uint8_t array_rows;
	uint8_t array_cols;
	uint8_t dtypes_supported;
};

/* Device lifecycle */
NO_DISCARD NO_INLINE COLD_CALL API_CALL
int npu_open(struct npu_device* dev, volatile void* mmio_base);

NO_DISCARD NO_INLINE COLD_CALL API_CALL
int npu_close(struct npu_device* dev);

/* Control */
NO_DISCARD API_CALL
int npu_reset(struct npu_device* dev);

NO_DISCARD API_CALL
int npu_enable(struct npu_device* dev);

/* Status queries */
NO_DISCARD API_CALL
int npu_read_status(const struct npu_device* dev, struct npu_status* out);

NO_DISCARD API_CALL
int npu_read_info(const struct npu_device* dev, struct npu_device_info* out);

NO_DISCARD API_CALL
int npu_poll_idle(const struct npu_device* dev, uint32_t timeout_cycles);

/* IRQ management */
NO_DISCARD API_CALL
int npu_irq_enable(struct npu_device* dev);

NO_DISCARD API_CALL
int npu_irq_disable(struct npu_device* dev);

NO_DISCARD API_CALL
int npu_irq_clear(struct npu_device* dev);

NO_DISCARD API_CALL
int npu_irq_pending(const struct npu_device* dev);

/* Performance counters */
NO_DISCARD API_CALL
uint32_t npu_perf_read_cycles(const struct npu_device* dev);

NO_DISCARD API_CALL
uint32_t npu_perf_read_active(const struct npu_device* dev);

NO_DISCARD API_CALL
uint32_t npu_perf_read_stall(const struct npu_device* dev);

/* Command submission */
NO_DISCARD API_CALL
int npu_submit(struct npu_device* dev, const struct npu_cmd_desc* desc);

/* DMA */
NO_DISCARD API_CALL
int npu_dma_start(struct npu_device* dev, const struct npu_dma_req* req);

NO_DISCARD API_CALL
int npu_dma_poll(const struct npu_device* dev, uint32_t timeout_cycles);

#endif /* _LGNPU_IOCTL_H */
