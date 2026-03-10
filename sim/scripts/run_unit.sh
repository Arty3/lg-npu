#!/usr/bin/env bash
# ============================================================================
# run_unit.sh - Compile and run unit-level testbenches with Verilator
# ============================================================================
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
FILELIST="${REPO_ROOT}/tools/lint/rtl.f"
BUILD_DIR="${REPO_ROOT}/sim/build/unit"
RESULTS="${REPO_ROOT}/sim/results/unit"
PASS=0; FAIL=0

mkdir -p "${BUILD_DIR}" "${RESULTS}"

UNIT_TBS=(
    "tb/unit/common/fifo_tb.sv:fifo_tb"
    "tb/unit/common/arbiter_rr_tb.sv:arbiter_rr_tb"
    "tb/unit/conv/conv_pe_tb.sv:conv_pe_tb"
    "tb/unit/conv/conv_array_tb.sv:conv_array_tb"
    "tb/unit/conv/conv_quantize_tb.sv:conv_quantize_tb"
    "tb/unit/conv/conv_line_buffer_tb.sv:conv_line_buffer_tb"
    "tb/unit/conv/conv_window_gen_tb.sv:conv_window_gen_tb"
    "tb/unit/control/npu_queue_tb.sv:npu_queue_tb"
    "tb/unit/control/npu_reg_block_tb.sv:npu_reg_block_tb"
    "tb/unit/memory/npu_dma_reader_tb.sv:npu_dma_reader_tb"
    "tb/unit/memory/npu_dma_writer_tb.sv:npu_dma_writer_tb"
    "tb/unit/memory/npu_scratchpad_tb.sv:npu_scratchpad_tb"
)

for entry in "${UNIT_TBS[@]}"; do
    TB_FILE="${entry%%:*}"
    TB_TOP="${entry##*:}"
    TB_PATH="${REPO_ROOT}/${TB_FILE}"

    # Skip empty / placeholder testbenches
    if [ ! -s "${TB_PATH}" ]; then
        echo "[SKIP] ${TB_TOP} (empty)"
        continue
    fi

    echo "──── ${TB_TOP} ────"
    MDIR="${BUILD_DIR}/${TB_TOP}"

    if verilator --sv --cc --exe --build \
        --top-module "${TB_TOP}" \
        -Wall -Wno-MULTITOP \
        --Mdir "${MDIR}" \
        --trace \
        -CFLAGS "-std=c++17" \
        -f "${FILELIST}" \
        "${TB_PATH}" \
        > "${RESULTS}/${TB_TOP}.compile.log" 2>&1; then

        if "${MDIR}/V${TB_TOP}" +trace \
            > "${RESULTS}/${TB_TOP}.run.log" 2>&1; then
            echo "[PASS] ${TB_TOP}"
            PASS=$((PASS + 1))
        else
            echo "[FAIL] ${TB_TOP} (runtime)"
            FAIL=$((FAIL + 1))
        fi
    else
        echo "[FAIL] ${TB_TOP} (compile)"
        FAIL=$((FAIL + 1))
    fi
done

echo ""
echo "Unit results: ${PASS} passed, ${FAIL} failed"
exit "${FAIL}"