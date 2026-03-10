// ============================================================================
// npu_tb.h - Reusable Verilator testbench harness for npu_shell
// ============================================================================

#ifndef NPU_TB_H
#define NPU_TB_H

#include "Vnpu_shell.h"
#include "verilated.h"
#include "verilated_vcd_c.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

// Address map (matches npu_addrmap_pkg.sv)
static constexpr uint32_t REG_CTRL        = 0x00000;
static constexpr uint32_t REG_STATUS      = 0x00004;
static constexpr uint32_t REG_DOORBELL    = 0x00008;
static constexpr uint32_t REG_IRQ_STATUS  = 0x0000C;
static constexpr uint32_t REG_IRQ_ENABLE  = 0x00010;
static constexpr uint32_t REG_IRQ_CLEAR   = 0x00014;
static constexpr uint32_t REG_FEATURE_ID  = 0x00018;
static constexpr uint32_t REG_PERF_CYCLES = 0x00020;
static constexpr uint32_t REG_PERF_ACTIVE = 0x00024;
static constexpr uint32_t REG_PERF_STALL  = 0x00028;
static constexpr uint32_t CMD_QUEUE_BASE  = 0x01000;
static constexpr uint32_t WEIGHT_BUF_BASE = 0x10000;
static constexpr uint32_t ACT_BUF_BASE    = 0x20000;
static constexpr uint32_t PSUM_BUF_BASE   = 0x30000;

// CTRL register bits
static constexpr uint32_t CTRL_SOFT_RESET_BIT = 0;
static constexpr uint32_t CTRL_ENABLE_BIT     = 1;

// Status register bits
static constexpr uint32_t STATUS_IDLE_BIT       = 0;
static constexpr uint32_t STATUS_BUSY_BIT       = 1;
static constexpr uint32_t STATUS_QUEUE_FULL_BIT = 2;

// Convolution command descriptor (16 words)
struct ConvDesc
{
    uint32_t opcode;
    uint32_t act_in_addr;
    uint32_t act_out_addr;
    uint32_t weight_addr;
    uint32_t bias_addr;
    uint32_t in_h, in_w, in_c;
    uint32_t out_k;
    uint32_t filt_r, filt_s;
    uint32_t stride_h, stride_w;
    uint32_t pad_h, pad_w;
    uint32_t quant_shift;
    uint32_t act_mode;      // 0=None (default), 1=ReLU, 2=Leaky ReLU

    void to_words(uint32_t out[16]) const
    {
        out[0]  = opcode;
        out[1]  = act_in_addr;
        out[2]  = act_out_addr;
        out[3]  = weight_addr;
        out[4]  = bias_addr;
        out[5]  = in_h;
        out[6]  = in_w;
        out[7]  = in_c;
        out[8]  = out_k;
        out[9]  = filt_r;
        out[10] = filt_s;
        out[11] = stride_h;
        out[12] = stride_w;
        out[13] = pad_h;
        out[14] = pad_w;
        out[15] = (quant_shift & 0x1F) | ((act_mode & 0x3) << 5);
    }
};

// GEMM command descriptor (16 words, same transport as ConvDesc)
struct GemmDesc
{
    uint32_t opcode    = 2;  // OP_GEMM
    uint32_t a_addr    = 0;  // Matrix A base (activation buffer)
    uint32_t c_addr    = 0;  // Output matrix C base (activation buffer)
    uint32_t b_addr    = 0;  // Matrix B base (weight buffer)
    uint32_t bias_addr = 0;  // Bias vector base (weight buffer)
    uint32_t m_dim     = 0;
    uint32_t n_dim     = 0;
    uint32_t k_dim     = 0;
    uint32_t quant_shift = 0;
    uint32_t act_mode    = 0; // ACT_MODE_NONE

    void to_words(uint32_t out[16]) const
    {
        std::memset(out, 0, sizeof(uint32_t) * 16);
        out[0]  = opcode;
        out[1]  = a_addr;
        out[2]  = c_addr;
        out[3]  = b_addr;
        out[4]  = bias_addr;
        out[5]  = m_dim;
        out[6]  = n_dim;
        out[7]  = k_dim;
        out[15] = (quant_shift & 0x1F) | ((act_mode & 0x3) << 5);
    }
};

// Softmax command descriptor (16 words, same transport as ConvDesc)
struct SoftmaxDesc
{
    uint32_t opcode   = 3;  // OP_SOFTMAX
    uint32_t in_addr  = 0;  // Input base (activation buffer)
    uint32_t out_addr = 0;  // Output base (activation buffer)
    uint32_t num_rows = 0;  // M - independent softmax rows
    uint32_t row_len  = 0;  // N - elements per row

    void to_words(uint32_t out[16]) const
    {
        std::memset(out, 0, sizeof(uint32_t) * 16);
        out[0] = opcode;
        out[1] = in_addr;
        out[2] = out_addr;
        out[5] = num_rows;
        out[6] = row_len;
    }
};

// Vec command descriptor (16 words, same transport as ConvDesc)
struct VecDesc
{
    uint32_t opcode      = 4;  // OP_VEC
    uint32_t a_addr      = 0;  // Source A base (activation buffer)
    uint32_t out_addr    = 0;  // Output base (activation buffer)
    uint32_t b_addr      = 0;  // Source B base (weight buffer)
    uint32_t length      = 0;  // Number of elements
    uint32_t vec_op      = 0;  // 0=ADD, 1=MUL
    uint32_t quant_shift = 0;
    uint32_t act_mode    = 0;

    void to_words(uint32_t out[16]) const
    {
        std::memset(out, 0, sizeof(uint32_t) * 16);
        out[0]  = opcode;
        out[1]  = a_addr;
        out[2]  = out_addr;
        out[3]  = b_addr;
        out[5]  = length;
        out[6]  = vec_op;
        out[15] = (quant_shift & 0x1F) | ((act_mode & 0x3) << 5);
    }
};

// Activation mode constants (match act_mode_e in npu_types_pkg.sv)
static constexpr uint32_t ACT_MODE_NONE       = 0;
static constexpr uint32_t ACT_MODE_RELU       = 1;
static constexpr uint32_t ACT_MODE_LEAKY_RELU = 2;

// Leaky ReLU shift (must match LEAKY_SHIFT in npu_types_pkg.sv)
static constexpr int LEAKY_SHIFT = 3;

// C++ reference GEMM: C = A * B + bias (INT8 -> INT32 accum -> INT8 out)
//   A is row-major M x K, B is row-major K x N, bias is length N.
inline std::vector<int8_t> ref_gemm(
    const std::vector<int8_t> &a,
    const std::vector<int8_t> &b,
    const std::vector<int8_t> &bias,
    int M, int N, int K,
    int quant_shift,
    uint32_t act_mode = ACT_MODE_NONE)
{
    std::vector<int8_t> out(M * N);
    for (int m = 0; m < M; ++m)
    {
        for (int n = 0; n < N; ++n)
        {
            int32_t acc = 0;
            for (int k = 0; k < K; ++k)
                acc += static_cast<int32_t>(a[m * K + k])
                     * static_cast<int32_t>(b[k * N + n]);
            // Bias
            if (!bias.empty())
                acc += static_cast<int32_t>(bias[n]);
            // Activation
            if (act_mode == ACT_MODE_RELU)
            {
                if (acc < 0) acc = 0;
            }
            else if (act_mode == ACT_MODE_LEAKY_RELU)
            {
                if (acc < 0) acc >>= LEAKY_SHIFT;
            }
            // Quantize
            acc >>= quant_shift;
            if (acc > 127) acc = 127;
            if (acc < -128) acc = -128;
            out[m * N + n] = static_cast<int8_t>(acc);
        }
    }
    return out;
}

// C++ reference convolution (INT8 in -> INT32 accum -> INT8 out)
inline std::vector<int8_t> ref_conv(
    const std::vector<int8_t> &act,
    const std::vector<int8_t> &wt,
    const std::vector<int8_t> &bias,
    int H, int W, int C, int K, int R, int S,
    int stride_h, int stride_w, int pad_h, int pad_w,
    int quant_shift,
    uint32_t act_mode = ACT_MODE_RELU)
{
    int OH = (H + 2 * pad_h - R) / stride_h + 1;
    int OW = (W + 2 * pad_w - S) / stride_w + 1;
    std::vector<int8_t> out(OH * OW * K);

    for (int oh = 0; oh < OH; ++oh)
    {
        for (int ow = 0; ow < OW; ++ow)
        {
            for (int k = 0; k < K; ++k)
            {
                int32_t acc = 0;
                for (int r = 0; r < R; ++r)
                {
                    for (int s = 0; s < S; ++s)
                    {
                        for (int c = 0; c < C; ++c)
                        {
                            int ih = oh * stride_h + r - pad_h;
                            int iw = ow * stride_w + s - pad_w;
                            int8_t a = 0;
                            if (ih >= 0 && ih < H && iw >= 0 && iw < W)
                                a = act[(ih * W + iw) * C + c];
                            int8_t w = wt[((k * R + r) * S + s) * C + c];
                            acc += static_cast<int32_t>(a) * static_cast<int32_t>(w);
                        }
                    }
                }
                // Bias
                if (!bias.empty())
                    acc += static_cast<int32_t>(bias[k]);
                // Activation
                if (act_mode == ACT_MODE_RELU)
                {
                    if (acc < 0) acc = 0;
                }
                else if (act_mode == ACT_MODE_LEAKY_RELU)
                {
                    if (acc < 0) acc >>= LEAKY_SHIFT;
                }
                // ACT_MODE_NONE: passthrough
                // Quantize (right arith shift + saturate to [-128, 127])
                acc >>= quant_shift;
                if (acc > 127) acc = 127;
                if (acc < -128) acc = -128;
                out[(oh * OW + ow) * K + k] = static_cast<int8_t>(acc);
            }
        }
    }
    return out;
}

// Softmax exp approximation matching hardware (softmax_exp_lut.sv)
//   Decompose diff into int_part (d[7:5]) and frac_part (d[4:0]).
//   Result = (exp_int[int_part] * exp_frac[frac_part]) >> 16.
inline uint16_t softmax_exp_approx(uint8_t diff)
{
    static const uint16_t exp_int[8] = {
        65535, 24109, 8869, 3263, 1200, 442, 162, 60
    };
    static const uint16_t exp_frac[32] = {
        65535, 63519, 61564, 59670, 57834, 56055, 54330, 52659,
        51039, 49468, 47946, 46471, 45042, 43656, 42313, 41011,
        39749, 38526, 37341, 36192, 35078, 33999, 32953, 31939,
        30957, 30004, 29081, 28186, 27319, 26479, 25664, 24874
    };
    uint32_t product = static_cast<uint32_t>(exp_int[diff >> 5])
                     * static_cast<uint32_t>(exp_frac[diff & 31]);
    return static_cast<uint16_t>(product >> 16);
}

// C++ reference softmax (INT8 in -> INT8 out [0,127])
//   Uses same two-table exp LUT decomposition as hardware for
//   bit-exact matching.
inline std::vector<int8_t> ref_softmax(
    const std::vector<int8_t> &input,
    int num_rows, int row_len)
{
    std::vector<int8_t> out(num_rows * row_len);
    for (int r = 0; r < num_rows; ++r)
    {
        int base = r * row_len;
        // Pass 1: find max
        int8_t max_val = -128;
        for (int i = 0; i < row_len; ++i)
            if (input[base + i] > max_val) max_val = input[base + i];
        // Pass 2: compute exp and accumulate sum
        uint32_t sum = 0;
        for (int i = 0; i < row_len; ++i)
        {
            uint8_t diff = static_cast<uint8_t>(max_val) -
                           static_cast<uint8_t>(input[base + i]);
            sum += softmax_exp_approx(diff);
        }
        // Pass 3: normalize - matches hardware restoring divider (floor)
        for (int i = 0; i < row_len; ++i)
        {
            uint8_t diff = static_cast<uint8_t>(max_val) -
                           static_cast<uint8_t>(input[base + i]);
            uint16_t exp_val = softmax_exp_approx(diff);
            uint32_t numer = static_cast<uint32_t>(exp_val) * 127u;
            uint8_t quot = static_cast<uint8_t>(numer / sum);
            if (quot > 127) quot = 127;
            out[base + i] = static_cast<int8_t>(quot);
        }
    }
    return out;
}

// C++ reference vec: element-wise ADD or MUL (INT8 in -> INT8 out)
//   vec_op=0: C[i] = clamp(A[i] + B[i], -128, 127)
//   vec_op!=0: C[i] = quantize(activate(A[i]*B[i], mode), shift)
inline std::vector<int8_t> ref_vec(
    const std::vector<int8_t> &a,
    const std::vector<int8_t> &b,
    int length,
    uint32_t vec_op,
    int quant_shift = 0,
    uint32_t act_mode = ACT_MODE_NONE)
{
    std::vector<int8_t> out(length);
    for (int i = 0; i < length; ++i)
    {
        if (vec_op == 0)
        {
            int32_t sum = static_cast<int32_t>(a[i]) + static_cast<int32_t>(b[i]);
            if (sum > 127) sum = 127;
            if (sum < -128) sum = -128;
            out[i] = static_cast<int8_t>(sum);
        }
        else
        {
            int32_t acc = static_cast<int32_t>(a[i]) * static_cast<int32_t>(b[i]);
            if (act_mode == ACT_MODE_RELU)
            {
                if (acc < 0) acc = 0;
            }
            else if (act_mode == ACT_MODE_LEAKY_RELU)
            {
                if (acc < 0) acc >>= LEAKY_SHIFT;
            }
            acc >>= quant_shift;
            if (acc > 127) acc = 127;
            if (acc < -128) acc = -128;
            out[i] = static_cast<int8_t>(acc);
        }
    }
    return out;
}

// Testbench wrapper around Vnpu_shell
class NpuTb
{
    Vnpu_shell    *dut;
    VerilatedVcdC *trace;
    uint64_t       sim_time;
    bool           trace_enabled;

public:
    explicit NpuTb(const char *vcd_path = nullptr)
        : sim_time(0), trace_enabled(vcd_path != nullptr)
    {
        dut = new Vnpu_shell;
        if (trace_enabled)
        {
            Verilated::traceEverOn(true);
            trace = new VerilatedVcdC;
            dut->trace(trace, 99);
            trace->open(vcd_path);
        }
        else
        {
            trace = nullptr;
        }
    }

    ~NpuTb()
    {
        if (trace)
        {
            trace->close();
            delete trace;
        }
        delete dut;
    }

    Vnpu_shell *raw() { return dut; }
    uint64_t time() const { return sim_time; }

    void tick()
    {
        dut->clk = 0;
        dut->eval();
        if (trace) trace->dump(sim_time++);
        dut->clk = 1;
        dut->eval();
        if (trace) trace->dump(sim_time++);
    }

    void reset(int cycles = 10)
    {
        dut->rst_async_n = 0;
        dut->mmio_valid  = 0;
        dut->mmio_wr     = 0;
        dut->mmio_addr   = 0;
        dut->mmio_wdata  = 0;
        for (int i = 0; i < cycles; ++i) tick();
        dut->rst_async_n = 1;
        tick(); tick();
    }

    void mmio_write(uint32_t addr, uint32_t data)
    {
        dut->mmio_addr  = addr;
        dut->mmio_wdata = data;
        dut->mmio_wr    = 1;
        dut->mmio_valid = 1;
        do { tick(); } while (!dut->mmio_ready);
        dut->mmio_valid = 0;
        dut->mmio_wr    = 0;
    }

    uint32_t mmio_read(uint32_t addr)
    {
        dut->mmio_addr  = addr;
        dut->mmio_wr    = 0;
        dut->mmio_valid = 1;
        do { tick(); } while (!dut->mmio_ready);
        uint32_t data = dut->mmio_rdata;
        dut->mmio_valid = 0;
        return data;
    }

    void enable()
    {
        mmio_write(REG_CTRL, 1u << CTRL_ENABLE_BIT);
    }

    void soft_reset_pulse()
    {
        mmio_write(REG_CTRL, 1u << CTRL_SOFT_RESET_BIT);
        for (int i = 0; i < 5; ++i) tick();
        mmio_write(REG_CTRL, 1u << CTRL_ENABLE_BIT);
    }

    uint32_t read_status()
    {
        return mmio_read(REG_STATUS);
    }

    bool is_idle()
    {
        return (read_status() >> STATUS_IDLE_BIT) & 1;
    }

    bool is_busy()
    {
        return (read_status() >> STATUS_BUSY_BIT) & 1;
    }

    bool is_queue_full()
    {
        return (read_status() >> STATUS_QUEUE_FULL_BIT) & 1;
    }

    // Load a byte array into an SRAM window starting at mmio_base
    void load_bytes(uint32_t mmio_base, const int8_t *data, int count)
    {
        for (int i = 0; i < count; ++i)
            mmio_write(mmio_base + static_cast<uint32_t>(i),
                       static_cast<uint32_t>(static_cast<uint8_t>(data[i])));
    }

    void load_bytes(uint32_t mmio_base, const std::vector<int8_t> &data)
    {
        load_bytes(mmio_base, data.data(), static_cast<int>(data.size()));
    }

    // Read bytes back from an SRAM window
    std::vector<int8_t> read_bytes(uint32_t mmio_base, int count)
    {
        std::vector<int8_t> out(count);
        for (int i = 0; i < count; ++i)
        {
            uint32_t raw = mmio_read(mmio_base + static_cast<uint32_t>(i));
            out[i] = static_cast<int8_t>(raw & 0xFF);
        }
        return out;
    }

    // Submit a convolution command and ring the doorbell
    void submit_conv(const ConvDesc &desc)
    {
        uint32_t words[16];
        desc.to_words(words);
        for (int i = 0; i < 16; ++i)
            mmio_write(CMD_QUEUE_BASE + static_cast<uint32_t>(i) * 4, words[i]);
        mmio_write(REG_DOORBELL, 1);
        // Free-run to let the command pipeline fetch, decode, and begin
        // execution without competing for SRAM ports with MMIO reads
        // from the polling loop.
        run_clocks(2000);
    }

    // Submit a GEMM command and ring the doorbell
    void submit_gemm(const GemmDesc &desc)
    {
        uint32_t words[16];
        desc.to_words(words);
        for (int i = 0; i < 16; ++i)
            mmio_write(CMD_QUEUE_BASE + static_cast<uint32_t>(i) * 4, words[i]);
        mmio_write(REG_DOORBELL, 1);
        run_clocks(2000);
    }

    // Submit a softmax command and ring the doorbell
    void submit_softmax(const SoftmaxDesc &desc)
    {
        uint32_t words[16];
        desc.to_words(words);
        for (int i = 0; i < 16; ++i)
            mmio_write(CMD_QUEUE_BASE + static_cast<uint32_t>(i) * 4, words[i]);
        mmio_write(REG_DOORBELL, 1);
        run_clocks(2000);
    }

    // Submit a vec command and ring the doorbell
    void submit_vec(const VecDesc &desc)
    {
        uint32_t words[16];
        desc.to_words(words);
        for (int i = 0; i < 16; ++i)
            mmio_write(CMD_QUEUE_BASE + static_cast<uint32_t>(i) * 4, words[i]);
        mmio_write(REG_DOORBELL, 1);
        run_clocks(2000);
    }

    // Wait for the NPU to become idle, with a cycle timeout.
    // Returns true if idle was reached, false on timeout.
    bool wait_idle(int max_polls = 200000)
    {
        for (int i = 0; i < max_polls; ++i)
        {
            if (is_idle()) return true;
        }
        return false;
    }

    // Free-run the clock for N cycles (no MMIO activity)
    void run_clocks(int n)
    {
        for (int i = 0; i < n; ++i) tick();
    }

    // Run a convolution end-to-end: load data, fire command, wait, read back.
    // Returns true on match with expected output.
    bool run_conv_test(
        const char *name,
        const std::vector<int8_t> &act,
        const std::vector<int8_t> &wt,
        const std::vector<int8_t> &bias,
        int H, int W, int C, int K, int R, int S,
        int sh, int sw, int ph, int pw,
        int qshift,
        uint32_t act_mode     = ACT_MODE_RELU,
        uint32_t act_in_addr  = 0,
        uint32_t wt_addr      = 0,
        uint32_t bias_addr    = 0,
        uint32_t act_out_addr = 2048)
    {
        // Compute reference
        auto expected = ref_conv(act, wt, bias, H, W, C, K, R, S,
                                 sh, sw, ph, pw, qshift, act_mode);
        int out_size = static_cast<int>(expected.size());

        // Load weights
        load_bytes(WEIGHT_BUF_BASE + wt_addr, wt);

        // Hardware always reads K bias values; when none are supplied,
        // place zeros after the weights so the bias read returns 0.
        if (!bias.empty())
        {
            load_bytes(WEIGHT_BUF_BASE + bias_addr, bias);
        }
        else
        {
            bias_addr = wt_addr + static_cast<uint32_t>(wt.size());
            for (int i = 0; i < K; ++i)
                mmio_write(WEIGHT_BUF_BASE + bias_addr + static_cast<uint32_t>(i), 0);
        }

        // Load activations
        load_bytes(ACT_BUF_BASE + act_in_addr, act);

        // Build and submit command
        ConvDesc desc{};
        desc.opcode       = 1;  // OP_CONV
        desc.act_in_addr  = act_in_addr;
        desc.act_out_addr = act_out_addr;
        desc.weight_addr  = wt_addr;
        desc.bias_addr    = bias_addr;
        desc.in_h         = static_cast<uint32_t>(H);
        desc.in_w         = static_cast<uint32_t>(W);
        desc.in_c         = static_cast<uint32_t>(C);
        desc.out_k        = static_cast<uint32_t>(K);
        desc.filt_r       = static_cast<uint32_t>(R);
        desc.filt_s       = static_cast<uint32_t>(S);
        desc.stride_h     = static_cast<uint32_t>(sh);
        desc.stride_w     = static_cast<uint32_t>(sw);
        desc.pad_h        = static_cast<uint32_t>(ph);
        desc.pad_w        = static_cast<uint32_t>(pw);
        desc.quant_shift  = static_cast<uint32_t>(qshift);
        desc.act_mode     = act_mode;
        submit_conv(desc);

        // Wait for completion
        if (!wait_idle())
        {
            printf("  [FAIL] %s: TIMEOUT waiting for idle\n", name);
            return false;
        }

        // Read back and compare
        auto got = read_bytes(ACT_BUF_BASE + act_out_addr, out_size);
        bool pass = true;
        for (int i = 0; i < out_size; ++i)
        {
            if (got[i] != expected[i])
            {
                printf("  [FAIL] %s: output[%d] expected %d, got %d\n",
                       name, i, expected[i], got[i]);
                pass = false;
            }
        }
        if (pass)
            printf("  [PASS] %s (%d outputs verified)\n", name, out_size);
        return pass;
    }

    // Run a GEMM end-to-end: load data, fire command, wait, read back.
    // Returns true on match with expected output.
    bool run_gemm_test(
        const char *name,
        const std::vector<int8_t> &a,
        const std::vector<int8_t> &b,
        const std::vector<int8_t> &bias,
        int M, int N, int K,
        int qshift,
        uint32_t act_mode  = ACT_MODE_NONE,
        uint32_t a_addr    = 0,
        uint32_t b_addr    = 0,
        uint32_t bias_addr = 0,
        uint32_t c_addr    = 2048)
    {
        auto expected = ref_gemm(a, b, bias, M, N, K, qshift, act_mode);
        int out_size = static_cast<int>(expected.size());

        // Load B matrix into weight buffer
        load_bytes(WEIGHT_BUF_BASE + b_addr, b);

        // Place bias after B to avoid overlap
        bias_addr = b_addr + static_cast<uint32_t>(b.size());
        if (!bias.empty())
        {
            load_bytes(WEIGHT_BUF_BASE + bias_addr, bias);
        }
        else
        {
            for (int i = 0; i < N; ++i)
                mmio_write(WEIGHT_BUF_BASE + bias_addr
                           + static_cast<uint32_t>(i), 0);
        }

        // Load A matrix into activation buffer
        load_bytes(ACT_BUF_BASE + a_addr, a);

        // Build and submit command
        GemmDesc desc{};
        desc.a_addr      = a_addr;
        desc.c_addr      = c_addr;
        desc.b_addr      = b_addr;
        desc.bias_addr   = bias_addr;
        desc.m_dim       = static_cast<uint32_t>(M);
        desc.n_dim       = static_cast<uint32_t>(N);
        desc.k_dim       = static_cast<uint32_t>(K);
        desc.quant_shift = static_cast<uint32_t>(qshift);
        desc.act_mode    = act_mode;
        submit_gemm(desc);

        if (!wait_idle())
        {
            printf("  [FAIL] %s: TIMEOUT waiting for idle\n", name);
            return false;
        }

        auto got = read_bytes(ACT_BUF_BASE + c_addr, out_size);
        bool pass = true;
        for (int i = 0; i < out_size; ++i)
        {
            if (got[i] != expected[i])
            {
                printf("  [FAIL] %s: output[%d] expected %d, got %d\n",
                       name, i, expected[i], got[i]);
                pass = false;
            }
        }
        if (pass)
            printf("  [PASS] %s (%d outputs verified)\n", name, out_size);
        return pass;
    }

    // Run a softmax end-to-end: load data, fire command, wait, read back.
    bool run_softmax_test(
        const char *name,
        const std::vector<int8_t> &input,
        int num_rows, int row_len,
        uint32_t in_addr  = 0,
        uint32_t out_addr = 2048)
    {
        auto expected = ref_softmax(input, num_rows, row_len);
        int out_size = static_cast<int>(expected.size());

        load_bytes(ACT_BUF_BASE + in_addr, input);

        SoftmaxDesc desc{};
        desc.in_addr  = in_addr;
        desc.out_addr = out_addr;
        desc.num_rows = static_cast<uint32_t>(num_rows);
        desc.row_len  = static_cast<uint32_t>(row_len);
        submit_softmax(desc);

        if (!wait_idle())
        {
            printf("  [FAIL] %s: TIMEOUT waiting for idle\n", name);
            return false;
        }

        auto got = read_bytes(ACT_BUF_BASE + out_addr, out_size);
        bool pass = true;
        for (int i = 0; i < out_size; ++i)
        {
            if (got[i] != expected[i])
            {
                printf("  [FAIL] %s: output[%d] expected %d, got %d\n",
                       name, i, expected[i], got[i]);
                pass = false;
            }
        }
        if (pass)
            printf("  [PASS] %s (%d outputs verified)\n", name, out_size);
        return pass;
    }

    // Run a vec op end-to-end: load data, fire command, wait, read back.
    bool run_vec_test(
        const char *name,
        const std::vector<int8_t> &a,
        const std::vector<int8_t> &b,
        int length,
        uint32_t vec_op,
        int qshift            = 0,
        uint32_t act_mode     = ACT_MODE_NONE,
        uint32_t a_addr       = 0,
        uint32_t b_addr       = 0,
        uint32_t out_addr     = 2048)
    {
        auto expected = ref_vec(a, b, length, vec_op, qshift, act_mode);
        int out_size = static_cast<int>(expected.size());

        load_bytes(ACT_BUF_BASE + a_addr, a);
        load_bytes(WEIGHT_BUF_BASE + b_addr, b);

        VecDesc desc{};
        desc.a_addr      = a_addr;
        desc.out_addr    = out_addr;
        desc.b_addr      = b_addr;
        desc.length      = static_cast<uint32_t>(length);
        desc.vec_op      = vec_op;
        desc.quant_shift = static_cast<uint32_t>(qshift);
        desc.act_mode    = act_mode;
        submit_vec(desc);

        if (!wait_idle())
        {
            printf("  [FAIL] %s: TIMEOUT waiting for idle\n", name);
            return false;
        }

        auto got = read_bytes(ACT_BUF_BASE + out_addr, out_size);
        bool pass = true;
        for (int i = 0; i < out_size; ++i)
        {
            if (got[i] != expected[i])
            {
                printf("  [FAIL] %s: output[%d] expected %d, got %d\n",
                       name, i, expected[i], got[i]);
                pass = false;
            }
        }
        if (pass)
            printf("  [PASS] %s (%d outputs verified)\n", name, out_size);
        return pass;
    }
};

// Test runner helper
struct TestResult
{
    int total   = 0;
    int passed  = 0;
    int failed  = 0;

    void record(bool pass)
    {
        ++total;
        if (pass) ++passed;
        else ++failed;
    }

    int exit_code() const { return failed > 0 ? 1 : 0; }

    void summary(const char *suite) const
    {
        printf("\n=== %s: %d/%d passed", suite, passed, total);
        if (failed > 0) printf(" (%d FAILED)", failed);
        printf(" ===\n");
    }
};

#endif // NPU_TB_H
