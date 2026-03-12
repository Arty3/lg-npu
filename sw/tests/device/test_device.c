/* test_device.c - Device control tests using mock backend
*
* Tests the runtime API for device lifecycle, status queries,
* IRQ management, and perf counters against a mock register file.
*/

#include "../../uapi/lgnpu_ioctl.h"
#include "../test_harness.h"
#include "mock_device.h"

#include <string.h>

/* Lifecycle */

static void test_open_null_dev(void)
{
    TEST_BEGIN("open: null dev");

    uint32_t fake_base;
    ASSERT_EQ(npu_open(NULL, &fake_base), -1);

    TEST_PASS();
}

static void test_open_null_base(void)
{
    TEST_BEGIN("open: null mmio_base");

    struct npu_device dev;
    ASSERT_EQ(npu_open(&dev, NULL), -1);

    TEST_PASS();
}

static void test_open_close(void)
{
    TEST_BEGIN("open+close: normal lifecycle");

    struct mock_device mock;
    mock_device_init(&mock);

    struct npu_device dev;
    ASSERT_EQ(npu_open(&dev, mock.regs), 0);
    ASSERT_TRUE(dev.base != NULL);
    ASSERT_EQ(npu_close(&dev), 0);
    ASSERT_TRUE(dev.base == NULL);

    TEST_PASS();
}

static void test_close_null(void)
{
    TEST_BEGIN("close: null dev");

    ASSERT_EQ(npu_close(NULL), -1);

    TEST_PASS();
}

/* Reset and enable */

static void test_reset(void)
{
    TEST_BEGIN("reset: writes CTRL_SOFT_RESET");

    struct mock_device mock;
    mock_device_init(&mock);

    struct npu_device dev;
    mock_setup_device(&mock, &dev);

    ASSERT_EQ(npu_reset(&dev), 0);
    ASSERT_EQ(mock.regs[LGNPU_REG_CTRL / sizeof(uint32_t)], LGNPU_CTRL_SOFT_RESET);

    TEST_PASS();
}

static void test_reset_null(void)
{
    TEST_BEGIN("reset: null dev");

    ASSERT_EQ(npu_reset(NULL), -1);

    TEST_PASS();
}

static void test_enable(void)
{
    TEST_BEGIN("enable: writes CTRL_ENABLE");

    struct mock_device mock;
    mock_device_init(&mock);

    struct npu_device dev;
    mock_setup_device(&mock, &dev);

    ASSERT_EQ(npu_enable(&dev), 0);
    ASSERT_EQ(mock.regs[LGNPU_REG_CTRL / sizeof(uint32_t)], LGNPU_CTRL_ENABLE);

    TEST_PASS();
}

static void test_enable_null(void)
{
    TEST_BEGIN("enable: null dev");

    ASSERT_EQ(npu_enable(NULL), -1);

    TEST_PASS();
}

/* Status queries */

static void test_read_status(void)
{
    TEST_BEGIN("read_status: parses idle/busy/queue_full");

    struct mock_device mock;
    mock_device_init(&mock);

    struct npu_device dev;
    mock_setup_device(&mock, &dev);

    mock.regs[LGNPU_REG_STATUS / sizeof(uint32_t)] =
        LGNPU_STATUS_IDLE | LGNPU_STATUS_BUSY | LGNPU_STATUS_QUEUE_FULL;

    struct npu_status st;
    ASSERT_EQ(npu_read_status(&dev, &st), 0);
    ASSERT_TRUE(st.idle);
    ASSERT_TRUE(st.busy);
    ASSERT_TRUE(st.queue_full);

    /* Clear all bits */
    mock.regs[LGNPU_REG_STATUS / sizeof(uint32_t)] = 0;
    ASSERT_EQ(npu_read_status(&dev, &st), 0);
    ASSERT_TRUE(!st.idle);
    ASSERT_TRUE(!st.busy);
    ASSERT_TRUE(!st.queue_full);

    TEST_PASS();
}

static void test_read_status_null(void)
{
    TEST_BEGIN("read_status: null pointers");

    struct mock_device mock;
    mock_device_init(&mock);

    struct npu_device dev;
    mock_setup_device(&mock, &dev);

    struct npu_status st;
    ASSERT_EQ(npu_read_status(NULL, &st), -1);
    ASSERT_EQ(npu_read_status(&dev, NULL), -1);

    TEST_PASS();
}

static void test_read_info(void)
{
    TEST_BEGIN("read_info: decodes FEATURE_ID");

    struct mock_device mock;
    mock_device_init(&mock);

    struct npu_device dev;
    mock_setup_device(&mock, &dev);

    /* v1.0, 6 backends, 8x8 array, 1 dtype (set by mock_device_init) */
    struct npu_device_info info;
    ASSERT_EQ(npu_read_info(&dev, &info), 0);
    ASSERT_EQ(info.version_major, 1);
    ASSERT_EQ(info.version_minor, 0);
    ASSERT_EQ(info.num_backends, 6);
    ASSERT_EQ(info.array_rows, 8);
    ASSERT_EQ(info.array_cols, 8);
    ASSERT_EQ(info.dtypes_supported, 1);

    TEST_PASS();
}

static void test_read_info_null(void)
{
    TEST_BEGIN("read_info: null pointers");

    struct mock_device mock;
    mock_device_init(&mock);

    struct npu_device dev;
    mock_setup_device(&mock, &dev);

    struct npu_device_info info;
    ASSERT_EQ(npu_read_info(NULL, &info), -1);
    ASSERT_EQ(npu_read_info(&dev, NULL), -1);

    TEST_PASS();
}

static void test_poll_idle_immediate(void)
{
    TEST_BEGIN("poll_idle: device already idle");

    struct mock_device mock;
    mock_device_init(&mock);

    struct npu_device dev;
    mock_setup_device(&mock, &dev);

    /* Mock starts IDLE */
    ASSERT_EQ(npu_poll_idle(&dev, 100), 0);

    TEST_PASS();
}

static void test_poll_idle_timeout(void)
{
    TEST_BEGIN("poll_idle: timeout when busy");

    struct mock_device mock;
    mock_device_init(&mock);

    struct npu_device dev;
    mock_setup_device(&mock, &dev);

    mock.regs[LGNPU_REG_STATUS / sizeof(uint32_t)] = LGNPU_STATUS_BUSY;
    ASSERT_EQ(npu_poll_idle(&dev, 10), -1);

    TEST_PASS();
}

static void test_poll_idle_null(void)
{
    TEST_BEGIN("poll_idle: null dev");

    ASSERT_EQ(npu_poll_idle(NULL, 100), -1);

    TEST_PASS();
}

/* IRQ management */

static void test_irq_enable(void)
{
    TEST_BEGIN("irq_enable: writes IRQ_ENABLE=1");

    struct mock_device mock;
    mock_device_init(&mock);

    struct npu_device dev;
    mock_setup_device(&mock, &dev);

    ASSERT_EQ(npu_irq_enable(&dev), 0);
    ASSERT_EQ(mock.regs[LGNPU_REG_IRQ_ENABLE / sizeof(uint32_t)], 1u);

    TEST_PASS();
}

static void test_irq_disable(void)
{
    TEST_BEGIN("irq_disable: writes IRQ_ENABLE=0");

    struct mock_device mock;
    mock_device_init(&mock);

    struct npu_device dev;
    mock_setup_device(&mock, &dev);

    mock.regs[LGNPU_REG_IRQ_ENABLE / sizeof(uint32_t)] = 1;
    ASSERT_EQ(npu_irq_disable(&dev), 0);
    ASSERT_EQ(mock.regs[LGNPU_REG_IRQ_ENABLE / sizeof(uint32_t)], 0u);

    TEST_PASS();
}

static void test_irq_clear(void)
{
    TEST_BEGIN("irq_clear: writes IRQ_CLEAR=1");

    struct mock_device mock;
    mock_device_init(&mock);

    struct npu_device dev;
    mock_setup_device(&mock, &dev);

    ASSERT_EQ(npu_irq_clear(&dev), 0);
    ASSERT_EQ(mock.regs[LGNPU_REG_IRQ_CLEAR / sizeof(uint32_t)], 1u);

    TEST_PASS();
}

static void test_irq_pending(void)
{
    TEST_BEGIN("irq_pending: reads IRQ_STATUS");

    struct mock_device mock;
    mock_device_init(&mock);

    struct npu_device dev;
    mock_setup_device(&mock, &dev);

    mock.regs[LGNPU_REG_IRQ_STATUS / sizeof(uint32_t)] = 0;
    ASSERT_EQ(npu_irq_pending(&dev), 0);

    mock.regs[LGNPU_REG_IRQ_STATUS / sizeof(uint32_t)] = 1;
    ASSERT_EQ(npu_irq_pending(&dev), 1);

    TEST_PASS();
}

static void test_irq_null(void)
{
    TEST_BEGIN("irq_*: null dev returns -1 or 0");

    ASSERT_EQ(npu_irq_enable(NULL), -1);
    ASSERT_EQ(npu_irq_disable(NULL), -1);
    ASSERT_EQ(npu_irq_clear(NULL), -1);
    /* irq_pending returns 0 on null (no pending) */
    ASSERT_EQ(npu_irq_pending(NULL), 0);

    TEST_PASS();
}

/* Performance counters */

static void test_perf_counters(void)
{
    TEST_BEGIN("perf: reads cycle/active/stall counters");

    struct mock_device mock;
    mock_device_init(&mock);

    struct npu_device dev;
    mock_setup_device(&mock, &dev);

    mock.regs[LGNPU_REG_PERF_CYCLES / sizeof(uint32_t)] = 1000;
    mock.regs[LGNPU_REG_PERF_ACTIVE / sizeof(uint32_t)] = 800;
    mock.regs[LGNPU_REG_PERF_STALL  / sizeof(uint32_t)] = 200;

    ASSERT_EQ(npu_perf_read_cycles(&dev), 1000u);
    ASSERT_EQ(npu_perf_read_active(&dev), 800u);
    ASSERT_EQ(npu_perf_read_stall(&dev),  200u);

    TEST_PASS();
}

static void test_perf_null(void)
{
    TEST_BEGIN("perf: null dev returns 0");

    ASSERT_EQ(npu_perf_read_cycles(NULL), 0u);
    ASSERT_EQ(npu_perf_read_active(NULL), 0u);
    ASSERT_EQ(npu_perf_read_stall(NULL), 0u);

    TEST_PASS();
}

/* Submit */

static void test_submit_writes_queue(void)
{
    TEST_BEGIN("submit: writes descriptor to CMD_QUEUE_BASE");

    struct mock_device mock;
    mock_device_init(&mock);

    struct npu_device dev;
    mock_setup_device(&mock, &dev);

    struct npu_cmd_desc desc;
    __builtin_memset(&desc, 0, sizeof(desc));

    desc.words[0] = (uint32_t)LGNPU_OP_CONV;
    desc.words[1] = 0xAAAA;

    ASSERT_EQ(npu_submit(&dev, &desc), 0);

    const uint32_t q_base = LGNPU_CMD_QUEUE_BASE / sizeof(uint32_t);
    ASSERT_EQ(mock.regs[q_base + 0], (uint32_t)LGNPU_OP_CONV);
    ASSERT_EQ(mock.regs[q_base + 1], 0xAAAAu);

    /* Doorbell was rung */
    ASSERT_EQ(mock.regs[LGNPU_REG_DOORBELL / sizeof(uint32_t)], 1u);

    TEST_PASS();
}

static void test_submit_null(void)
{
    TEST_BEGIN("submit: null pointers");

    struct mock_device mock;
    mock_device_init(&mock);

    struct npu_device dev;
    mock_setup_device(&mock, &dev);

    struct npu_cmd_desc desc;
    __builtin_memset(&desc, 0, sizeof(desc));

    ASSERT_EQ(npu_submit(NULL, &desc), -1);
    ASSERT_EQ(npu_submit(&dev, NULL), -1);

    TEST_PASS();
}

int main(void)
{
    SUITE_BEGIN("Device Control");

    test_open_null_dev();
    test_open_null_base();
    test_open_close();
    test_close_null();

    test_reset();
    test_reset_null();
    test_enable();
    test_enable_null();

    test_read_status();
    test_read_status_null();
    test_read_info();
    test_read_info_null();
    test_poll_idle_immediate();
    test_poll_idle_timeout();
    test_poll_idle_null();

    test_irq_enable();
    test_irq_disable();
    test_irq_clear();
    test_irq_pending();
    test_irq_null();

    test_perf_counters();
    test_perf_null();

    test_submit_writes_queue();
    test_submit_null();

    SUITE_END();

    return SUITE_RESULT();
}
