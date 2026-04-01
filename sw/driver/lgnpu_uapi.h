/* lgnpu_uapi.h - LG NPU kernel/userspace IOCTL interface
 *
 * Shared between the kernel driver and userspace programs.
 * See docs/spec/register_map.md for hardware register details.
 */

#ifndef _LGNPU_UAPI_H
#define _LGNPU_UAPI_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define LGNPU_CMD_WORDS 16

/* IOCTL data structures */

struct lgnpu_ioctl_status
{
    __u32 idle;
    __u32 busy;
    __u32 queue_full;
};

struct lgnpu_ioctl_info
{
    __u8 version_major;
    __u8 version_minor;
    __u8 num_backends;
    __u8 array_rows;
    __u8 array_cols;
    __u8 dtypes_supported;
    __u8 pad[2];
};

struct lgnpu_ioctl_submit
{
    __u32 words[LGNPU_CMD_WORDS];
};

/* DMA direction values for lgnpu_ioctl_dma.dir */
#define LGNPU_DMA_TO_DEVICE   0U
#define LGNPU_DMA_FROM_DEVICE 1U

struct lgnpu_ioctl_dma
{
    __u32 ext_addr;
    __u32 loc_addr;
    __u32 len;
    __u32 dir;
};

struct lgnpu_ioctl_perf
{
    __u32 cycles;
    __u32 active;
    __u32 stall;
};

/* IOCTL command numbers */

#define LGNPU_IOC_MAGIC 'L'

#define LGNPU_IOCTL_RESET     _IO(LGNPU_IOC_MAGIC,  0)
#define LGNPU_IOCTL_ENABLE    _IO(LGNPU_IOC_MAGIC,  1)
#define LGNPU_IOCTL_STATUS    _IOR(LGNPU_IOC_MAGIC, 2, struct lgnpu_ioctl_status)
#define LGNPU_IOCTL_INFO      _IOR(LGNPU_IOC_MAGIC, 3, struct lgnpu_ioctl_info)
#define LGNPU_IOCTL_SUBMIT    _IOW(LGNPU_IOC_MAGIC, 4, struct lgnpu_ioctl_submit)
#define LGNPU_IOCTL_DMA_XFER  _IOW(LGNPU_IOC_MAGIC, 5, struct lgnpu_ioctl_dma)
#define LGNPU_IOCTL_PERF      _IOR(LGNPU_IOC_MAGIC, 6, struct lgnpu_ioctl_perf)

#endif /* _LGNPU_UAPI_H */
