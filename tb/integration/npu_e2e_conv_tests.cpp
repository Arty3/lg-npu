// ============================================================================
// npu_e2e_conv_tests.cpp - Deterministic convolution E2E test suite
//
// Tests different input sizes, C/K values, kernel sizes, stride/padding
// combinations, value ranges, and saturation edge cases.
// ============================================================================

#include "npu_tb.h"
#include <cstdio>

// Helper: fill a vector with a sequential pattern [start, start+1, ...]
// wrapping within [-128, 127]
static std::vector<int8_t> seq_fill(int count, int8_t start = 1)
{
    std::vector<int8_t> v(count);
    for (int i = 0; i < count; ++i)
        v[i] = static_cast<int8_t>((start + i) % 256);
    return v;
}

// Helper: fill with a constant value
static std::vector<int8_t> const_fill(int count, int8_t val)
{
    return std::vector<int8_t>(count, val);
}

// Helper: fill with alternating +val / -val
static std::vector<int8_t> alt_fill(int count, int8_t pos, int8_t neg)
{
    std::vector<int8_t> v(count);
    for (int i = 0; i < count; ++i)
        v[i] = (i % 2 == 0) ? pos : neg;
    return v;
}

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);
    TestResult results;

    printf("=== E2E Convolution Test Suite ===\n\n");

    // Test 1: Original 4x4x1, 3x3, stride=1, pad=0 (sanity)
    {
        NpuTb tb("sim/waves/e2e_conv.vcd");
        tb.reset();
        tb.enable();

        auto act = seq_fill(4 * 4 * 1, 1);
        auto wt  = const_fill(1 * 3 * 3 * 1, 1);
        std::vector<int8_t> bias;

        results.record(tb.run_conv_test(
            "4x4x1 k3 s1 p0",
            act, wt, bias,
            4, 4, 1, 1, 3, 3,
            1, 1, 0, 0, 0,
            0, 0, 0, 4096));
    }

    // Test 2: 5x5x1, 3x3, stride=1, pad=0 -> 3x3 output
    {
        NpuTb tb;
        tb.reset();
        tb.enable();

        auto act = seq_fill(5 * 5 * 1, 1);
        auto wt  = const_fill(1 * 3 * 3 * 1, 1);
        std::vector<int8_t> bias;

        results.record(tb.run_conv_test(
            "5x5x1 k3 s1 p0",
            act, wt, bias,
            5, 5, 1, 1, 3, 3,
            1, 1, 0, 0, 0));
    }

    // Test 3: 4x4x2 input, C=2, K=1, 3x3 filter
    {
        NpuTb tb;
        tb.reset();
        tb.enable();

        auto act = seq_fill(4 * 4 * 2, 1);
        auto wt  = const_fill(1 * 3 * 3 * 2, 1);
        std::vector<int8_t> bias;

        results.record(tb.run_conv_test(
            "4x4x2 k3 s1 p0 (multi-channel)",
            act, wt, bias,
            4, 4, 2, 1, 3, 3,
            1, 1, 0, 0, 0,
            0, 0, 0, 2048));
    }

    // Test 4: 4x4x1, K=2, 3x3 filter (multi-filter)
    {
        NpuTb tb;
        tb.reset();
        tb.enable();

        auto act = seq_fill(4 * 4 * 1, 1);
        // K=2: first filter all-1, second filter all-2
        std::vector<int8_t> wt(2 * 3 * 3 * 1);
        for (int i = 0; i < 9; ++i) wt[i] = 1;
        for (int i = 9; i < 18; ++i) wt[i] = 2;
        std::vector<int8_t> bias;

        results.record(tb.run_conv_test(
            "4x4x1 K=2 k3 s1 p0 (multi-filter)",
            act, wt, bias,
            4, 4, 1, 2, 3, 3,
            1, 1, 0, 0, 0));
    }

    // Test 5: 6x6x1, 3x3, stride=2, pad=0 -> 2x2 output
    {
        NpuTb tb;
        tb.reset();
        tb.enable();

        auto act = seq_fill(6 * 6 * 1, 1);
        auto wt  = const_fill(1 * 3 * 3 * 1, 1);
        std::vector<int8_t> bias;

        results.record(tb.run_conv_test(
            "6x6x1 k3 s2 p0 (strided)",
            act, wt, bias,
            6, 6, 1, 1, 3, 3,
            2, 2, 0, 0, 0));
    }

    // Test 6: 3x3x1, 3x3, stride=1, pad=1 -> 3x3 output (padded)
    {
        NpuTb tb;
        tb.reset();
        tb.enable();

        auto act = seq_fill(3 * 3 * 1, 1);
        auto wt  = const_fill(1 * 3 * 3 * 1, 1);
        std::vector<int8_t> bias;

        results.record(tb.run_conv_test(
            "3x3x1 k3 s1 p1 (padded)",
            act, wt, bias,
            3, 3, 1, 1, 3, 3,
            1, 1, 1, 1, 0));
    }

    // Test 7: 4x4x1, 1x1 filter (pointwise convolution)
    {
        NpuTb tb;
        tb.reset();
        tb.enable();

        auto act = seq_fill(4 * 4 * 1, 1);
        auto wt  = const_fill(1 * 1 * 1 * 1, 3);
        std::vector<int8_t> bias;

        results.record(tb.run_conv_test(
            "4x4x1 k1 s1 p0 (pointwise)",
            act, wt, bias,
            4, 4, 1, 1, 1, 1,
            1, 1, 0, 0, 0));
    }

    // Test 8: Zero input -> zero output
    {
        NpuTb tb;
        tb.reset();
        tb.enable();

        auto act = const_fill(4 * 4 * 1, 0);
        auto wt  = const_fill(1 * 3 * 3 * 1, 5);
        std::vector<int8_t> bias;

        results.record(tb.run_conv_test(
            "4x4x1 zero input",
            act, wt, bias,
            4, 4, 1, 1, 3, 3,
            1, 1, 0, 0, 0));
    }

    // Test 9: Max positive values (saturation test)
    //   9 * 127 * 127 = 145161 -> shift right 11 -> 70 (within INT8)
    {
        NpuTb tb;
        tb.reset();
        tb.enable();

        auto act = const_fill(4 * 4 * 1, 127);
        auto wt  = const_fill(1 * 3 * 3 * 1, 127);
        std::vector<int8_t> bias;

        results.record(tb.run_conv_test(
            "4x4x1 max positive (qshift=11)",
            act, wt, bias,
            4, 4, 1, 1, 3, 3,
            1, 1, 0, 0, 11));
    }

    // Test 10: Negative weights, positive activations -> negative acc -> ReLU clamps to 0
    {
        NpuTb tb;
        tb.reset();
        tb.enable();

        auto act = const_fill(4 * 4 * 1, 10);
        auto wt  = const_fill(1 * 3 * 3 * 1, -5);
        std::vector<int8_t> bias;
        // acc = 9 * 10 * (-5) = -450, ReLU -> 0

        results.record(tb.run_conv_test(
            "4x4x1 negative accum (ReLU clamp)",
            act, wt, bias,
            4, 4, 1, 1, 3, 3,
            1, 1, 0, 0, 0));
    }

    results.summary("E2E Convolution Tests");
    return results.exit_code();
}
