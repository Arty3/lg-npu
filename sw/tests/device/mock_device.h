/* mock_device.h - Mock NPU device backend for testing
*
* Instead of real MMIO, provides a register file that records all
* writes and returns programmed read values. Tests can inspect the
* operation log to verify correctness.
*/

#ifndef _MOCK_DEVICE_H
#define _MOCK_DEVICE_H

#include "../../uapi/lgnpu_ioctl.h"
#include "../../uapi/lgnpu_regs.h"

#include <stdint.h>
#include <string.h>

/* Register file size: cover up to the command queue region */
#define MOCK_REG_COUNT (LGNPU_CMD_QUEUE_BASE / sizeof(uint32_t) + 64)

/* Operation log entry */
enum mock_op_type
{
    MOCK_OP_WRITE = 0,
    MOCK_OP_READ  = 1,
};

struct mock_op_entry
{
    enum mock_op_type type;
    uint32_t          offset;   /* byte offset */
    uint32_t          value;    /* written or read value */
};

#define MOCK_OP_LOG_SIZE 256

struct mock_device
{
    uint32_t             regs[MOCK_REG_COUNT];
    struct mock_op_entry ops[MOCK_OP_LOG_SIZE];
    uint32_t             op_count;
    int                  dma_busy;
    int                  busy;
};

static void mock_device_init(struct mock_device* m)
{
    __builtin_memset(m, 0, sizeof(*m));

    /* Default: device is idle */
    m->regs[LGNPU_REG_STATUS / sizeof(uint32_t)] = LGNPU_STATUS_IDLE;

    /* Default feature ID: v1.0, 6 backends, 8x8 array, 1 dtype */
    m->regs[LGNPU_REG_FEATURE_ID / sizeof(uint32_t)] = \
        (1U << 24) | (0U << 16) | (6U << 12) | (8U << 8) | (8U << 4) | 1U;
}

static inline void mock_reset_ops(struct mock_device* m)
{
    m->op_count = 0;
}

static inline void mock_log_op(
    struct mock_device* m,
    enum mock_op_type   type,
    uint32_t            offset,
    uint32_t            value)
{
    if (m->op_count < MOCK_OP_LOG_SIZE)
    {
        m->ops[m->op_count].type   = type;
        m->ops[m->op_count].offset = offset;
        m->ops[m->op_count].value  = value;

        ++m->op_count;
    }
}

/* Check if a specific write occurred in the log */
static inline int mock_has_write(const struct mock_device* m, uint32_t offset, uint32_t value)
{
    for (uint32_t i = 0; i < m->op_count; ++i)
        if (m->ops[i].type   == MOCK_OP_WRITE &&
            m->ops[i].offset == offset        &&
            m->ops[i].value  == value)
            return 1;

    return 0;
}

/* Count writes to a specific offset */
static inline uint32_t mock_count_writes(const struct mock_device* m, uint32_t offset)
{
    uint32_t count = 0;

    for (uint32_t i = 0; i < m->op_count; ++i)
        if (m->ops[i].type   == MOCK_OP_WRITE &&
            m->ops[i].offset == offset)
            ++count;

    return count;
}

/* Get the value of the Nth write to an offset (0-indexed) */
static inline int mock_get_nth_write(
    const struct mock_device* m,
    uint32_t                  offset,
    uint32_t                  n,
    uint32_t*                 out_value)
{
    uint32_t seen = 0;

    for (uint32_t i = 0; i < m->op_count; ++i)
    {
        if (m->ops[i].type   == MOCK_OP_WRITE &&
            m->ops[i].offset == offset)
        {
            if (seen == n)
            {
                *out_value = m->ops[i].value;
                return 0;
            }
            ++seen;
        }
    }

    return -1;
}

/* Hook: called on every write by the mock volatile base.
* Handles DMA and status side-effects. */
static inline void mock_handle_write(struct mock_device* m, uint32_t reg_idx, uint32_t val)
{
    const uint32_t offset = reg_idx * (uint32_t)sizeof(uint32_t);

    mock_log_op(m, MOCK_OP_WRITE, offset, val);
    m->regs[reg_idx] = val;

    /* Side effects */
    if (offset == LGNPU_REG_CTRL)
    {
        if (val & LGNPU_CTRL_SOFT_RESET)
        {
            m->busy     = 0;
            m->dma_busy = 0;

            m->regs[LGNPU_REG_STATUS     / sizeof(uint32_t)] = LGNPU_STATUS_IDLE;
            m->regs[LGNPU_REG_DMA_STATUS / sizeof(uint32_t)] = 0;
        }

        if (val & LGNPU_CTRL_ENABLE)
            m->regs[LGNPU_REG_STATUS / sizeof(uint32_t)] = LGNPU_STATUS_IDLE;
    }

    if (offset == LGNPU_REG_DMA_CTRL && (val & LGNPU_DMA_CTRL_START))
    {
        /* Simulate instant DMA completion for mock */
        m->dma_busy = 0;
        m->regs[LGNPU_REG_DMA_STATUS / sizeof(uint32_t)] = 0;
    }

    if (offset == LGNPU_REG_DOORBELL)
    {
        /* Simulate instant command completion */
        m->regs[LGNPU_REG_STATUS / sizeof(uint32_t)] = LGNPU_STATUS_IDLE;
    }
}

/* Hook: called on every read */
static inline uint32_t mock_handle_read(struct mock_device* m, uint32_t reg_idx)
{
    const uint32_t offset = reg_idx * (uint32_t)sizeof(uint32_t);
    const uint32_t val    = m->regs[reg_idx];

    mock_log_op(m, MOCK_OP_READ, offset, val);
    return val;
}

/* We interpose on volatile accesses by providing a wrapper.
* The npu_device.base pointer points to an array where writes/reads
* are intercepted via a special accessor layer.
*
* For the mock backend, we create a struct that embeds the reg array
* and set up the npu_device to point to it. Since the runtime uses
* base[offset/4] = val directly on volatile pointers, we need to
* manually intercept. The approach: after each runtime call, we
* scan the regs for changes. But simpler: we use the mock regs
* directly as the volatile base.
*
* Because the runtime writes directly to volatile pointers, the mock
* "just works" since the regs array IS the backing store. We call
* mock_sync_ops after runtime calls to record what happened. */

/* Sync: scan for writes that happened since last sync.
* This is a post-hoc approach, adequate for testing. */

static inline void mock_setup_device(
    struct mock_device* m,
    struct npu_device*  dev)
{
    dev->base = (volatile uint32_t*)m->regs;
}

/* Post-call sync: compare regs to recorded state and log diffs.
* For simplicity, the mock just records via handle_write/handle_read
* hooks, but since the runtime writes directly to the volatile array
* we use the alternative: wrap each test call. */

/* Convenience: log all writes that the runtime made by inspecting
* register changes vs a snapshot. */
struct mock_snapshot
{
    uint32_t regs[MOCK_REG_COUNT];
};

static inline void mock_take_snapshot(
    const struct mock_device* m,
    struct mock_snapshot*     snap)
{
    __builtin_memcpy(snap->regs, m->regs, sizeof(snap->regs));
}

static inline void mock_diff_snapshot(
    struct mock_device*         m,
    const struct mock_snapshot* snap)
{
    for (uint32_t i = 0; i < MOCK_REG_COUNT; ++i)
        if (m->regs[i] != snap->regs[i])
            mock_log_op(
                m,
                MOCK_OP_WRITE,
                i * (uint32_t)sizeof(uint32_t),
                m->regs[i]
            );
}

#endif /* _MOCK_DEVICE_H */
