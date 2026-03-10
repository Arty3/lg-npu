#!/usr/bin/env bash
# ============================================================================
# run_integration.sh - Compile and run integration testbenches with Verilator
# ============================================================================
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
FILELIST="${REPO_ROOT}/tools/lint/rtl.f"
BUILD_DIR="${REPO_ROOT}/sim/build/integration"
RESULTS="${REPO_ROOT}/sim/results/integration"
PASS=0; FAIL=0

mkdir -p "${BUILD_DIR}" "${RESULTS}"

INTEG_TBS=(
    "tb/integration/npu_core_smoke_tb.sv:npu_core_smoke_tb"
    "tb/integration/npu_conv_path_tb.sv:npu_conv_path_tb"
    "tb/integration/npu_dma_conv_tb.sv:npu_dma_conv_tb"
    "tb/integration/npu_full_tb.sv:npu_full_tb"
)

for entry in "${INTEG_TBS[@]}"; do
    TB_FILE="${entry%%:*}"
    TB_TOP="${entry##*:}"
    TB_PATH="${REPO_ROOT}/${TB_FILE}"

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
echo "Integration results: ${PASS} passed, ${FAIL} failed"
exit "${FAIL}"