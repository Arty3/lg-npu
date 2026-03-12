/* test_dma.c - DMA runtime logic tests
 *
 * Verifies the register programming sequence:
 *   1. program EXT_ADDR
 *   2. program LOC_ADDR
 *   3. program LEN
 *   4. start DMA (CTRL register)
 *   5. poll BUSY
 *
 * Edge cases: zero length, overlapping buffers, DMA while busy,
 * reset during DMA.
 */

#include "../../uapi/lgnpu_ioctl.h"
#include "../device/mock_device.h"
#include "../test_harness.h"

#include <string.h>

/* Normal DMA: to-device */
static void test_dma_to_device(void)
{
    TEST_BEGIN("dma: to-device register sequence");

    struct mock_device mock;
    mock_device_init(&mock);
    mock_reset_ops(&mock);

    struct npu_device dev;
    mock_setup_device(&mock, &dev);

    struct mock_snapshot snap;
    mock_take_snapshot(&mock, &snap);

    struct npu_dma_req req = {
        .ext_addr = 0x80000000,
        .loc_addr = 0x0000,
        .len      = 256,
        .dir      = LGNPU_DMA_TO_DEVICE,
    };

    ASSERT_EQ(npu_dma_start(&dev, &req), 0);

    mock_diff_snapshot(&mock, &snap);

    /* Verify registers programmed */
    ASSERT_EQ(mock.regs[LGNPU_REG_DMA_EXT_ADDR / sizeof(uint32_t)], 0x80000000u);
    ASSERT_EQ(mock.regs[LGNPU_REG_DMA_LOC_ADDR / sizeof(uint32_t)], 0x0000u);
    ASSERT_EQ(mock.regs[LGNPU_REG_DMA_LEN      / sizeof(uint32_t)], 256u);

    /* CTRL = START, no DIR bit for to-device */
    ASSERT_EQ(
        mock.regs[LGNPU_REG_DMA_CTRL / sizeof(uint32_t)],
        LGNPU_DMA_CTRL_START
    );

    TEST_PASS();
}

/* Normal DMA: from-device */
static void test_dma_from_device(void)
{
    TEST_BEGIN("dma: from-device sets DIR bit");

    struct mock_device mock;
    mock_device_init(&mock);

    struct npu_device dev;
    mock_setup_device(&mock, &dev);

    struct npu_dma_req req = {
        .ext_addr = 0xA0000000,
        .loc_addr = 0x1000,
        .len      = 512,
        .dir      = LGNPU_DMA_FROM_DEVICE,
    };

    ASSERT_EQ(npu_dma_start(&dev, &req), 0);

    ASSERT_EQ(
        mock.regs[LGNPU_REG_DMA_CTRL / sizeof(uint32_t)],
        LGNPU_DMA_CTRL_START | LGNPU_DMA_CTRL_DIR
    );

    TEST_PASS();
}

/* DMA poll: immediate completion */
static void test_dma_poll_immediate(void)
{
    TEST_BEGIN("dma_poll: not busy -> immediate success");

    struct mock_device mock;
    mock_device_init(&mock);

    struct npu_device dev;
    mock_setup_device(&mock, &dev);

    mock.regs[LGNPU_REG_DMA_STATUS / sizeof(uint32_t)] = 0;
    ASSERT_EQ(npu_dma_poll(&dev, 100), 0);

    TEST_PASS();
}

/* DMA poll: timeout */
static void test_dma_poll_timeout(void)
{
    TEST_BEGIN("dma_poll: busy -> timeout");

    struct mock_device mock;
    mock_device_init(&mock);

    struct npu_device dev;
    mock_setup_device(&mock, &dev);

    mock.regs[LGNPU_REG_DMA_STATUS / sizeof(uint32_t)] = LGNPU_DMA_STATUS_BUSY;
    ASSERT_EQ(npu_dma_poll(&dev, 10), -1);

    TEST_PASS();
}

/* Zero length transfer */
static void test_dma_zero_length(void)
{
    TEST_BEGIN("dma: zero length transfer accepted by runtime");

    struct mock_device mock;
    mock_device_init(&mock);

    struct npu_device dev;
    mock_setup_device(&mock, &dev);

    struct npu_dma_req req = {
        .ext_addr = 0x1000,
        .loc_addr = 0x0000,
        .len      = 0,
        .dir      = LGNPU_DMA_TO_DEVICE,
    };

    /* Runtime should still program registers predictably */
    ASSERT_EQ(npu_dma_start(&dev, &req), 0);
    ASSERT_EQ(mock.regs[LGNPU_REG_DMA_LEN / sizeof(uint32_t)], 0u);

    TEST_PASS();
}

/* DMA while already busy: runtime still programs registers
 * (hardware handles sequencing). The point is predictable behavior. */
static void test_dma_while_busy(void)
{
    TEST_BEGIN("dma: start while busy still programs registers");

    struct mock_device mock;
    mock_device_init(&mock);

    struct npu_device dev;
    mock_setup_device(&mock, &dev);

    /* Set DMA busy */
    mock.regs[LGNPU_REG_DMA_STATUS / sizeof(uint32_t)] = LGNPU_DMA_STATUS_BUSY;

    struct npu_dma_req req = {
        .ext_addr = 0x2000,
        .loc_addr = 0x100,
        .len      = 64,
        .dir      = LGNPU_DMA_TO_DEVICE,
    };

    /* Should still succeed at the API level */
    ASSERT_EQ(npu_dma_start(&dev, &req), 0);

    /* Registers were written despite busy */
    ASSERT_EQ(mock.regs[LGNPU_REG_DMA_EXT_ADDR / sizeof(uint32_t)], 0x2000u);
    ASSERT_EQ(mock.regs[LGNPU_REG_DMA_LEN / sizeof(uint32_t)], 64u);

    TEST_PASS();
}

/* Overlapping addresses: runtime does not check, hardware handles it */
static void test_dma_overlapping_addresses(void)
{
    TEST_BEGIN("dma: overlapping ext/loc allowed at runtime level");

    struct mock_device mock;
    mock_device_init(&mock);

    struct npu_device dev;
    mock_setup_device(&mock, &dev);

    struct npu_dma_req req = {
        .ext_addr = 0x100,
        .loc_addr = 0x100,  /* same as ext */
        .len      = 128,
        .dir      = LGNPU_DMA_TO_DEVICE,
    };

    ASSERT_EQ(npu_dma_start(&dev, &req), 0);
    ASSERT_EQ(mock.regs[LGNPU_REG_DMA_EXT_ADDR / sizeof(uint32_t)], 0x100u);
    ASSERT_EQ(mock.regs[LGNPU_REG_DMA_LOC_ADDR / sizeof(uint32_t)], 0x100u);

    TEST_PASS();
}

/* Reset during DMA: runtime resets, hardware clears busy.
 * Since the mock volatile array doesn't have write-intercept hooks,
 * we verify the runtime issues the reset, then simulate the hardware
 * side-effect (DMA busy cleared) and confirm poll succeeds. */
static void test_reset_during_dma(void)
{
    TEST_BEGIN("dma: reset then poll succeeds");

    struct mock_device mock;
    mock_device_init(&mock);

    struct npu_device dev;
    mock_setup_device(&mock, &dev);

    /* Simulate DMA in progress */
    mock.regs[LGNPU_REG_DMA_STATUS / sizeof(uint32_t)] = LGNPU_DMA_STATUS_BUSY;

    /* Reset the device */
    ASSERT_EQ(npu_reset(&dev), 0);

    /* Verify reset was written */
    ASSERT_EQ(mock.regs[LGNPU_REG_CTRL / sizeof(uint32_t)], LGNPU_CTRL_SOFT_RESET);

    /* Simulate hardware side-effect: reset clears DMA busy */
    mock.regs[LGNPU_REG_DMA_STATUS / sizeof(uint32_t)] = 0;

    /* DMA poll should now succeed */
    ASSERT_EQ(npu_dma_poll(&dev, 1), 0);

    TEST_PASS();
}

/* Register programming order verification */
static void test_dma_register_order(void)
{
    TEST_BEGIN("dma: EXT_ADDR, LOC_ADDR, LEN, CTRL written in order");

    struct mock_device mock;
    mock_device_init(&mock);
    mock_reset_ops(&mock);

    struct npu_device dev;
    mock_setup_device(&mock, &dev);

    struct mock_snapshot snap;
    mock_take_snapshot(&mock, &snap);

    struct npu_dma_req req = {
        .ext_addr = 0x50000,
        .loc_addr = 0x200,
        .len      = 1024,
        .dir      = LGNPU_DMA_TO_DEVICE,
    };

    ASSERT_EQ(npu_dma_start(&dev, &req), 0);
    mock_diff_snapshot(&mock, &snap);

    /* Verify all four registers were written */
    ASSERT_TRUE(mock.op_count >= 4);

    /* Check offsets appear in the log in correct order */
    int found_ext = 0, found_loc = 0, found_len = 0, found_ctrl = 0;

    for (uint32_t i = 0; i < mock.op_count; ++i)
    {
        if (mock.ops[i].type != MOCK_OP_WRITE)
            continue;

        if (mock.ops[i].offset == LGNPU_REG_DMA_EXT_ADDR)
        {
            found_ext = 1;
            ASSERT_TRUE(!found_loc && !found_len && !found_ctrl);
        }

        if (mock.ops[i].offset == LGNPU_REG_DMA_LOC_ADDR)
        {
            found_loc = 1;
            ASSERT_TRUE(found_ext && !found_len && !found_ctrl);
        }

        if (mock.ops[i].offset == LGNPU_REG_DMA_LEN)
        {
            found_len = 1;
            ASSERT_TRUE(found_ext && found_loc && !found_ctrl);
        }
        
        if (mock.ops[i].offset == LGNPU_REG_DMA_CTRL)
        {
            found_ctrl = 1;
            ASSERT_TRUE(found_ext && found_loc && found_len);
        }
    }

    ASSERT_TRUE(found_ext && found_loc && found_len && found_ctrl);

    TEST_PASS();
}

/* Null pointer tests */
static void test_dma_null(void)
{
    TEST_BEGIN("dma: null pointers");

    struct mock_device mock;
    mock_device_init(&mock);

    struct npu_device dev;
    mock_setup_device(&mock, &dev);

    struct npu_dma_req req;
    __builtin_memset(&req, 0, sizeof(req));

    ASSERT_EQ(npu_dma_start(NULL, &req), -1);
    ASSERT_EQ(npu_dma_start(&dev, NULL), -1);
    ASSERT_EQ(npu_dma_poll(NULL, 100), -1);

    TEST_PASS();
}

/* Max length transfer */
static void test_dma_max_length(void)
{
    TEST_BEGIN("dma: max uint16 length");

    struct mock_device mock;
    mock_device_init(&mock);

    struct npu_device dev;
    mock_setup_device(&mock, &dev);

    struct npu_dma_req req = {
        .ext_addr = 0x10000000,
        .loc_addr = 0x0000,
        .len      = 0xFFFF,
        .dir      = LGNPU_DMA_TO_DEVICE,
    };

    ASSERT_EQ(npu_dma_start(&dev, &req), 0);
    ASSERT_EQ(mock.regs[LGNPU_REG_DMA_LEN / sizeof(uint32_t)], 0xFFFFu);

    TEST_PASS();
}

int main(void)
{
    SUITE_BEGIN("DMA Operations");

    test_dma_to_device();
    test_dma_from_device();
    test_dma_poll_immediate();
    test_dma_poll_timeout();
    test_dma_zero_length();
    test_dma_while_busy();
    test_dma_overlapping_addresses();
    test_reset_during_dma();
    test_dma_register_order();
    test_dma_null();
    test_dma_max_length();

    SUITE_END();

    return SUITE_RESULT();
}
