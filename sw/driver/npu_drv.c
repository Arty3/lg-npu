/* npu_drv.c - LG NPU platform driver core
 *
 * Module init/exit, platform probe/remove, and misc device file operations.
 * See docs/arch/programming_model.md for the host-device interaction model.
 */

#include "lgnpu_uapi.h"
#include "lgnpu_drv.h"

#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/of.h>

static long lgnpu_ioctl(struct file* filp, unsigned int cmd, unsigned long arg)
{
    struct lgnpu_device* npu = container_of(
        filp->private_data,
        struct lgnpu_device,
        miscdev
    );

    void __user* uptr = (void __user*)arg;

    switch (cmd)
    {
    case LGNPU_IOCTL_RESET:
        return lgnpu_hw_reset(npu);

    case LGNPU_IOCTL_ENABLE:
        return lgnpu_hw_enable(npu);

    case LGNPU_IOCTL_STATUS:
    {
        struct lgnpu_ioctl_status st;
        const u32 val = lgnpu_reg_read(npu, LGNPU_REG_STATUS);

        st.idle       = !!(val & LGNPU_STATUS_IDLE);
        st.busy       = !!(val & LGNPU_STATUS_BUSY);
        st.queue_full = !!(val & LGNPU_STATUS_QUEUE_FULL);

        if (copy_to_user(uptr, &st, sizeof(st)))
            return -EFAULT;

        return 0;
    }

    case LGNPU_IOCTL_INFO:
    {
        struct lgnpu_ioctl_info info;
        const u32 val = npu->feature_id;

        info.version_major    = (u8)((val >> 24) & 0xFFU);
        info.version_minor    = (u8)((val >> 16) & 0xFFU);
        info.num_backends     = (u8)((val >> 12) & 0x0FU);
        info.array_rows       = (u8)((val >>  8) & 0x0FU);
        info.array_cols       = (u8)((val >>  4) & 0x0FU);
        info.dtypes_supported = (u8)( val        & 0x0FU);
        info.pad[0]           = 0;
        info.pad[1]           = 0;

        if (copy_to_user(uptr, &info, sizeof(info)))
            return -EFAULT;

        return 0;
    }

    case LGNPU_IOCTL_SUBMIT:
    {
        struct lgnpu_ioctl_submit sub;

        if (copy_from_user(&sub, uptr, sizeof(sub)))
            return -EFAULT;

        return lgnpu_cmd_submit(npu, sub.words);
    }

    case LGNPU_IOCTL_DMA_XFER:
    {
        struct lgnpu_ioctl_dma dma;

        if (copy_from_user(&dma, uptr, sizeof(dma)))
            return -EFAULT;

        if (dma.dir > LGNPU_DMA_FROM_DEVICE)
            return -EINVAL;

        return lgnpu_dma_transfer(
            npu,
            dma.ext_addr,
            dma.loc_addr,
            dma.len,
            dma.dir
        );
    }

    case LGNPU_IOCTL_PERF:
    {
        struct lgnpu_ioctl_perf perf;

        perf.cycles = lgnpu_reg_read(npu, LGNPU_REG_PERF_CYCLES);
        perf.active = lgnpu_reg_read(npu, LGNPU_REG_PERF_ACTIVE);
        perf.stall  = lgnpu_reg_read(npu, LGNPU_REG_PERF_STALL);

        if (copy_to_user(uptr, &perf, sizeof(perf)))
            return -EFAULT;

        return 0;
    }

    default:
        return -ENOTTY;
    }
}

static const struct file_operations lgnpu_fops =
{
    .owner          = THIS_MODULE,
    .unlocked_ioctl = lgnpu_ioctl,
};

static int lgnpu_probe(struct platform_device* pdev)
{
    struct lgnpu_device* npu;
    int ret;

    npu = devm_kzalloc(&pdev->dev, sizeof(*npu), GFP_KERNEL);

    if (!npu)
        return -ENOMEM;

    npu->dev = &pdev->dev;

    mutex_init(&npu->cmd_lock);
    init_completion(&npu->cmd_done);
    spin_lock_init(&npu->irq_lock);

    platform_set_drvdata(pdev, npu);

    ret = lgnpu_mmio_init(npu, pdev);

    if (ret)
        return ret;

    ret = lgnpu_irq_init(npu, pdev);

    if (ret)
        return ret;

    ret = lgnpu_hw_init(npu);

    if (ret)
        return ret;

    npu->miscdev.minor = MISC_DYNAMIC_MINOR;
    npu->miscdev.name  = LGNPU_DRV_NAME;
    npu->miscdev.fops  = &lgnpu_fops;

    ret = misc_register(&npu->miscdev);

    if (ret)
    {
        dev_err(npu->dev, "failed to register misc device\n");
        return ret;
    }

    dev_info(npu->dev, "LG NPU registered (feature_id 0x%08x)\n", npu->feature_id);

    return 0;
}

/* Targets Linux >= 6.11 where .remove returns void.
 * For 6.3-6.10, rename to .remove_new in the driver struct.
 * For < 6.3, change return type to int and return 0. */
static void lgnpu_remove(struct platform_device* pdev)
{
    struct lgnpu_device* npu = platform_get_drvdata(pdev);

    lgnpu_irq_disable(npu);
    misc_deregister(&npu->miscdev);
    dev_info(npu->dev, "LG NPU removed\n");
}

static const struct of_device_id lgnpu_of_match[] =
{
    { .compatible = "lg,npu" },
    { },
};

MODULE_DEVICE_TABLE(of, lgnpu_of_match);

static struct platform_driver lgnpu_platform_driver =
{
    .probe  = lgnpu_probe,
    .remove = lgnpu_remove,

    .driver = {
        .name           = LGNPU_DRV_NAME,
        .of_match_table = lgnpu_of_match,
    },
};

module_platform_driver(lgnpu_platform_driver);

MODULE_DESCRIPTION("LG NPU driver");

/* SPDX-License-Identifier: GPL-2.0 OR Apache-2.0 */
MODULE_LICENSE("GPL");

MODULE_AUTHOR("Luca Goddijn");
