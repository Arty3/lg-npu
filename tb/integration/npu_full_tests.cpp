// npu_full_tests.cpp - Comprehensive regression suite for sim-full
//
// Sections:
//   1. Randomized convolution sweeps (seeded PRNG)
//   2. Edge-value / saturation distribution tests
//   3. Dimension sweeps (H, W, C, K, R, S, stride, pad)
//   4. Invalid-command combinations
//   5. Activation function tests
//   6. GEMM tests
//   7. Softmax tests
//   8. Vec tests
//   9. LayerNorm tests
//  10. Pooling tests
//  11. DMA tests

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
    int qshift,
    uint32_t act_mode = ACT_MODE_RELU)
{
    int in_bytes = H * W * C;
    // Place output right after input, aligned up to next 256-byte boundary
    uint32_t out_addr = static_cast<uint32_t>((in_bytes + 255) & ~255);
    uint32_t bias_addr = static_cast<uint32_t>(wt.size());

    return tb.run_conv_test(
        name, act, wt, bias,
        H, W, C, K, R, S,
        sh, sw, ph, pw, qshift,
        act_mode,
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

        // Generate random data with occasional bias
        auto act  = rand_fill(H * W * C, rng);
        auto wt   = rand_fill(K * R * S * C, rng);
        std::vector<int8_t> bias;
        std::uniform_int_distribution<int> bias_coin(0, 2);  // ~1/3 chance of bias
        if (bias_coin(rng) == 0)
        {
            bias.resize(K);
            std::uniform_int_distribution<int> bval(-128, 127);
            for (int b = 0; b < K; ++b)
                bias[b] = static_cast<int8_t>(bval(rng));
        }

        // Random activation mode (~1/3 each)
        std::uniform_int_distribution<int> act_coin(0, 2);
        int am = act_coin(rng);
        uint32_t act_mode = (am == 0) ? ACT_MODE_RELU
                          : (am == 1) ? ACT_MODE_NONE
                          :             ACT_MODE_LEAKY_RELU;
        const char *am_tag = (act_mode == ACT_MODE_RELU) ? "relu"
                           : (act_mode == ACT_MODE_NONE) ? "none"
                           :                               "leaky";

        char name[128];
        snprintf(name, sizeof(name),
                 "rand[%d] %dx%dx%d K=%d %dx%d s=%d,%d p=%d,%d q=%d %s",
                 generated, H, W, C, K, R, S, sh, sw, ph, pw, qshift, am_tag);

        NpuTb tb;
        tb.reset();
        tb.enable();
        r.record(run_test(tb, name, act, wt, bias,
                          H, W, C, K, R, S, sh, sw, ph, pw, qshift,
                          act_mode));
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

    // 2h: Bias tests
    // Positive bias: acc = 9*1*1 = 9, bias = +50 -> acc = 59, ReLU -> 59, q=0 -> 59
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        r.record(run_test(tb, "bias +50",
                          const_fill(4 * 4 * 1, 1),
                          const_fill(1 * 3 * 3 * 1, 1),
                          const_fill(1, 50),
                          4, 4, 1, 1, 3, 3, 1, 1, 0, 0, 0));
    }
    // Max positive bias: acc = 0, bias = +127 -> 127, q=0 -> 127
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        r.record(run_test(tb, "bias +127 (max)",
                          const_fill(4 * 4 * 1, 0),
                          const_fill(1 * 3 * 3 * 1, 0),
                          const_fill(1, 127),
                          4, 4, 1, 1, 3, 3, 1, 1, 0, 0, 0));
    }
    // Negative bias causing ReLU clamp: acc = 9*1*1 = 9, bias = -128 -> -119, ReLU -> 0
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        r.record(run_test(tb, "bias -128 (ReLU clamp)",
                          const_fill(4 * 4 * 1, 1),
                          const_fill(1 * 3 * 3 * 1, 1),
                          const_fill(1, -128),
                          4, 4, 1, 1, 3, 3, 1, 1, 0, 0, 0));
    }
    // Multi-filter bias: K=2, each filter gets its own bias
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> bias = {10, -20};
        r.record(run_test(tb, "bias K=2 per-filter",
                          seq_fill(4 * 4 * 1, 1),
                          const_fill(2 * 3 * 3 * 1, 1),
                          bias,
                          4, 4, 1, 2, 3, 3, 1, 1, 0, 0, 0));
    }
    // Bias with quantization: acc = 9*127*127 = 145161, bias = 100 -> 145261
    // >> 11 = 70
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        r.record(run_test(tb, "bias + qshift=11",
                          const_fill(4 * 4 * 1, 127),
                          const_fill(1 * 3 * 3 * 1, 127),
                          const_fill(1, 100),
                          4, 4, 1, 1, 3, 3, 1, 1, 0, 0, 11));
    }

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

// Section 9: LayerNorm tests
static void run_lnorm_tests(TestResult &r)
{
    printf("\n--- LayerNorm Tests ---\n\n");

    // 9a: Single element (N=1) - mean = x, var = 0, std = 1, output = 0
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> input = {42};
        r.record(tb.run_lnorm_test("lnorm single N=1", input, 1, 1, 0));
    }

    // 9b: All-same values - output all zeros
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> input = {10, 10, 10, 10};
        r.record(tb.run_lnorm_test("lnorm all-same", input, 1, 4, 0));
    }

    // 9c: Simple pair [0, 4] shift=0
    //   mean=2, var_sum=8, var=4, std=isqrt(5)=2, out=[-1, 1]
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> input = {0, 4};
        r.record(tb.run_lnorm_test("lnorm pair [0,4] s=0", input, 1, 2, 0));
    }

    // 9d: Simple pair with shift=3
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> input = {0, 10};
        r.record(tb.run_lnorm_test("lnorm pair [0,10] s=3", input, 1, 2, 3));
    }

    // 9e: Multi-element row
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> input = {-4, -2, 0, 2, 4, 6};
        r.record(tb.run_lnorm_test("lnorm 6-elem s=2", input, 1, 6, 2));
    }

    // 9f: Multi-row (M=2, N=4)
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> input = {
             1, 3, 5, 7,       // row 0: mean=4
            -10, -5, 0, 5      // row 1: mean=-2 (integer truncation of -10/4)
        };
        r.record(tb.run_lnorm_test("lnorm 2x4 s=2", input, 2, 4, 2));
    }

    // 9g: Saturation test - large shift forces clamping
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> input = {-128, 127, -128, 127};
        r.record(tb.run_lnorm_test("lnorm saturate s=7", input, 1, 4, 7));
    }

    // 9h: Negative mean with odd sum
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> input = {-7, -3, -1};
        r.record(tb.run_lnorm_test("lnorm neg mean s=1", input, 1, 3, 1));
    }

    // 9i: Random 16-element test
    {
        std::mt19937 rng(301);
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> input(16);
        std::uniform_int_distribution<int> dist(-128, 127);
        for (int i = 0; i < 16; ++i)
            input[i] = static_cast<int8_t>(dist(rng));
        r.record(tb.run_lnorm_test("lnorm rand-16 s=4", input, 1, 16, 4));
    }

    // 9j: Random multi-row (M=3, N=8)
    {
        std::mt19937 rng(302);
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> input(24);
        std::uniform_int_distribution<int> dist(-128, 127);
        for (int i = 0; i < 24; ++i)
            input[i] = static_cast<int8_t>(dist(rng));
        r.record(tb.run_lnorm_test("lnorm rand 3x8 s=3", input, 3, 8, 3));
    }

    // 9k: Lnorm then conv on same instance (backend interleave)
    {
        NpuTb tb;
        tb.reset();
        tb.enable();

        // First: lnorm 1x4
        std::vector<int8_t> ln_in = {-2, 0, 2, 4};
        bool ln_ok = tb.run_lnorm_test("lnorm-then-conv (lnorm)",
                                        ln_in, 1, 4, 2);

        // Second: conv 4x4x1 k3
        auto ca  = seq_fill(4 * 4, 1);
        auto cwt = const_fill(9, 1);
        bool conv_ok = tb.run_conv_test("lnorm-then-conv (conv)",
                                        ca, cwt, {},
                                        4, 4, 1, 1, 3, 3,
                                        1, 1, 0, 0, 0);
        r.record(ln_ok && conv_ok);
    }
}

// Section 10: Pooling tests
static void run_pool_tests(TestResult &r)
{
    printf("\n--- Pooling Tests ---\n\n");

    // 10a: Max pool 2x2 stride 1 on 3x3x1
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        // 3x3 input (NHWC, C=1):
        //  1  2  3
        //  4  5  6
        //  7  8  9
        std::vector<int8_t> input = {1, 2, 3, 4, 5, 6, 7, 8, 9};
        r.record(tb.run_pool_test("maxpool 2x2 s1 3x3x1",
                                  input, 3, 3, 1, 2, 2, 1, 1, 0, 0, 0));
    }

    // 10b: Max pool 2x2 stride 2 on 4x4x1
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> input = {
            1,  2,  3,  4,
            5,  6,  7,  8,
            9,  10, 11, 12,
            13, 14, 15, 16
        };
        r.record(tb.run_pool_test("maxpool 2x2 s2 4x4x1",
                                  input, 4, 4, 1, 2, 2, 2, 2, 0, 0, 0));
    }

    // 10c: Avg pool 2x2 stride 1 on 3x3x1
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> input = {4, 8, 12, 16, 20, 24, 28, 32, 36};
        r.record(tb.run_pool_test("avgpool 2x2 s1 3x3x1",
                                  input, 3, 3, 1, 2, 2, 1, 1, 0, 0, 1));
    }

    // 10d: Max pool with padding
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> input = {1, 2, 3, 4};
        // 2x2 input, 2x2 pool, stride 1, pad 1 -> 3x3 output
        r.record(tb.run_pool_test("maxpool 2x2 s1 pad1 2x2x1",
                                  input, 2, 2, 1, 2, 2, 1, 1, 1, 1, 0));
    }

    // 10e: Max pool multi-channel (C=2)
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        // 2x2x2 NHWC: [(1,10), (2,20), (3,30), (4,40)]
        std::vector<int8_t> input = {1, 10, 2, 20, 3, 30, 4, 40};
        r.record(tb.run_pool_test("maxpool 2x2 s1 2x2x2",
                                  input, 2, 2, 2, 2, 2, 1, 1, 0, 0, 0));
    }

    // 10f: Avg pool multi-channel (C=2)
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> input = {4, 8, 12, 16, 20, 24, 28, 32};
        r.record(tb.run_pool_test("avgpool 2x2 s1 2x2x2",
                                  input, 2, 2, 2, 2, 2, 1, 1, 0, 0, 1));
    }

    // 10g: Max pool all-negative (should still find max)
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> input = {-128, -64, -32, -16};
        r.record(tb.run_pool_test("maxpool all-negative",
                                  input, 2, 2, 1, 2, 2, 1, 1, 0, 0, 0));
    }

    // 10h: Avg pool with rounding toward zero
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        // Sum = 1+2+3+4 = 10, 10/4 = 2 (truncated)
        std::vector<int8_t> input = {1, 2, 3, 4};
        r.record(tb.run_pool_test("avgpool rounding",
                                  input, 2, 2, 1, 2, 2, 1, 1, 0, 0, 1));
    }

    // 10i: Avg pool with padding (pad contributes zeros)
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> input = {10, 20, 30, 40};
        r.record(tb.run_pool_test("avgpool 2x2 s1 pad1 2x2x1",
                                  input, 2, 2, 1, 2, 2, 1, 1, 1, 1, 1));
    }

    // 10j: Random max pool
    {
        std::mt19937 rng(401);
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> input(4 * 4 * 2);
        std::uniform_int_distribution<int> dist(-128, 127);
        for (auto &x : input) x = static_cast<int8_t>(dist(rng));
        r.record(tb.run_pool_test("maxpool rand 4x4x2 k2 s2",
                                  input, 4, 4, 2, 2, 2, 2, 2, 0, 0, 0));
    }

    // 10k: Random avg pool
    {
        std::mt19937 rng(402);
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> input(3 * 3 * 2);
        std::uniform_int_distribution<int> dist(-128, 127);
        for (auto &x : input) x = static_cast<int8_t>(dist(rng));
        r.record(tb.run_pool_test("avgpool rand 3x3x2 k2 s1",
                                  input, 3, 3, 2, 2, 2, 1, 1, 0, 0, 1));
    }

    // 10l: Pool-then-conv interleave
    {
        NpuTb tb;
        tb.reset();
        tb.enable();

        // First: max pool 3x3x1 k2 s1
        std::vector<int8_t> pool_in = {1, 2, 3, 4, 5, 6, 7, 8, 9};
        bool pool_ok = tb.run_pool_test("pool-then-conv (pool)",
                                        pool_in, 3, 3, 1, 2, 2, 1, 1, 0, 0, 0);

        // Second: conv 4x4x1 k3
        auto ca  = seq_fill(4 * 4, 1);
        auto cwt = const_fill(9, 1);
        bool conv_ok = tb.run_conv_test("pool-then-conv (conv)",
                                        ca, cwt, {},
                                        4, 4, 1, 1, 3, 3,
                                        1, 1, 0, 0, 0);
        r.record(pool_ok && conv_ok);
    }

    // 10m: Max pool 3x3 stride 2 on 5x5x1
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> input(25);
        for (int i = 0; i < 25; ++i) input[i] = static_cast<int8_t>(i + 1);
        r.record(tb.run_pool_test("maxpool 3x3 s2 5x5x1",
                                  input, 5, 5, 1, 3, 3, 2, 2, 0, 0, 0));
    }
}

// Section 4: Invalid-command and error-path tests
static void run_invalid_command_tests(TestResult &r)
{
    printf("\n--- Invalid-Command Combinations ---\n\n");

    // 4a: Bad opcodes (0, 6..15)
    for (int op : {0, 7, 8, 15})
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

// Section 5: Activation function tests
static void run_activation_tests(TestResult &r)
{
    printf("\n--- Activation Function Tests ---\n\n");

    // 5a: ACT_NONE: negative accumulation passes through (no ReLU clamp)
    //     4x4x1, 3x3 all-1 weights, act=-1 -> acc = 9*(-1)*1 = -9
    //     With no activation and q=0, output = saturate(-9) = -9
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        r.record(run_test(tb, "act_none neg passthrough",
                          const_fill(4 * 4, -1),
                          const_fill(9, 1), {},
                          4, 4, 1, 1, 3, 3, 1, 1, 0, 0, 0,
                          ACT_MODE_NONE));
    }

    // 5b: ACT_NONE: positive values unchanged (same as ReLU for positives)
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        r.record(run_test(tb, "act_none pos passthrough",
                          seq_fill(4 * 4, 1),
                          const_fill(9, 1), {},
                          4, 4, 1, 1, 3, 3, 1, 1, 0, 0, 0,
                          ACT_MODE_NONE));
    }

    // 5c: ACT_NONE with quant shift: negative acc >> shift saturated
    //     acc = 9*(-128)*127 = -146304, >> 11 = -72 (no clamp)
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        r.record(run_test(tb, "act_none neg + qshift=11",
                          const_fill(4 * 4, -128),
                          const_fill(9, 127), {},
                          4, 4, 1, 1, 3, 3, 1, 1, 0, 0, 11,
                          ACT_MODE_NONE));
    }

    // 5d: Leaky ReLU: positive values pass through unchanged
    //     Same as ReLU for positives
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        r.record(run_test(tb, "leaky_relu pos passthrough",
                          seq_fill(4 * 4, 1),
                          const_fill(9, 1), {},
                          4, 4, 1, 1, 3, 3, 1, 1, 0, 0, 0,
                          ACT_MODE_LEAKY_RELU));
    }

    // 5e: Leaky ReLU: negative accumulation scaled by 1/8
    //     acc = 9*(-1)*1 = -9, leaky = -9 >> 3 = -2 (arith shift), q=0 -> -2
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        r.record(run_test(tb, "leaky_relu neg scale",
                          const_fill(4 * 4, -1),
                          const_fill(9, 1), {},
                          4, 4, 1, 1, 3, 3, 1, 1, 0, 0, 0,
                          ACT_MODE_LEAKY_RELU));
    }

    // 5f: Leaky ReLU with large negative + quant shift
    //     act=-128, wt=127 -> acc = 9*(-128)*127 = -146304
    //     leaky = -146304 >> 3 = -18288, >> 11 = -9
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        r.record(run_test(tb, "leaky_relu large neg + qshift=11",
                          const_fill(4 * 4, -128),
                          const_fill(9, 127), {},
                          4, 4, 1, 1, 3, 3, 1, 1, 0, 0, 11,
                          ACT_MODE_LEAKY_RELU));
    }

    // 5g: Leaky ReLU with bias pushing negative
    //     acc = 9*1*1 = 9, bias = -50 -> -41, leaky = -41 >> 3 = -6, q=0 -> -6
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        r.record(run_test(tb, "leaky_relu neg bias",
                          const_fill(4 * 4, 1),
                          const_fill(9, 1),
                          const_fill(1, -50),
                          4, 4, 1, 1, 3, 3, 1, 1, 0, 0, 0,
                          ACT_MODE_LEAKY_RELU));
    }

    // 5h: Leaky ReLU multi-filter: K=2, per-filter bias
    //     k=0: acc=9, bias=100 -> 109 (positive, pass through)
    //     k=1: acc=9, bias=-100 -> -91, leaky = -91>>3 = -12
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> bias = {100, -100};
        r.record(run_test(tb, "leaky_relu K=2 mixed",
                          const_fill(4 * 4, 1),
                          const_fill(2 * 9, 1),
                          bias,
                          4, 4, 1, 2, 3, 3, 1, 1, 0, 0, 0,
                          ACT_MODE_LEAKY_RELU));
    }

    // 5i: ReLU still works (regression): negative clamped to 0
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        r.record(run_test(tb, "relu neg clamp (regression)",
                          const_fill(4 * 4, -1),
                          const_fill(9, 1), {},
                          4, 4, 1, 1, 3, 3, 1, 1, 0, 0, 0,
                          ACT_MODE_RELU));
    }
}

// Section 6: GEMM tests
static void run_gemm_tests(TestResult &r)
{
    printf("\n--- GEMM Tests ---\n\n");

    // 6a: 2x2 * 2x2, all ones, no bias, no activation
    //     Each output = sum of 2 products of 1*1 = 2
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        auto a = const_fill(2 * 2, 1);
        auto b = const_fill(2 * 2, 1);
        r.record(tb.run_gemm_test("gemm 2x2 ones", a, b, {}, 2, 2, 2, 0));
    }

    // 6b: 2x3 * 3x2, sequential A, all-one B, no bias
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        auto a = seq_fill(2 * 3, 1);   // [1,2,3, 4,5,6]
        auto b = const_fill(3 * 2, 1);
        r.record(tb.run_gemm_test("gemm 2x3 * 3x2 seq", a, b, {}, 2, 2, 3, 0));
    }

    // 6c: 3x4 * 4x2 with bias
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        auto a    = seq_fill(3 * 4, 1);
        auto b    = const_fill(4 * 2, 2);
        auto bias = std::vector<int8_t>{10, -10};
        r.record(tb.run_gemm_test("gemm 3x4 * 4x2 bias", a, b, bias,
                                  3, 2, 4, 0));
    }

    // 6d: 2x3 * 3x4 with ReLU activation (negative value row)
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        auto a = std::vector<int8_t>{ 1, 2, 3, -1, -2, -3 };
        auto b = const_fill(3 * 4, 1);
        r.record(tb.run_gemm_test("gemm 2x3 * 3x4 relu", a, b, {},
                                  2, 4, 3, 0, ACT_MODE_RELU));
    }

    // 6e: 2x3 * 3x4 with Leaky ReLU
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        auto a = std::vector<int8_t>{ 1, 2, 3, -8, -16, -24 };
        auto b = const_fill(3 * 4, 1);
        r.record(tb.run_gemm_test("gemm 2x3 * 3x4 leaky", a, b, {},
                                  2, 4, 3, 0, ACT_MODE_LEAKY_RELU));
    }

    // 6f: 4x8 * 8x3 with quantization shift
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        auto a = seq_fill(4 * 8, 1);
        auto b = const_fill(8 * 3, 1);
        r.record(tb.run_gemm_test("gemm 4x8 * 8x3 qshift=3", a, b, {},
                                  4, 3, 8, 3));
    }

    // 6g: 1x1 * 1x1 scalar multiply
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        auto a = std::vector<int8_t>{ 7 };
        auto b = std::vector<int8_t>{ 11 };
        r.record(tb.run_gemm_test("gemm 1x1 scalar", a, b, {}, 1, 1, 1, 0));
    }

    // 6h: 8x8 * 8x8 larger test
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        auto a = seq_fill(8 * 8, 1);
        auto b = seq_fill(8 * 8, 1);
        r.record(tb.run_gemm_test("gemm 8x8 * 8x8", a, b, {},
                                  8, 8, 8, 7));
    }

    // 6i: saturation test (large values accumulate to >127 or <-128)
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        auto a = const_fill(2 * 4, 127);
        auto b = const_fill(4 * 2, 127);
        // acc = 4 * 127 * 127 = 64516 per element; qshift=9 -> 64516>>9 = 126
        r.record(tb.run_gemm_test("gemm saturation", a, b, {},
                                  2, 2, 4, 9));
    }

    // 6j: GEMM then conv on same instance (backend interleave)
    {
        NpuTb tb;
        tb.reset();
        tb.enable();

        // First: GEMM 2x2 * 2x2
        auto ga = const_fill(2 * 2, 3);
        auto gb = const_fill(2 * 2, 2);
        bool gemm_ok = tb.run_gemm_test("gemm-then-conv (gemm)",
                                        ga, gb, {}, 2, 2, 2, 0);

        // Second: conv 4x4x1 k3
        auto ca  = seq_fill(4 * 4, 1);
        auto cwt = const_fill(9, 1);
        bool conv_ok = tb.run_conv_test("gemm-then-conv (conv)",
                                        ca, cwt, {},
                                        4, 4, 1, 1, 3, 3,
                                        1, 1, 0, 0, 0);
        r.record(gemm_ok && conv_ok);
    }
}

// Section 7: Softmax tests
static void run_softmax_tests(TestResult &r)
{
    printf("\n--- Softmax Tests ---\n\n");

    // 7a: Single element - output must be 127
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> input = {42};
        r.record(tb.run_softmax_test("softmax single", input, 1, 1));
    }

    // 7b: Two equal elements - each output = floor(65535*127 / (2*65535)) = 63
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> input = {10, 10};
        r.record(tb.run_softmax_test("softmax 2-equal", input, 1, 2));
    }

    // 7c: Four equal elements
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> input = {-5, -5, -5, -5};
        r.record(tb.run_softmax_test("softmax 4-equal", input, 1, 4));
    }

    // 7d: Peaked distribution (one dominant element)
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> input = {100, -50, -50, -50};
        r.record(tb.run_softmax_test("softmax peaked", input, 1, 4));
    }

    // 7e: Sequential values (ascending)
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> input = {1, 2, 3, 4, 5, 6, 7, 8};
        r.record(tb.run_softmax_test("softmax sequential", input, 1, 8));
    }

    // 7f: Multi-row (M=2, N=4) - two independent softmax operations
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> input = {10, 10, 10, 10, 127, -128, -128, -128};
        r.record(tb.run_softmax_test("softmax 2-row", input, 2, 4));
    }

    // 7g: All maximum value
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> input(8, 127);
        r.record(tb.run_softmax_test("softmax all-max", input, 1, 8));
    }

    // 7h: All minimum value
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> input(4, -128);
        r.record(tb.run_softmax_test("softmax all-min", input, 1, 4));
    }

    // 7i: Random softmax (small)
    {
        std::mt19937 rng(99);
        NpuTb tb;
        tb.reset();
        tb.enable();
        auto input = rand_fill(16, rng);
        r.record(tb.run_softmax_test("softmax rand-16", input, 1, 16));
    }

    // 7j: Larger random softmax (M=3, N=8)
    {
        std::mt19937 rng(77);
        NpuTb tb;
        tb.reset();
        tb.enable();
        auto input = rand_fill(3 * 8, rng);
        r.record(tb.run_softmax_test("softmax rand-3x8", input, 3, 8));
    }

    // 7k: Softmax then conv on same instance (backend interleave)
    {
        NpuTb tb;
        tb.reset();
        tb.enable();

        // First: softmax 1x4
        std::vector<int8_t> sinput = {10, 20, 30, 40};
        bool smax_ok = tb.run_softmax_test("smax-then-conv (smax)",
                                           sinput, 1, 4);

        // Second: conv 4x4x1 k3
        auto ca  = seq_fill(4 * 4, 1);
        auto cwt = const_fill(9, 1);
        bool conv_ok = tb.run_conv_test("smax-then-conv (conv)",
                                        ca, cwt, {},
                                        4, 4, 1, 1, 3, 3,
                                        1, 1, 0, 0, 0);
        r.record(smax_ok && conv_ok);
    }
}

// Section 8: Vec (element-wise) tests
static void run_vec_tests(TestResult &r)
{
    printf("\n--- Vec (Element-Wise) Tests ---\n\n");

    // 8a: Single-element ADD
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> a = {10}, b = {20};
        r.record(tb.run_vec_test("vec add single", a, b, 1, 0));
    }

    // 8b: Multi-element ADD
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> a = {1, 2, 3, 4}, b = {5, 6, 7, 8};
        r.record(tb.run_vec_test("vec add 4-elem", a, b, 4, 0));
    }

    // 8c: ADD saturation positive
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> a = {100, 127}, b = {100, 127};
        r.record(tb.run_vec_test("vec add sat-pos", a, b, 2, 0));
    }

    // 8d: ADD saturation negative
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> a = {-100, -128}, b = {-100, -128};
        r.record(tb.run_vec_test("vec add sat-neg", a, b, 2, 0));
    }

    // 8e: MUL no shift (small values to avoid overflow)
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> a = {3, -2, 5, 0}, b = {4, 7, -3, 10};
        r.record(tb.run_vec_test("vec mul no-shift", a, b, 4, 1, 0));
    }

    // 8f: MUL with quant shift
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> a = {50, -60, 70, -80};
        std::vector<int8_t> b = {40, 30, -20, 10};
        r.record(tb.run_vec_test("vec mul qshift=7", a, b, 4, 1, 7));
    }

    // 8g: MUL + ReLU
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> a = {10, -10, 5, -5};
        std::vector<int8_t> b = {3,   3, -4, -4};
        r.record(tb.run_vec_test("vec mul relu", a, b, 4, 1, 0, ACT_MODE_RELU));
    }

    // 8h: MUL + Leaky ReLU
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> a = {10, -10, 5, -5};
        std::vector<int8_t> b = {3,   3, -4, -4};
        r.record(tb.run_vec_test("vec mul leaky", a, b, 4, 1, 0, ACT_MODE_LEAKY_RELU));
    }

    // 8i: Random ADD
    {
        std::mt19937 rng(200);
        NpuTb tb;
        tb.reset();
        tb.enable();
        auto a = rand_fill(32, rng);
        auto b = rand_fill(32, rng);
        r.record(tb.run_vec_test("vec rand add-32", a, b, 32, 0));
    }

    // 8j: Random MUL with shift
    {
        std::mt19937 rng(201);
        NpuTb tb;
        tb.reset();
        tb.enable();
        auto a = rand_fill(32, rng);
        auto b = rand_fill(32, rng);
        r.record(tb.run_vec_test("vec rand mul-32", a, b, 32, 1, 8));
    }

    // 8k: Vec then conv on same instance (backend interleave)
    {
        NpuTb tb;
        tb.reset();
        tb.enable();

        // First: vec add 4
        std::vector<int8_t> va = {1, 2, 3, 4}, vb = {5, 6, 7, 8};
        bool vec_ok = tb.run_vec_test("vec-then-conv (vec)", va, vb, 4, 0);

        // Second: conv 4x4x1 k3
        auto ca  = seq_fill(4 * 4, 1);
        auto cwt = const_fill(9, 1);
        bool conv_ok = tb.run_conv_test("vec-then-conv (conv)",
                                        ca, cwt, {},
                                        4, 4, 1, 1, 3, 3,
                                        1, 1, 0, 0, 0);
        r.record(vec_ok && conv_ok);
    }
}

// Section 11: DMA tests
static void run_dma_tests(TestResult &r)
{
    printf("\n--- DMA Tests ---\n\n");

    // 11a: DMA read 4 bytes into weight buffer
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> data = {10, 20, 30, 40};
        r.record(tb.run_dma_read_test("dma read 4B -> wt",
                                      data, 0x1000, WEIGHT_BUF_BASE));
    }

    // 11b: DMA read 8 bytes into activation buffer
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> data = {1, 2, 3, 4, 5, 6, 7, 8};
        r.record(tb.run_dma_read_test("dma read 8B -> act",
                                      data, 0x2000, ACT_BUF_BASE));
    }

    // 11c: DMA write 4 bytes from weight buffer to external
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> data = {-1, -2, -3, -4};
        r.record(tb.run_dma_write_test("dma write 4B wt -> ext",
                                       data, WEIGHT_BUF_BASE, 0x5000));
    }

    // 11d: DMA write 8 bytes from activation buffer to external
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> data = {11, 22, 33, 44, 55, 66, 77, 88};
        r.record(tb.run_dma_write_test("dma write 8B act -> ext",
                                       data, ACT_BUF_BASE, 0x6000));
    }

    // 11e: DMA read with negative values
    {
        NpuTb tb;
        tb.reset();
        tb.enable();
        std::vector<int8_t> data = {-128, -64, 0, 64, 127};
        r.record(tb.run_dma_read_test("dma read signed",
                                      data, 0x3000, ACT_BUF_BASE));
    }

    // 11f: DMA read then compute (DMA loads weights, MMIO loads activations)
    {
        NpuTb tb;
        tb.reset();
        tb.enable();

        // DMA weights into weight buffer
        std::vector<int8_t> wt = const_fill(9, 1);  // 3x3 all-one filter
        tb.ext_load_bytes(0x8000, wt);
        bool dma_ok = tb.dma_transfer(0x8000, WEIGHT_BUF_BASE,
                                      static_cast<uint32_t>(wt.size()), 0);
        if (!dma_ok)
        {
            printf("  [FAIL] dma-then-conv: DMA TIMEOUT\n");
            r.record(false);
        }
        else
        {
            tb.mmio_write(REG_IRQ_CLEAR, 1);

            // Also place zeros for bias after weights
            uint32_t bias_addr = static_cast<uint32_t>(wt.size());
            tb.mmio_write(WEIGHT_BUF_BASE + bias_addr, 0);

            // MMIO-load activations
            auto act = seq_fill(4 * 4, 1);
            tb.load_bytes(ACT_BUF_BASE, act);

            // Run conv
            ConvDesc desc{};
            desc.opcode       = 1;
            desc.act_in_addr  = 0;
            desc.act_out_addr = 2048;
            desc.weight_addr  = 0;
            desc.bias_addr    = bias_addr;
            desc.in_h = 4; desc.in_w = 4; desc.in_c = 1;
            desc.out_k = 1; desc.filt_r = 3; desc.filt_s = 3;
            desc.stride_h = 1; desc.stride_w = 1;
            desc.pad_h = 0; desc.pad_w = 0;
            desc.quant_shift = 0;
            desc.act_mode = ACT_MODE_RELU;
            tb.submit_conv(desc);

            if (!tb.wait_idle())
            {
                printf("  [FAIL] dma-then-conv: conv TIMEOUT\n");
                r.record(false);
            }
            else
            {
                auto expected = ref_conv(act, wt, {}, 4, 4, 1, 1, 3, 3,
                                         1, 1, 0, 0, 0, ACT_MODE_RELU);
                auto got = tb.read_bytes(ACT_BUF_BASE + 2048,
                                         static_cast<int>(expected.size()));
                bool pass = true;
                for (int i = 0; i < static_cast<int>(expected.size()); ++i)
                {
                    if (got[i] != expected[i])
                    {
                        printf("  [FAIL] dma-then-conv: out[%d] exp %d got %d\n",
                               i, expected[i], got[i]);
                        pass = false;
                    }
                }
                if (pass)
                    printf("  [PASS] dma-then-conv (%d outputs)\n",
                           static_cast<int>(expected.size()));
                r.record(pass);
            }
        }
    }

    // 11g: DMA write results out (compute, then DMA output to ext memory)
    {
        NpuTb tb;
        tb.reset();
        tb.enable();

        // Run a simple conv first
        auto act = seq_fill(4 * 4, 1);
        auto wt  = const_fill(9, 1);
        bool conv_ok = tb.run_conv_test("dma-write-result (conv)",
                                        act, wt, {},
                                        4, 4, 1, 1, 3, 3,
                                        1, 1, 0, 0, 0);
        if (!conv_ok)
        {
            r.record(false);
        }
        else
        {
            // DMA the output (2x2 = 4 bytes at act_out_addr=2048) to ext
            bool dma_ok = tb.dma_transfer(0xA000, ACT_BUF_BASE + 2048, 4, 1);
            if (!dma_ok)
            {
                printf("  [FAIL] dma-write-result: DMA TIMEOUT\n");
                r.record(false);
            }
            else
            {
                auto expected = ref_conv(act, wt, {}, 4, 4, 1, 1, 3, 3,
                                         1, 1, 0, 0, 0, ACT_MODE_RELU);
                auto got = tb.ext_read_bytes(0xA000,
                                             static_cast<int>(expected.size()));
                bool pass = true;
                for (int i = 0; i < static_cast<int>(expected.size()); ++i)
                {
                    if (got[i] != expected[i])
                    {
                        printf("  [FAIL] dma-write-result: ext[%d] exp %d got %d\n",
                               i, expected[i], got[i]);
                        pass = false;
                    }
                }
                if (pass)
                    printf("  [PASS] dma-write-result (%d bytes)\n",
                           static_cast<int>(expected.size()));
                r.record(pass);
            }
        }
    }

    // 11h: DMA status register reflects busy
    {
        NpuTb tb;
        tb.reset();
        tb.enable();

        // After reset, DMA should not be busy
        uint32_t status = tb.mmio_read(REG_DMA_STATUS);
        bool pass = (status & 1) == 0;
        if (!pass)
            printf("  [FAIL] dma status idle: got 0x%x\n", status);
        else
            printf("  [PASS] dma status idle\n");
        r.record(pass);
    }

    // 11i: DMA register read-back
    {
        NpuTb tb;
        tb.reset();
        tb.enable();

        tb.mmio_write(REG_DMA_EXT_ADDR, 0xDEAD0000);
        tb.mmio_write(REG_DMA_LOC_ADDR, WEIGHT_BUF_BASE);
        tb.mmio_write(REG_DMA_LEN, 42);

        uint32_t ext = tb.mmio_read(REG_DMA_EXT_ADDR);
        uint32_t loc = tb.mmio_read(REG_DMA_LOC_ADDR);
        uint32_t len = tb.mmio_read(REG_DMA_LEN);

        bool pass = (ext == 0xDEAD0000) &&
                    (loc == WEIGHT_BUF_BASE) &&
                    (len == 42);
        if (!pass)
            printf("  [FAIL] dma reg readback: ext=0x%x loc=0x%x len=%u\n",
                   ext, loc, len);
        else
            printf("  [PASS] dma reg readback\n");
        r.record(pass);
    }

    // 11j: Random DMA round-trip (ext -> local -> ext)
    {
        std::mt19937 rng(500);
        NpuTb tb;
        tb.reset();
        tb.enable();

        std::uniform_int_distribution<int> dist(-128, 127);
        std::vector<int8_t> data(32);
        for (auto &x : data) x = static_cast<int8_t>(dist(rng));

        // DMA read into act buffer
        bool rd_ok = tb.run_dma_read_test("dma round-trip (read)",
                                          data, 0xC000, ACT_BUF_BASE);

        // DMA write back out to different ext address
        if (rd_ok)
        {
            bool wr_ok = tb.dma_transfer(0xD000, ACT_BUF_BASE,
                                         static_cast<uint32_t>(data.size()), 1);
            if (!wr_ok)
            {
                printf("  [FAIL] dma round-trip (write): TIMEOUT\n");
                r.record(false);
            }
            else
            {
                auto got = tb.ext_read_bytes(0xD000,
                                             static_cast<int>(data.size()));
                bool pass = true;
                for (int i = 0; i < static_cast<int>(data.size()); ++i)
                {
                    if (got[i] != data[i])
                    {
                        printf("  [FAIL] dma round-trip: byte[%d] exp %d got %d\n",
                               i, data[i], got[i]);
                        pass = false;
                    }
                }
                if (pass)
                    printf("  [PASS] dma round-trip (%d bytes)\n",
                           static_cast<int>(data.size()));
                r.record(pass);
            }
        }
        else
        {
            r.record(false);
        }
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
    run_activation_tests(r);
    run_gemm_tests(r);
    run_softmax_tests(r);
    run_vec_tests(r);
    run_lnorm_tests(r);
    run_pool_tests(r);
    run_dma_tests(r);
    run_invalid_command_tests(r);

    r.summary("Full Regression");
    return r.exit_code();
}
