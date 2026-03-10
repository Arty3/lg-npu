// npu_e2e_harness.cpp - Verilator E2E simulation harness
//
// Drives MMIO to: load weights -> load activations -> submit command ->
// wait for completion -> read back output -> compare with expected values.

#include "Vnpu_shell.h"
#include "verilated.h"
#include "verilated_vcd_c.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>

// Address map (must match npu_addrmap_pkg.sv)
static constexpr uint32_t REG_CTRL        = 0x00000;
static constexpr uint32_t REG_STATUS      = 0x00004;
static constexpr uint32_t REG_DOORBELL    = 0x00008;
static constexpr uint32_t CMD_QUEUE_BASE  = 0x01000;
static constexpr uint32_t WEIGHT_BUF_BASE = 0x10000;
static constexpr uint32_t ACT_BUF_BASE    = 0x20000;

// Status register bits
static constexpr uint32_t STATUS_IDLE = 1u << 0;

// Test parameters (4x4x1 input, 3x3x1x1 filter, stride=1, pad=0)
static constexpr int IN_H  = 4, IN_W  = 4, IN_C = 1;
static constexpr int OUT_K = 1, FILT_R = 3, FILT_S = 3;
static constexpr int STRIDE_H = 1, STRIDE_W = 1;
static constexpr int PAD_H = 0, PAD_W = 0;
static constexpr int QUANT_SHIFT = 0;
static constexpr int OUT_H = (IN_H - FILT_R) / STRIDE_H + 1;  // 2
static constexpr int OUT_W = (IN_W - FILT_S) / STRIDE_W + 1;  // 2
static constexpr uint32_t ACT_OUT_ADDR = 4096;

// Activations: 1..16 (NHWC layout, C=1)
static const int8_t activations[] = {
     1,  2,  3,  4,
     5,  6,  7,  8,
     9, 10, 11, 12,
    13, 14, 15, 16,
};

// Weights: all 1 (K=1, R=3, S=3, C=1)
static const int8_t weights[] = {
    1, 1, 1,
    1, 1, 1,
    1, 1, 1,
};

// Expected output (from Python reference model / manual calculation)
//   out[oh][ow] = sum of 3x3 window of activations
//   [0,0]=54  [0,1]=63  [1,0]=90  [1,1]=99
static const int8_t expected[] = { 54, 63, 90, 99 };

class NpuTb
{
    Vnpu_shell     *dut;
    VerilatedVcdC  *trace;
    uint64_t        sim_time;

public:
    NpuTb() : sim_time(0)
    {
        dut = new Vnpu_shell;
        Verilated::traceEverOn(true);
        trace = new VerilatedVcdC;
        dut->trace(trace, 99);
        trace->open("sim/waves/e2e.vcd");
    }

    ~NpuTb()
    {
        trace->close();
        delete trace;
        delete dut;
    }

    // Advance one full clock cycle (negedge then posedge)
    void tick()
    {
        dut->clk = 0;
        dut->eval();
        trace->dump(sim_time++);
        dut->clk = 1;
        dut->eval();
        trace->dump(sim_time++);
    }

    void reset(int cycles = 10)
    {
        dut->rst_async_n = 0;
        dut->mmio_valid  = 0;
        dut->mmio_wr     = 0;
        dut->mmio_addr   = 0;
        dut->mmio_wdata  = 0;
        for (int i = 0; i < cycles; ++i)
            tick();
        dut->rst_async_n = 1;
        tick();
        tick();
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

    bool run()
    {
        printf("[E2E] Resetting ...\n");
        reset();

        // Enable NPU (CTRL.ENABLE = bit 1)
        printf("[E2E] Enabling NPU\n");
        mmio_write(REG_CTRL, 0x2);

        // Load weights into SRAM
        printf("[E2E] Loading %d weights\n", OUT_K * FILT_R * FILT_S * IN_C);
        for (int i = 0; i < OUT_K * FILT_R * FILT_S * IN_C; ++i)
            mmio_write(WEIGHT_BUF_BASE + i,
                       static_cast<uint32_t>(static_cast<uint8_t>(weights[i])));

        // Verify weights written correctly
        printf("[E2E] Verifying weights...\n");
        for (int i = 0; i < OUT_K * FILT_R * FILT_S * IN_C; ++i)
        {
            uint32_t raw = mmio_read(WEIGHT_BUF_BASE + i);
            auto got = static_cast<uint8_t>(raw & 0xFF);
            if (got != static_cast<uint8_t>(weights[i]))
                printf("[E2E]  wt[%d]: wrote %d, read %d\n", i,
                       static_cast<uint8_t>(weights[i]), got);
        }

        // Load activations into SRAM
        printf("[E2E] Loading %d activations\n", IN_H * IN_W * IN_C);
        for (int i = 0; i < IN_H * IN_W * IN_C; ++i)
            mmio_write(ACT_BUF_BASE + i,
                       static_cast<uint32_t>(static_cast<uint8_t>(activations[i])));

        // Build 16-word command descriptor
        uint32_t cmd[16] = {};
        cmd[0]  = 1;             // opcode = OP_CONV
        cmd[1]  = 0;             // act_in_addr
        cmd[2]  = ACT_OUT_ADDR;  // act_out_addr
        cmd[3]  = 0;             // weight_addr
        cmd[4]  = 0;             // bias_addr (stubbed)
        cmd[5]  = IN_H;
        cmd[6]  = IN_W;
        cmd[7]  = IN_C;
        cmd[8]  = OUT_K;
        cmd[9]  = FILT_R;
        cmd[10] = FILT_S;
        cmd[11] = STRIDE_H;
        cmd[12] = STRIDE_W;
        cmd[13] = PAD_H;
        cmd[14] = PAD_W;
        cmd[15] = QUANT_SHIFT;

        printf("[E2E] Writing command descriptor\n");
        for (int i = 0; i < 16; ++i)
            mmio_write(CMD_QUEUE_BASE + i * 4, cmd[i]);

        // Ring doorbell
        printf("[E2E] Ringing doorbell\n");
        mmio_write(REG_DOORBELL, 1);

        // Verify the bus is working - read back CTRL
        uint32_t ctrl_rb = mmio_read(REG_CTRL);
        printf("[E2E] CTRL readback = 0x%08x (expect 0x2)\n", ctrl_rb);

        // Free-run then start polling
        printf("[E2E] Free-running 2000 ticks ...\n");
        for (int t = 0; t < 2000; ++t)
            tick();

        // Read status once outside of polling to see value
        uint32_t st_snap = mmio_read(REG_STATUS);
        printf("[E2E] STATUS snapshot = 0x%08x\n", st_snap);
        uint32_t irq_snap = mmio_read(0x0000C); // REG_IRQ_STATUS
        printf("[E2E] IRQ_STATUS = 0x%08x\n", irq_snap);

        // Poll STATUS.IDLE
        printf("[E2E] Waiting for completion ...\n");
        int timeout = 200000;
        while (--timeout > 0)
        {
            uint32_t status = mmio_read(REG_STATUS);
            if ((200000 - timeout) <= 10 || (200000 - timeout) % 10000 == 0)
                printf("[E2E]   poll %d: status=0x%08x (idle=%d busy=%d qfull=%d)\n",
                       200000 - timeout, status,
                       (status >> 0) & 1, (status >> 1) & 1, (status >> 2) & 1);
            if (status & STATUS_IDLE)
                break;
        }
        if (timeout == 0)
        {
            printf("[E2E] TIMEOUT waiting for idle\n");
            return false;
        }
        printf("[E2E] Done (idle after %d polls)\n", 200000 - timeout);

        // Diagnostic: scan weight buffer
        printf("[E2E] Scanning weight buffer (0..8):\n");
        for (int i = 0; i < 9; ++i)
        {
            uint32_t raw = mmio_read(WEIGHT_BUF_BASE + i);
            printf("  wt[%d] = 0x%02x (%d)\n", i, raw & 0xFF, static_cast<int8_t>(raw & 0xFF));
        }

        // Diagnostic: scan activation buffer around expected output area
        printf("[E2E] Scanning activation buffer (input area 0..15):\n");
        for (int i = 0; i < 16; ++i)
        {
            uint32_t raw = mmio_read(ACT_BUF_BASE + i);
            printf("  act[%d] = 0x%02x (%d)\n", i, raw & 0xFF, static_cast<int8_t>(raw & 0xFF));
        }
        printf("[E2E] Scanning activation buffer (output area %d..%d):\n",
               ACT_OUT_ADDR, ACT_OUT_ADDR + 3);
        for (int i = 0; i < 4; ++i)
        {
            uint32_t raw = mmio_read(ACT_BUF_BASE + ACT_OUT_ADDR + i);
            printf("  out[%d] = 0x%02x (%d)\n",
                   ACT_OUT_ADDR + i, raw & 0xFF, static_cast<int8_t>(raw & 0xFF));
        }

        // Read output and compare
        bool pass = true;
        int  out_size = OUT_H * OUT_W * OUT_K;
        for (int i = 0; i < out_size; ++i)
        {
            uint32_t raw = mmio_read(ACT_BUF_BASE + ACT_OUT_ADDR + i);
            auto got = static_cast<int8_t>(raw & 0xFF);
            if (got != expected[i])
            {
                printf("[E2E] MISMATCH output[%d]: expected %d, got %d\n",
                       i, expected[i], got);
                pass = false;
            }
            else
            {
                printf("[E2E] output[%d] = %d  OK\n", i, got);
            }
        }

        return pass;
    }
};

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);

    NpuTb tb;
    bool pass = tb.run();

    printf(pass ? "\n*** E2E TEST PASSED ***\n"
                : "\n*** E2E TEST FAILED ***\n");
    return pass ? 0 : 1;
}
