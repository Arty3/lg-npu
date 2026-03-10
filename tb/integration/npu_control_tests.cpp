// ============================================================================
// npu_control_tests.cpp - Control-flow, error handling, and sequencing tests
//
// Tests: bad opcode, reset during operation, busy/done sequencing,
//        queue-full detection, command handshake, output-only-when-valid,
//        back-to-back commands.
// ============================================================================

#include "npu_tb.h"
#include <cstdio>

// A small known-good convolution for use as a baseline within tests
static bool run_small_conv(NpuTb &tb, const char *label)
{
    auto act = std::vector<int8_t>{1,2,3,4, 5,6,7,8, 9,10,11,12, 13,14,15,16};
    auto wt  = std::vector<int8_t>(9, 1);
    std::vector<int8_t> bias;
    return tb.run_conv_test(label, act, wt, bias,
                            4, 4, 1, 1, 3, 3,
                            1, 1, 0, 0, 0);
}

// Test: bad opcode
static bool test_bad_opcode(TestResult &r)
{
    printf("[test] bad_opcode\n");
    NpuTb tb;
    tb.reset();
    tb.enable();

    // Submit a command with opcode=0 (invalid, only OP_CONV=1 is valid)
    uint32_t words[16] = {};
    words[0] = 0;  // bad opcode
    for (int i = 0; i < 16; ++i)
        tb.mmio_write(CMD_QUEUE_BASE + static_cast<uint32_t>(i) * 4, words[i]);
    tb.mmio_write(REG_DOORBELL, 1);

    // Give it time to process
    tb.run_clocks(200);

    // IRQ should fire (decode_err triggers IRQ)
    uint32_t irq_st = tb.mmio_read(REG_IRQ_STATUS);
    bool pass = (irq_st & 1) != 0;
    if (!pass)
        printf("  [FAIL] bad_opcode: IRQ not set (irq_status=0x%x)\n", irq_st);

    // NPU should return to idle (bad command is discarded)
    bool idle = tb.wait_idle(1000);
    if (!idle)
    {
        printf("  [FAIL] bad_opcode: NPU did not return to idle\n");
        pass = false;
    }

    // Acknowledge IRQ
    tb.mmio_write(REG_IRQ_CLEAR, 1);
    uint32_t irq_after = tb.mmio_read(REG_IRQ_STATUS);
    if (irq_after & 1)
    {
        printf("  [FAIL] bad_opcode: IRQ not cleared after write to IRQ_CLEAR\n");
        pass = false;
    }

    // Enable IRQ and resubmit bad opcode to verify irq_out pin asserts
    tb.mmio_write(REG_IRQ_ENABLE, 1);
    for (int i = 0; i < 16; ++i)
        tb.mmio_write(CMD_QUEUE_BASE + static_cast<uint32_t>(i) * 4, 0);
    tb.mmio_write(REG_DOORBELL, 1);
    tb.run_clocks(200);
    if (!tb.raw()->irq)
    {
        printf("  [FAIL] bad_opcode: irq_out pin not asserted with IRQ_ENABLE=1\n");
        pass = false;
    }
    tb.mmio_write(REG_IRQ_CLEAR, 1);

    if (pass) printf("  [PASS] bad_opcode\n");
    r.record(pass);
    return pass;
}

// Test: reset during active operation
static bool test_reset_during_operation(TestResult &r)
{
    printf("[test] reset_during_operation\n");
    NpuTb tb;
    tb.reset();
    tb.enable();

    // Load data for a convolution
    auto act = std::vector<int8_t>(25, 5);  // 5x5
    auto wt  = std::vector<int8_t>(9, 1);   // 3x3
    tb.load_bytes(WEIGHT_BUF_BASE, wt);
    tb.load_bytes(ACT_BUF_BASE, act);

    // Submit command
    ConvDesc desc{};
    desc.opcode       = 1;
    desc.act_in_addr  = 0;
    desc.act_out_addr = 2048;
    desc.in_h = 5; desc.in_w = 5; desc.in_c = 1;
    desc.out_k = 1; desc.filt_r = 3; desc.filt_s = 3;
    desc.stride_h = 1; desc.stride_w = 1;
    tb.submit_conv(desc);

    // Let it start processing (but don't wait for completion)
    tb.run_clocks(50);

    // Fire soft reset
    tb.soft_reset_pulse();

    // After soft reset, should be idle
    bool pass = tb.is_idle();
    if (!pass)
        printf("  [FAIL] reset_during_operation: not idle after soft reset\n");

    // Should be able to run another convolution cleanly
    if (pass)
    {
        tb.load_bytes(WEIGHT_BUF_BASE, wt);
        tb.load_bytes(ACT_BUF_BASE, act);
        pass = tb.run_conv_test(
            "reset_during_operation (post-reset conv)",
            act, wt, {}, 5, 5, 1, 1, 3, 3, 1, 1, 0, 0, 0);
    }

    if (pass) printf("  [PASS] reset_during_operation\n");
    r.record(pass);
    return pass;
}

// Test: busy/done sequencing is sane
//   After doorbell, BUSY should go high before IDLE.
//   After completion, IDLE should be high and BUSY low.
static bool test_busy_done_sequencing(TestResult &r)
{
    printf("[test] busy_done_sequencing\n");
    NpuTb tb;
    tb.reset();
    tb.enable();

    // Confirm starts idle
    bool pass = tb.is_idle() && !tb.is_busy();
    if (!pass)
    {
        printf("  [FAIL] busy_done_sequencing: not idle at start\n");
        r.record(false);
        return false;
    }

    // Load a small conv
    auto act = std::vector<int8_t>{1,2,3,4, 5,6,7,8, 9,10,11,12, 13,14,15,16};
    auto wt  = std::vector<int8_t>(9, 1);
    tb.load_bytes(WEIGHT_BUF_BASE, wt);
    tb.load_bytes(ACT_BUF_BASE, act);

    ConvDesc desc{};
    desc.opcode = 1;
    desc.act_out_addr = 2048;
    desc.in_h = 4; desc.in_w = 4; desc.in_c = 1;
    desc.out_k = 1; desc.filt_r = 3; desc.filt_s = 3;
    desc.stride_h = 1; desc.stride_w = 1;

    // Write command words + doorbell manually (no free-run)
    uint32_t words[16];
    desc.to_words(words);
    for (int i = 0; i < 16; ++i)
        tb.mmio_write(CMD_QUEUE_BASE + static_cast<uint32_t>(i) * 4, words[i]);
    tb.mmio_write(REG_DOORBELL, 1);

    // Immediately after doorbell, check that we transition out of idle
    // (Use free-running ticks so MMIO polling doesn't stall the pipeline)
    bool saw_not_idle = false;
    for (int i = 0; i < 2000; ++i)
    {
        tb.tick();
        if (!tb.is_idle())
        {
            saw_not_idle = true;
            break;
        }
    }

    if (!saw_not_idle)
    {
        printf("  [FAIL] busy_done_sequencing: never left idle after doorbell\n");
        pass = false;
    }

    // Free-run to let compute finish without MMIO interference
    tb.run_clocks(2000);

    // Wait for idle again
    if (!tb.wait_idle())
    {
        printf("  [FAIL] busy_done_sequencing: timeout waiting for idle\n");
        pass = false;
    }

    // After completion: idle=1, busy=0
    uint32_t final_st = tb.read_status();
    bool final_idle = (final_st >> STATUS_IDLE_BIT) & 1;
    bool final_busy = (final_st >> STATUS_BUSY_BIT) & 1;
    if (!final_idle || final_busy)
    {
        printf("  [FAIL] busy_done_sequencing: post-completion status=0x%x (idle=%d busy=%d)\n",
               final_st, final_idle, final_busy);
        pass = false;
    }

    if (pass) printf("  [PASS] busy_done_sequencing\n");
    r.record(pass);
    return pass;
}

// Test: back-to-back commands
//   Submit two convolutions in sequence without resetting.
static bool test_back_to_back(TestResult &r)
{
    printf("[test] back_to_back\n");
    NpuTb tb;
    tb.reset();
    tb.enable();

    // Command 1: 4x4x1, 3x3, all-1 weights
    auto act1 = std::vector<int8_t>{1,2,3,4, 5,6,7,8, 9,10,11,12, 13,14,15,16};
    auto wt1  = std::vector<int8_t>(9, 1);
    bool p1 = tb.run_conv_test(
        "back_to_back cmd1",
        act1, wt1, {}, 4, 4, 1, 1, 3, 3, 1, 1, 0, 0, 0);

    // Command 2: same size, all-2 weights -> outputs should double
    auto wt2 = std::vector<int8_t>(9, 2);
    bool p2 = tb.run_conv_test(
        "back_to_back cmd2",
        act1, wt2, {}, 4, 4, 1, 1, 3, 3, 1, 1, 0, 0, 0);

    bool pass = p1 && p2;
    if (pass) printf("  [PASS] back_to_back\n");
    r.record(pass);
    return pass;
}

// Test: hard reset clears expected state
static bool test_hard_reset_clears_state(TestResult &r)
{
    printf("[test] hard_reset_clears_state\n");
    NpuTb tb;
    tb.reset();
    tb.enable();

    // Write some MMIO state
    tb.mmio_write(REG_IRQ_ENABLE, 1);

    // Hard reset
    tb.reset();

    // After hard reset, status should be idle, IRQ enable should be 0
    uint32_t st = tb.read_status();
    bool idle = (st >> STATUS_IDLE_BIT) & 1;
    bool pass = idle;

    // IRQ enable should be cleared
    uint32_t irq_en = tb.mmio_read(REG_IRQ_ENABLE);
    if (irq_en != 0)
    {
        printf("  [FAIL] hard_reset_clears_state: IRQ_ENABLE not cleared (0x%x)\n", irq_en);
        pass = false;
    }

    // CTRL should be cleared
    uint32_t ctrl = tb.mmio_read(REG_CTRL);
    if (ctrl != 0)
    {
        printf("  [FAIL] hard_reset_clears_state: CTRL not cleared (0x%x)\n", ctrl);
        pass = false;
    }

    // Perf counters: cycles counter ticks every clock, so it will be non-zero
    // by the time we read it. Active and stall should be zero (no backend work).
    uint32_t perf_act = tb.mmio_read(REG_PERF_ACTIVE);
    uint32_t perf_stl = tb.mmio_read(REG_PERF_STALL);
    if (perf_act != 0 || perf_stl != 0)
    {
        printf("  [FAIL] hard_reset_clears_state: perf active/stall not zero "
               "(act=%u stl=%u)\n", perf_act, perf_stl);
        pass = false;
    }

    // IRQ_STATUS should be cleared (no pending interrupt)
    uint32_t irq_st = tb.mmio_read(REG_IRQ_STATUS);
    if (irq_st != 0)
    {
        printf("  [FAIL] hard_reset_clears_state: IRQ_STATUS not cleared (0x%x)\n", irq_st);
        pass = false;
    }

    // STATUS should show idle, not busy, not queue-full
    uint32_t st2 = tb.read_status();
    bool qfull = (st2 >> STATUS_QUEUE_FULL_BIT) & 1;
    bool busy  = (st2 >> STATUS_BUSY_BIT) & 1;
    if (qfull || busy)
    {
        printf("  [FAIL] hard_reset_clears_state: STATUS has busy=%d qfull=%d\n",
               busy, qfull);
        pass = false;
    }

    if (pass) printf("  [PASS] hard_reset_clears_state\n");
    r.record(pass);
    return pass;
}

// Test: output writes only occur during valid computation
//   Run a convolution, verify the output area before and after differs
//   only at expected locations.
static bool test_output_writes_only_when_valid(TestResult &r)
{
    printf("[test] output_writes_only_when_valid\n");
    NpuTb tb;
    tb.reset();
    tb.enable();

    // Pre-fill output area with a sentinel value (0x55 = 85)
    uint32_t out_addr = 2048;
    int sentinel_count = 8;  // fill more than needed
    for (int i = 0; i < sentinel_count; ++i)
        tb.mmio_write(ACT_BUF_BASE + out_addr + static_cast<uint32_t>(i), 0x55);

    // Run a 4x4x1 k3 -> 2x2 = 4 outputs
    auto act = std::vector<int8_t>{1,2,3,4, 5,6,7,8, 9,10,11,12, 13,14,15,16};
    auto wt  = std::vector<int8_t>(9, 1);
    auto expected = ref_conv(act, wt, {}, 4, 4, 1, 1, 3, 3, 1, 1, 0, 0, 0);
    int out_count = static_cast<int>(expected.size()); // 4

    tb.load_bytes(WEIGHT_BUF_BASE, wt);
    tb.load_bytes(ACT_BUF_BASE, act);

    ConvDesc desc{};
    desc.opcode = 1;
    desc.act_out_addr = out_addr;
    desc.in_h = 4; desc.in_w = 4; desc.in_c = 1;
    desc.out_k = 1; desc.filt_r = 3; desc.filt_s = 3;
    desc.stride_h = 1; desc.stride_w = 1;
    tb.submit_conv(desc);
    tb.wait_idle();

    // Read back: first 4 should match expected, remaining should be sentinel
    auto got = tb.read_bytes(ACT_BUF_BASE + out_addr, sentinel_count);
    bool pass = true;
    for (int i = 0; i < out_count; ++i)
    {
        if (got[i] != expected[i])
        {
            printf("  [FAIL] output[%d] expected %d, got %d\n", i, expected[i], got[i]);
            pass = false;
        }
    }
    for (int i = out_count; i < sentinel_count; ++i)
    {
        if (got[i] != static_cast<int8_t>(0x55))
        {
            printf("  [FAIL] sentinel[%d] was overwritten: got %d (expected 0x55)\n",
                   i, got[i]);
            pass = false;
        }
    }

    if (pass) printf("  [PASS] output_writes_only_when_valid\n");
    r.record(pass);
    return pass;
}

// Test: IRQ fires on completion and can be cleared
static bool test_irq_completion(TestResult &r)
{
    printf("[test] irq_completion\n");
    NpuTb tb;
    tb.reset();
    tb.enable();

    // Enable IRQ
    tb.mmio_write(REG_IRQ_ENABLE, 1);

    // IRQ should not be pending yet
    uint32_t irq_pre = tb.mmio_read(REG_IRQ_STATUS);
    bool pass = (irq_pre == 0);
    if (!pass)
        printf("  [FAIL] irq_completion: IRQ pending before command (0x%x)\n", irq_pre);

    // Run a convolution
    auto act = std::vector<int8_t>{1,2,3,4, 5,6,7,8, 9,10,11,12, 13,14,15,16};
    auto wt  = std::vector<int8_t>(9, 1);
    tb.load_bytes(WEIGHT_BUF_BASE, wt);
    tb.load_bytes(ACT_BUF_BASE, act);

    ConvDesc desc{};
    desc.opcode = 1;
    desc.act_out_addr = 2048;
    desc.in_h = 4; desc.in_w = 4; desc.in_c = 1;
    desc.out_k = 1; desc.filt_r = 3; desc.filt_s = 3;
    desc.stride_h = 1; desc.stride_w = 1;
    tb.submit_conv(desc);
    tb.wait_idle();

    // IRQ should be pending now
    uint32_t irq_post = tb.mmio_read(REG_IRQ_STATUS);
    if (!(irq_post & 1))
    {
        printf("  [FAIL] irq_completion: IRQ not set after completion (0x%x)\n", irq_post);
        pass = false;
    }

    // Check irq output pin
    if (!tb.raw()->irq)
    {
        printf("  [FAIL] irq_completion: irq_out pin not asserted\n");
        pass = false;
    }

    // Clear IRQ
    tb.mmio_write(REG_IRQ_CLEAR, 1);
    tb.run_clocks(2);
    uint32_t irq_cleared = tb.mmio_read(REG_IRQ_STATUS);
    if (irq_cleared & 1)
    {
        printf("  [FAIL] irq_completion: IRQ not cleared (0x%x)\n", irq_cleared);
        pass = false;
    }

    if (pass) printf("  [PASS] irq_completion\n");
    r.record(pass);
    return pass;
}

// Test: feature ID readback
static bool test_feature_id(TestResult &r)
{
    printf("[test] feature_id\n");
    NpuTb tb;
    tb.reset();

    uint32_t fid = tb.mmio_read(REG_FEATURE_ID);
    // Expected: VER_MAJ=0x00, VER_MIN=0x01, BACKENDS=1, ROWS=1, COLS=1, DTYPES=1
    //           = 0x00_01_1_1_1_1 = 0x00011111
    constexpr uint32_t EXPECTED_FID = 0x00011111;
    bool pass = (fid == EXPECTED_FID);
    if (!pass)
    {
        printf("  [FAIL] feature_id: read 0x%08x (expected 0x%08x)\n", fid, EXPECTED_FID);
    }
    else
    {
        // Verify individual fields
        uint32_t ver_maj  = (fid >> 24) & 0xFF;
        uint32_t ver_min  = (fid >> 16) & 0xFF;
        uint32_t backends = (fid >> 12) & 0xF;
        uint32_t rows     = (fid >>  8) & 0xF;
        uint32_t cols     = (fid >>  4) & 0xF;
        uint32_t dtypes   = (fid >>  0) & 0xF;
        printf("  [PASS] feature_id: 0x%08x (v%u.%u, %u backend(s), %ux%u, dtypes=0x%x)\n",
               fid, ver_maj, ver_min, backends, rows, cols, dtypes);
    }

    r.record(pass);
    return pass;
}

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);
    TestResult results;

    printf("=== Control & Sequencing Test Suite ===\n\n");

    test_bad_opcode(results);
    test_reset_during_operation(results);
    test_busy_done_sequencing(results);
    test_back_to_back(results);
    test_hard_reset_clears_state(results);
    test_output_writes_only_when_valid(results);
    test_irq_completion(results);
    test_feature_id(results);

    results.summary("Control & Sequencing Tests");
    return results.exit_code();
}
