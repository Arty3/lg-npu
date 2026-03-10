// ============================================================================
// npu_perf_tests.cpp - Performance counter and throughput tests
//
// Tests: perf counters increment correctly, cycle count scales with
//        problem size, active/stall counters are sane.
// ============================================================================

#include "npu_tb.h"
#include <cstdio>

// Run a convolution and return perf counter snapshots
struct PerfSnapshot
{
    uint32_t cycles;
    uint32_t active;
    uint32_t stall;
};

static PerfSnapshot run_and_measure(NpuTb &tb,
                                    const std::vector<int8_t> &act,
                                    const std::vector<int8_t> &wt,
                                    int H, int W, int C, int K,
                                    int R, int S,
                                    int sh, int sw, int ph, int pw,
                                    int qshift)
{
    tb.load_bytes(WEIGHT_BUF_BASE, wt);
    tb.load_bytes(ACT_BUF_BASE, act);

    // Read perf counters before
    uint32_t cyc_pre  = tb.mmio_read(REG_PERF_CYCLES);
    uint32_t act_pre  = tb.mmio_read(REG_PERF_ACTIVE);
    uint32_t stl_pre  = tb.mmio_read(REG_PERF_STALL);

    ConvDesc desc{};
    desc.opcode       = 1;
    desc.act_out_addr = 2048;
    desc.in_h = static_cast<uint32_t>(H);
    desc.in_w = static_cast<uint32_t>(W);
    desc.in_c = static_cast<uint32_t>(C);
    desc.out_k = static_cast<uint32_t>(K);
    desc.filt_r = static_cast<uint32_t>(R);
    desc.filt_s = static_cast<uint32_t>(S);
    desc.stride_h = static_cast<uint32_t>(sh);
    desc.stride_w = static_cast<uint32_t>(sw);
    desc.pad_h = static_cast<uint32_t>(ph);
    desc.pad_w = static_cast<uint32_t>(pw);
    desc.quant_shift = static_cast<uint32_t>(qshift);
    tb.submit_conv(desc);
    tb.wait_idle();

    // Read perf counters after
    uint32_t cyc_post = tb.mmio_read(REG_PERF_CYCLES);
    uint32_t act_post = tb.mmio_read(REG_PERF_ACTIVE);
    uint32_t stl_post = tb.mmio_read(REG_PERF_STALL);

    return PerfSnapshot
    {
        cyc_post - cyc_pre,
        act_post - act_pre,
        stl_post - stl_pre
    };
}

// Test: perf counters increment
static bool test_perf_counters_increment(TestResult &r)
{
    printf("[test] perf_counters_increment\n");
    NpuTb tb;
    tb.reset();
    tb.enable();

    auto act = std::vector<int8_t>{1,2,3,4, 5,6,7,8, 9,10,11,12, 13,14,15,16};
    auto wt  = std::vector<int8_t>(9, 1);

    auto snap = run_and_measure(tb, act, wt, 4, 4, 1, 1, 3, 3, 1, 1, 0, 0, 0);

    printf("    cycles=%u  active=%u  stall=%u\n", snap.cycles, snap.active, snap.stall);

    bool pass = true;
    if (snap.cycles == 0)
    {
        printf("  [FAIL] perf_counters_increment: cycles counter did not increment\n");
        pass = false;
    }
    // Active should be non-zero (we did compute work)
    if (snap.active == 0)
    {
        printf("  [FAIL] perf_counters_increment: active counter is 0\n");
        pass = false;
    }
    // Active should not exceed cycles
    if (snap.active > snap.cycles)
    {
        printf("  [FAIL] perf_counters_increment: active > cycles\n");
        pass = false;
    }

    if (pass) printf("  [PASS] perf_counters_increment\n");
    r.record(pass);
    return pass;
}

// Test: cycle count scales with problem size
//   A 5x5 conv should take more cycles than a 4x4 conv.
static bool test_perf_scales_with_size(TestResult &r)
{
    printf("[test] perf_scales_with_size\n");
    bool pass = true;

    // Small: 4x4x1 k=3 -> 2x2 = 4 output pixels, 36 MACs
    NpuTb tb1;
    tb1.reset();
    tb1.enable();
    auto act1 = std::vector<int8_t>(16, 1);
    auto wt1  = std::vector<int8_t>(9, 1);
    auto snap1 = run_and_measure(tb1, act1, wt1, 4, 4, 1, 1, 3, 3, 1, 1, 0, 0, 0);

    // Larger: 6x6x1 k=3 -> 4x4 = 16 output pixels, 144 MACs
    NpuTb tb2;
    tb2.reset();
    tb2.enable();
    auto act2 = std::vector<int8_t>(36, 1);
    auto wt2  = std::vector<int8_t>(9, 1);
    auto snap2 = run_and_measure(tb2, act2, wt2, 6, 6, 1, 1, 3, 3, 1, 1, 0, 0, 0);

    printf("    small: cycles=%u active=%u | large: cycles=%u active=%u\n",
           snap1.cycles, snap1.active, snap2.cycles, snap2.active);

    if (snap2.active <= snap1.active)
    {
        printf("  [FAIL] perf_scales_with_size: larger problem did not take more active cycles\n");
        pass = false;
    }

    if (pass) printf("  [PASS] perf_scales_with_size\n");
    r.record(pass);
    return pass;
}

// Test: soft reset clears perf counters
static bool test_perf_reset_clears(TestResult &r)
{
    printf("[test] perf_reset_clears\n");
    NpuTb tb;
    tb.reset();
    tb.enable();

    // Run a conv to get non-zero counters
    auto act = std::vector<int8_t>(16, 1);
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

    uint32_t cyc_pre = tb.mmio_read(REG_PERF_CYCLES);
    if (cyc_pre == 0)
    {
        printf("  [FAIL] perf_reset_clears: cycles counter is 0 before reset\n");
        r.record(false);
        return false;
    }

    // Soft reset
    tb.soft_reset_pulse();

    uint32_t cyc_post = tb.mmio_read(REG_PERF_CYCLES);
    uint32_t act_post = tb.mmio_read(REG_PERF_ACTIVE);
    uint32_t stl_post = tb.mmio_read(REG_PERF_STALL);

    bool pass = true;
    // After soft reset, counters should be very small (they may count
    // a few cycles during the reset window itself, so allow a small margin)
    if (cyc_post > 20)
    {
        printf("  [FAIL] perf_reset_clears: cycles not cleared (got %u)\n", cyc_post);
        pass = false;
    }
    if (act_post > 20)
    {
        printf("  [FAIL] perf_reset_clears: active not cleared (got %u)\n", act_post);
        pass = false;
    }
    if (stl_post > 20)
    {
        printf("  [FAIL] perf_reset_clears: stall not cleared (got %u)\n", stl_post);
        pass = false;
    }

    if (pass) printf("  [PASS] perf_reset_clears\n");
    r.record(pass);
    return pass;
}

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);
    TestResult results;

    printf("=== Performance Tests ===\n\n");

    test_perf_counters_increment(results);
    test_perf_scales_with_size(results);
    test_perf_reset_clears(results);

    results.summary("Performance Tests");
    return results.exit_code();
}
