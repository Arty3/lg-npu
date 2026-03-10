// npu_full_tests.cpp - Comprehensive regression suite for sim-full
//
// Sections:
//   1. Randomized convolution sweeps (seeded PRNG)
//   2. Edge-value / saturation distribution tests
//   3. Dimension sweeps (H, W, C, K, R, S, stride, pad)
//   4. Invalid-command combinations

#include "npu_tb.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <numeric>
#include <random>
#include <vector>

// Buffer constraints (from npu_cfg_pkg.sv)
static constexpr int ACT_BUF_BYTES    = 8192;
static constexpr int WEIGHT_BUF_BYTES = 4096;

// Compute output dimensions
static int out_dim(int in, int pad, int filt, int stride)
{
    return (in + 2 * pad - filt) / stride + 1;
}

// Check whether a conv configuration fits in hardware buffers.
// Returns true if input + output activations fit in ACT_BUF and
// weights fit in WEIGHT_BUF.
static bool fits(int H, int W, int C, int K, int R, int S,
                 int sh, int sw, int ph, int pw)
{
    int OH = out_dim(H, ph, R, sh);
    int OW = out_dim(W, pw, S, sw);
    if (OH <= 0 || OW <= 0) return false;
    int in_bytes  = H * W * C;
    int out_bytes = OH * OW * K;
    int wt_bytes  = K * R * S * C;
    if (in_bytes + out_bytes > ACT_BUF_BYTES) return false;
    if (wt_bytes > WEIGHT_BUF_BYTES) return false;
    return true;
}

// Fill helpers
static std::vector<int8_t> rand_fill(int count, std::mt19937 &rng)
{
    std::uniform_int_distribution<int> dist(-128, 127);
    std::vector<int8_t> v(count);
    for (auto &x : v) x = static_cast<int8_t>(dist(rng));
    return v;
}

static std::vector<int8_t> const_fill(int count, int8_t val)
{
    return std::vector<int8_t>(count, val);
}

static std::vector<int8_t> seq_fill(int count, int8_t start = 1)
{
    std::vector<int8_t> v(count);
    for (int i = 0; i < count; ++i)
        v[i] = static_cast<int8_t>((start + i) % 256);
    return v;
}

// Run a convolution through the harness and verify against C++ reference.
// Automatically places output after input in the activation buffer.
static bool run_test(
    NpuTb &tb,
    const char *name,
    const std::vector<int8_t> &act,
    const std::vector<int8_t> &wt,
    const std::vector<int8_t> &bias,
    int H, int W, int C, int K, int R, int S,
    int sh, int sw, int ph, int pw,
    int qshift)
{
    int in_bytes = H * W * C;
    // Place output right after input, aligned up to next 256-byte boundary
    uint32_t out_addr = static_cast<uint32_t>((in_bytes + 255) & ~255);
    uint32_t bias_addr = static_cast<uint32_t>(wt.size());

    return tb.run_conv_test(
        name, act, wt, bias,
        H, W, C, K, R, S,
        sh, sw, ph, pw, qshift,
        /*act_in_addr=*/0,
        /*wt_addr=*/0,
        /*bias_addr=*/bias_addr,
        /*act_out_addr=*/out_addr);
}

// Section 1: Randomized convolution sweeps
static void run_randomized_sweeps(TestResult &r, int count, uint32_t seed)
{
    printf("\n--- Randomized Conv Sweeps (seed=%u, n=%d) ---\n\n", seed, count);
    std::mt19937 rng(seed);

    int generated = 0;
    int attempts  = 0;
    while (generated < count && attempts < count * 50)
    {
        ++attempts;
        // Random dimensions within reasonable small ranges
        std::uniform_int_distribution<int> dim_dist(1, 16);
        std::uniform_int_distribution<int> chan_dist(1, 4);
        std::uniform_int_distribution<int> filt_dist(1, 5);
        std::uniform_int_distribution<int> stride_dist(1, 3);
        std::uniform_int_distribution<int> qshift_dist(0, 15);

        int H  = dim_dist(rng);
        int W  = dim_dist(rng);
        int C  = chan_dist(rng);
        int K  = chan_dist(rng);
        int R  = filt_dist(rng);
        int S  = filt_dist(rng);
        int sh = stride_dist(rng);
        int sw = stride_dist(rng);

        // Filter must be <= input dim
        if (R > H || S > W) continue;

        // Random padding 0..floor(filt/2)
        std::uniform_int_distribution<int> ph_dist(0, R / 2);
        std::uniform_int_distribution<int> pw_dist(0, S / 2);
        int ph = ph_dist(rng);
        int pw = pw_dist(rng);

        // Output dims must be positive
        int OH = out_dim(H, ph, R, sh);
        int OW = out_dim(W, pw, S, sw);
        if (OH <= 0 || OW <= 0) continue;

        // Must fit in buffers
        if (!fits(H, W, C, K, R, S, sh, sw, ph, pw)) continue;

        int qshift = qshift_dist(rng);

        // Generate random data
        // NOTE: bias is stubbed to 0 in npu_buffer_router.sv (TODO),
        // so we pass empty bias to match hardware behaviour.
        auto act  = rand_fill(H * W * C, rng);
        auto wt   = rand_fill(K * R * S * C, rng);
        std::vector<int8_t> bias;

        char name[128];
        snprintf(name, sizeof(name),
                 "rand[%d] %dx%dx%d K=%d %dx%d s=%d,%d p=%d,%d q=%d",
                 generated, H, W, C, K, R, S, sh, sw, ph, pw, qshift);

        NpuTb tb;
        tb.reset();
        tb.enable();
        r.record(run_test(tb, name, act, wt, bias,
                          H, W, C, K, R, S, sh, sw, ph, pw, qshift));
        ++generated;
    }
}

// Section 2: Edge-value / saturation distribution tests
static void run_edge_value_tests(TestResult &r)
{
    printf("\n--- Edge-Value Distribution Tests ---\n\n");

    // 2a: All max positive: act=127, wt=127, expect saturation
    for (int qshift : {0, 7, 11, 15})
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        auto act = const_fill(4 * 4 * 1, 127);
        auto wt  = const_fill(1 * 3 * 3 * 1, 127);
        char name[64];
        snprintf(name, sizeof(name), "max_pos q=%d", qshift);
        r.record(run_test(tb, name, act, wt, {},
                          4, 4, 1, 1, 3, 3, 1, 1, 0, 0, qshift));
    }

    // 2b: All min negative: act=-128, wt=-128
    // acc = 9 * (-128) * (-128) = 147456 (positive), needs large shift
    for (int qshift : {0, 10, 15})
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        auto act = const_fill(4 * 4 * 1, -128);
        auto wt  = const_fill(1 * 3 * 3 * 1, -128);
        char name[64];
        snprintf(name, sizeof(name), "min_neg q=%d", qshift);
        r.record(run_test(tb, name, act, wt, {},
                          4, 4, 1, 1, 3, 3, 1, 1, 0, 0, qshift));
    }

    // 2c: Mixed sign: act=127, wt=-128 -> negative accum -> ReLU clamps to 0
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        auto act = const_fill(4 * 4 * 1, 127);
        auto wt  = const_fill(1 * 3 * 3 * 1, -128);
        r.record(run_test(tb, "mix 127*-128 (ReLU->0)", act, wt, {},
                          4, 4, 1, 1, 3, 3, 1, 1, 0, 0, 0));
    }

    // 2d: Mixed sign: act=-128, wt=127 -> negative accum -> ReLU clamps to 0
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        auto act = const_fill(4 * 4 * 1, -128);
        auto wt  = const_fill(1 * 3 * 3 * 1, 127);
        r.record(run_test(tb, "mix -128*127 (ReLU->0)", act, wt, {},
                          4, 4, 1, 1, 3, 3, 1, 1, 0, 0, 0));
    }

    // 2e: Zero act, nonzero weight
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        r.record(run_test(tb, "zero_act nonzero_wt",
                          const_fill(4 * 4 * 1, 0),
                          const_fill(1 * 3 * 3 * 1, 99), {},
                          4, 4, 1, 1, 3, 3, 1, 1, 0, 0, 0));
    }

    // 2f: Nonzero act, zero weight
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        r.record(run_test(tb, "nonzero_act zero_wt",
                          const_fill(4 * 4 * 1, 99),
                          const_fill(1 * 3 * 3 * 1, 0), {},
                          4, 4, 1, 1, 3, 3, 1, 1, 0, 0, 0));
    }

    // 2g: Alternating +1/-1 weights, sequential act -> partial cancellation
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> wt = {1,-1,1, -1,1,-1, 1,-1,1};
        r.record(run_test(tb, "alternating_wt",
                          seq_fill(5 * 5 * 1, 1), wt, {},
                          5, 5, 1, 1, 3, 3, 1, 1, 0, 0, 0));
    }

    // 2h: Bias is NOT YET IMPLEMENTED in hardware (stubbed to 0 in
    //     npu_buffer_router.sv). When bias support is added, uncomment
    //     the tests below and remove this placeholder.
    //     { bias=+127 } and { bias=-128 (ReLU->0) }
    printf("  [SKIP] bias tests (not yet implemented in HW)\n");

    // 2i: Large positive accumulation with different quant shifts
    // C=4, K=1, 3x3: acc_max = 9 * 4 * 127 * 127 = 580644
    // Need qshift >= 13 to fit in INT8 (580644 >> 13 = 70)
    for (int C_val : {2, 4})
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        char name[64];
        snprintf(name, sizeof(name), "large_accum C=%d q=13", C_val);
        r.record(run_test(tb, name,
                          const_fill(4 * 4 * C_val, 127),
                          const_fill(1 * 3 * 3 * C_val, 127), {},
                          4, 4, C_val, 1, 3, 3, 1, 1, 0, 0, 13));
    }

    // 2j: quant_shift=0 (no shift, raw accumulator truncated)
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        // Small values so accum fits in INT8 without shift
        r.record(run_test(tb, "qshift=0 small",
                          const_fill(3 * 3 * 1, 1),
                          const_fill(1 * 1 * 1 * 1, 3), {},
                          3, 3, 1, 1, 1, 1, 1, 1, 0, 0, 0));
    }

    // 2k: quant_shift max (31 is field max at 5 bits)
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        r.record(run_test(tb, "qshift=31 (max)",
                          const_fill(4 * 4 * 1, 127),
                          const_fill(1 * 3 * 3 * 1, 127), {},
                          4, 4, 1, 1, 3, 3, 1, 1, 0, 0, 31));
    }
}

// Section 3: Dimension sweeps
static void run_dimension_sweeps(TestResult &r)
{
    printf("\n--- Dimension Sweeps ---\n\n");

    // 3a: Sweep H and W from minimum to larger sizes
    //     1x1 with 1x1 filter is the degenerate minimum
    for (int sz : {1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 16})
    {
        int R = std::min(sz, 3);
        if (!fits(sz, sz, 1, 1, R, R, 1, 1, 0, 0)) continue;

        NpuTb tb;
        tb.reset();
        tb.enable();
        char name[64];
        snprintf(name, sizeof(name), "HW=%dx%d k%d", sz, sz, R);
        r.record(run_test(tb, name,
                          seq_fill(sz * sz, 1),
                          const_fill(1 * R * R, 1), {},
                          sz, sz, 1, 1, R, R, 1, 1, 0, 0, 0));
    }

    // 3b: Non-square inputs
    int ns_cases[][2] = {{3,5}, {5,3}, {1,8}, {8,1}, {2,7}, {7,2}, {4,6}, {6,4}};
    for (auto &c : ns_cases)
    {
        int H = c[0], W = c[1];
        int R = std::min(H, 3), S = std::min(W, 3);
        if (!fits(H, W, 1, 1, R, S, 1, 1, 0, 0)) continue;

        NpuTb tb;
        tb.reset();
        tb.enable();
        char name[64];
        snprintf(name, sizeof(name), "nonsquare %dx%d k%dx%d", H, W, R, S);
        r.record(run_test(tb, name,
                          seq_fill(H * W, 1),
                          const_fill(1 * R * S, 1), {},
                          H, W, 1, 1, R, S, 1, 1, 0, 0, 0));
    }

    // 3c: Sweep C (input channels) with fixed spatial
    for (int C : {1, 2, 3, 4, 8})
    {
        if (!fits(4, 4, C, 1, 3, 3, 1, 1, 0, 0)) continue;

        NpuTb tb;
        tb.reset();
        tb.enable();
        char name[64];
        snprintf(name, sizeof(name), "C=%d (4x4 k3)", C);
        r.record(run_test(tb, name,
                          seq_fill(4 * 4 * C, 1),
                          const_fill(1 * 3 * 3 * C, 1), {},
                          4, 4, C, 1, 3, 3, 1, 1, 0, 0, 0));
    }

    // 3d: Sweep K (output filters) with fixed spatial
    for (int K : {1, 2, 3, 4})
    {
        if (!fits(4, 4, 1, K, 3, 3, 1, 1, 0, 0)) continue;

        NpuTb tb;
        tb.reset();
        tb.enable();
        char name[64];
        snprintf(name, sizeof(name), "K=%d (4x4 k3)", K);
        // Different weight pattern per filter
        std::vector<int8_t> wt(K * 9);
        for (int k = 0; k < K; ++k)
            for (int i = 0; i < 9; ++i)
                wt[k * 9 + i] = static_cast<int8_t>(k + 1);
        r.record(run_test(tb, name,
                          seq_fill(4 * 4, 1), wt, {},
                          4, 4, 1, K, 3, 3, 1, 1, 0, 0, 0));
    }

    // 3e: Sweep filter sizes
    for (int R : {1, 2, 3, 4, 5})
    {
        for (int S : {1, 2, 3, 4, 5})
        {
            int H = std::max(R, 5);
            int W = std::max(S, 5);
            if (!fits(H, W, 1, 1, R, S, 1, 1, 0, 0)) continue;

            NpuTb tb;
            tb.reset();
            tb.enable();
            char name[64];
            snprintf(name, sizeof(name), "filt %dx%d (%dx%d)", R, S, H, W);
            r.record(run_test(tb, name,
                              seq_fill(H * W, 1),
                              const_fill(R * S, 1), {},
                              H, W, 1, 1, R, S, 1, 1, 0, 0, 0));
        }
    }

    // 3f: Stride sweep
    for (int sh : {1, 2, 3, 4})
    {
        for (int sw : {1, 2, 3, 4})
        {
            int H = 8, W = 8, R = 3, S = 3;
            int OH = out_dim(H, 0, R, sh);
            int OW = out_dim(W, 0, S, sw);
            if (OH <= 0 || OW <= 0) continue;
            if (!fits(H, W, 1, 1, R, S, sh, sw, 0, 0)) continue;

            NpuTb tb;
            tb.reset();
            tb.enable();
            char name[64];
            snprintf(name, sizeof(name), "stride %d,%d (8x8 k3)", sh, sw);
            r.record(run_test(tb, name,
                              seq_fill(H * W, 1),
                              const_fill(R * S, 1), {},
                              H, W, 1, 1, R, S, sh, sw, 0, 0, 0));
        }
    }

    // 3g: Padding sweep
    for (int ph : {0, 1, 2})
    {
        for (int pw : {0, 1, 2})
        {
            int H = 5, W = 5, R = 3, S = 3;
            if (ph >= R || pw >= S) continue;
            int OH = out_dim(H, ph, R, 1);
            int OW = out_dim(W, pw, S, 1);
            if (OH <= 0 || OW <= 0) continue;
            if (!fits(H, W, 1, 1, R, S, 1, 1, ph, pw)) continue;

            NpuTb tb;
            tb.reset();
            tb.enable();
            char name[64];
            snprintf(name, sizeof(name), "pad %d,%d (5x5 k3)", ph, pw);
            r.record(run_test(tb, name,
                              seq_fill(H * W, 1),
                              const_fill(R * S, 1), {},
                              H, W, 1, 1, R, S, 1, 1, ph, pw, 0));
        }
    }

    // 3h: Combined C, K with larger spatial
    for (int C : {2, 3})
    {
        for (int K : {2, 3})
        {
            int H = 6, W = 6, R = 3, S = 3;
            if (!fits(H, W, C, K, R, S, 1, 1, 0, 0)) continue;

            NpuTb tb;
            tb.reset();
            tb.enable();
            char name[64];
            snprintf(name, sizeof(name), "CK=%dx%d (6x6 k3)", C, K);
            r.record(run_test(tb, name,
                              seq_fill(H * W * C, 1),
                              seq_fill(K * R * S * C, 1), {},
                              H, W, C, K, R, S, 1, 1, 0, 0, 0));
        }
    }

    // 3i: Stride + padding combos
    for (int sh : {1, 2})
    {
        for (int ph : {0, 1})
        {
            int H = 7, W = 7, R = 3, S = 3;
            int OH = out_dim(H, ph, R, sh);
            int OW = out_dim(W, ph, S, sh);
            if (OH <= 0 || OW <= 0) continue;
            if (!fits(H, W, 1, 1, R, S, sh, sh, ph, ph)) continue;

            NpuTb tb;
            tb.reset();
            tb.enable();
            char name[64];
            snprintf(name, sizeof(name), "s=%d p=%d (7x7 k3)", sh, ph);
            r.record(run_test(tb, name,
                              seq_fill(H * W, 1),
                              const_fill(R * S, 1), {},
                              H, W, 1, 1, R, S, sh, sh, ph, ph, 0));
        }
    }

    // 3j: 1x1 convolutions across different channel configs
    for (int C : {1, 2, 4})
    {
        for (int K : {1, 2, 4})
        {
            if (!fits(8, 8, C, K, 1, 1, 1, 1, 0, 0)) continue;

            NpuTb tb;
            tb.reset();
            tb.enable();
            char name[64];
            snprintf(name, sizeof(name), "1x1 C=%d K=%d (8x8)", C, K);
            r.record(run_test(tb, name,
                              seq_fill(8 * 8 * C, 1),
                              seq_fill(K * 1 * 1 * C, 1), {},
                              8, 8, C, K, 1, 1, 1, 1, 0, 0, 0));
        }
    }

    // 3k: Maximum-size test (push buffer limits)
    //     H*W*C input + OH*OW*K output <= 8192, K*R*S*C <= 4096
    //     Try 64x64x1, 1x1 filter, K=1: in=4096, out=4096, wt=1 -> fits
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        r.record(run_test(tb, "maxsize 64x64x1 k1",
                          seq_fill(64 * 64 * 1, 1),
                          const_fill(1, 2), {},
                          64, 64, 1, 1, 1, 1, 1, 1, 0, 0, 0));
    }
    //     Try 32x32x2, 1x1 filter, K=2: in=2048, out=2048, wt=4 -> fits
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        r.record(run_test(tb, "maxsize 32x32x2 K=2 k1",
                          seq_fill(32 * 32 * 2, 1),
                          seq_fill(2 * 1 * 1 * 2, 1), {},
                          32, 32, 2, 2, 1, 1, 1, 1, 0, 0, 0));
    }
}

// Section 4: Invalid-command and error-path tests
static void run_invalid_command_tests(TestResult &r)
{
    printf("\n--- Invalid-Command Combinations ---\n\n");

    // 4a: Bad opcodes (0, 2..15)
    for (int op : {0, 2, 3, 7, 15})
    {
        NpuTb tb;
        tb.reset();
        tb.enable();

        uint32_t words[16] = {};
        words[0] = static_cast<uint32_t>(op);
        for (int i = 0; i < 16; ++i)
            tb.mmio_write(CMD_QUEUE_BASE + static_cast<uint32_t>(i) * 4, words[i]);
        tb.mmio_write(REG_DOORBELL, 1);
        tb.run_clocks(200);

        bool idle = tb.wait_idle(2000);
        uint32_t irq = tb.mmio_read(REG_IRQ_STATUS);
        bool pass = idle && (irq & 1);
        if (!pass)
            printf("  [FAIL] bad_opcode=%d: idle=%d irq=0x%x\n", op, idle, irq);
        else
            printf("  [PASS] bad_opcode=%d\n", op);

        // Clear IRQ for next test
        tb.mmio_write(REG_IRQ_CLEAR, 1);
        r.record(pass);
    }

    // 4b: Valid opcode but zero dimensions: hardware loops have no exit
    //     condition for all-zero dims, so the NPU will stall.  Verify
    //     that a soft reset recovers cleanly.
    {
        NpuTb tb;
        tb.reset();
        tb.enable();

        ConvDesc desc{};
        desc.opcode = 1;
        desc.in_h = 0; desc.in_w = 0; desc.in_c = 0;
        desc.out_k = 0; desc.filt_r = 0; desc.filt_s = 0;
        desc.stride_h = 1; desc.stride_w = 1;
        uint32_t words[16];
        desc.to_words(words);
        for (int i = 0; i < 16; ++i)
            tb.mmio_write(CMD_QUEUE_BASE + static_cast<uint32_t>(i) * 4, words[i]);
        tb.mmio_write(REG_DOORBELL, 1);
        tb.run_clocks(500);

        // Soft reset to recover
        tb.soft_reset_pulse();
        bool idle = tb.is_idle();

        // Verify a real conv still works after recovery
        bool conv_ok = false;
        if (idle)
        {
            auto act = seq_fill(4 * 4, 1);
            auto wt  = const_fill(9, 1);
            conv_ok = tb.run_conv_test("zero_dims recovery",
                                       act, wt, {},
                                       4, 4, 1, 1, 3, 3,
                                       1, 1, 0, 0, 0);
        }

        bool pass = idle && conv_ok;
        if (!pass)
            printf("  [FAIL] zero_dims: idle=%d conv=%d\n", idle, conv_ok);
        else
            printf("  [PASS] zero_dims (soft reset recovery)\n");
        r.record(pass);
    }

    // 4c: Doorbell without writing command (stale queue contents)
    {
        NpuTb tb;
        tb.reset();
        tb.enable();

        // Don't write any command words, just ring doorbell
        tb.mmio_write(REG_DOORBELL, 1);
        tb.run_clocks(500);

        bool idle = tb.wait_idle(5000);
        bool pass = idle;
        if (!pass)
            printf("  [FAIL] stale_doorbell: NPU hung\n");
        else
            printf("  [PASS] stale_doorbell\n");
        r.record(pass);
    }

    // 4d: Multiple rapid doorbells (same command)
    {
        NpuTb tb;
        tb.reset();
        tb.enable();

        auto act = seq_fill(4 * 4, 1);
        auto wt  = const_fill(9, 1);
        tb.load_bytes(WEIGHT_BUF_BASE, wt);
        tb.load_bytes(ACT_BUF_BASE, act);

        ConvDesc desc{};
        desc.opcode = 1;
        desc.act_out_addr = 2048;
        desc.in_h = 4; desc.in_w = 4; desc.in_c = 1;
        desc.out_k = 1; desc.filt_r = 3; desc.filt_s = 3;
        desc.stride_h = 1; desc.stride_w = 1;
        uint32_t words[16];
        desc.to_words(words);
        for (int i = 0; i < 16; ++i)
            tb.mmio_write(CMD_QUEUE_BASE + static_cast<uint32_t>(i) * 4, words[i]);

        // Ring doorbell 3 times rapidly
        tb.mmio_write(REG_DOORBELL, 1);
        tb.mmio_write(REG_DOORBELL, 1);
        tb.mmio_write(REG_DOORBELL, 1);
        tb.run_clocks(3000);

        bool idle = tb.wait_idle(100000);
        bool pass = idle;
        if (!pass)
            printf("  [FAIL] rapid_doorbells: NPU hung\n");
        else
            printf("  [PASS] rapid_doorbells\n");
        r.record(pass);
    }

    // 4e: Write to read-only registers (STATUS, FEATURE_ID, PERF) - should not crash
    {
        NpuTb tb;
        tb.reset();
        tb.enable();

        tb.mmio_write(REG_STATUS, 0xDEADBEEF);
        tb.mmio_write(REG_FEATURE_ID, 0xDEADBEEF);
        tb.mmio_write(REG_PERF_CYCLES, 0xDEADBEEF);
        tb.mmio_write(REG_PERF_ACTIVE, 0xDEADBEEF);
        tb.mmio_write(REG_PERF_STALL, 0xDEADBEEF);

        // Should still be alive and idle
        bool pass = tb.is_idle();

        // Readback should not return our garbage
        uint32_t fid = tb.mmio_read(REG_FEATURE_ID);
        if (fid == 0xDEADBEEF)
        {
            printf("  [FAIL] ro_write: FEATURE_ID was overwritten\n");
            pass = false;
        }

        // Run a conv to prove hardware is not corrupted
        if (pass)
        {
            auto act = seq_fill(4 * 4, 1);
            auto wt  = const_fill(9, 1);
            pass = tb.run_conv_test("ro_write recovery",
                                    act, wt, {},
                                    4, 4, 1, 1, 3, 3,
                                    1, 1, 0, 0, 0);
        }

        if (pass) printf("  [PASS] ro_write_harmless\n");
        r.record(pass);
    }

    // 4f: Soft reset between two commands
    {
        NpuTb tb;
        tb.reset();
        tb.enable();

        auto act = seq_fill(4 * 4, 1);
        auto wt  = const_fill(9, 1);

        // First conv
        bool p1 = tb.run_conv_test("softrst cmd1",
                                   act, wt, {},
                                   4, 4, 1, 1, 3, 3,
                                   1, 1, 0, 0, 0);

        // Soft reset
        tb.soft_reset_pulse();

        // Second conv (same data reloaded)
        tb.load_bytes(WEIGHT_BUF_BASE, wt);
        tb.load_bytes(ACT_BUF_BASE, act);
        bool p2 = tb.run_conv_test("softrst cmd2",
                                   act, wt, {},
                                   4, 4, 1, 1, 3, 3,
                                   1, 1, 0, 0, 0);

        bool pass = p1 && p2;
        if (pass) printf("  [PASS] soft_reset_between_cmds\n");
        r.record(pass);
    }

    // 4g: Enable toggling
    {
        NpuTb tb;
        tb.reset();
        // Start without enable
        bool idle_pre = tb.is_idle();

        // Enable
        tb.enable();
        bool idle_post = tb.is_idle();

        // Disable (write CTRL=0)
        tb.mmio_write(REG_CTRL, 0);
        tb.run_clocks(10);

        // Re-enable and run a conv
        tb.enable();
        auto act = seq_fill(4 * 4, 1);
        auto wt  = const_fill(9, 1);
        bool conv_ok = tb.run_conv_test("enable_toggle",
                                        act, wt, {},
                                        4, 4, 1, 1, 3, 3,
                                        1, 1, 0, 0, 0);

        bool pass = idle_pre && idle_post && conv_ok;
        if (pass)
            printf("  [PASS] enable_toggle\n");
        else
            printf("  [FAIL] enable_toggle: pre=%d post=%d conv=%d\n",
                   idle_pre, idle_post, conv_ok);
        r.record(pass);
    }

    // 4h: Back-to-back different-size convolutions (state leak check)
    {
        NpuTb tb;
        tb.reset();
        tb.enable();

        auto act1 = seq_fill(3 * 3, 1);
        auto wt1  = const_fill(4, 1);  // 2x2 filter
        bool p1 = tb.run_conv_test("leak_check cmd1 (3x3 k2)",
                                   act1, wt1, {},
                                   3, 3, 1, 1, 2, 2,
                                   1, 1, 0, 0, 0);

        auto act2 = seq_fill(5 * 5, 1);
        auto wt2  = const_fill(9, 2);  // 3x3 filter, weight=2
        bool p2 = tb.run_conv_test("leak_check cmd2 (5x5 k3)",
                                   act2, wt2, {},
                                   5, 5, 1, 1, 3, 3,
                                   1, 1, 0, 0, 0);

        auto act3 = seq_fill(4 * 4 * 2, 1);
        auto wt3  = const_fill(2 * 3 * 3 * 2, 1);  // C=2 K=2
        bool p3 = tb.run_conv_test("leak_check cmd3 (4x4x2 K=2)",
                                   act3, wt3, {},
                                   4, 4, 2, 2, 3, 3,
                                   1, 1, 0, 0, 0);

        bool pass = p1 && p2 && p3;
        if (pass) printf("  [PASS] state_leak_check\n");
        r.record(pass);
    }
}

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);
    TestResult r;

    printf("=== Full Regression Suite ===\n");

    // Parse optional seed from environment
    uint32_t seed = 42;
    const char *seed_env = std::getenv("NPU_TEST_SEED");
    if (seed_env) seed = static_cast<uint32_t>(std::stoul(seed_env));

    run_randomized_sweeps(r, 50, seed);
    run_edge_value_tests(r);
    run_dimension_sweeps(r);
    run_invalid_command_tests(r);

    r.summary("Full Regression");
    return r.exit_code();
}
