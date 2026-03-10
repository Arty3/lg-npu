#!/usr/bin/env bash
# ============================================================================
# run_verilator.sh - Lint all RTL with Verilator
# ============================================================================
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
FILELIST="${REPO_ROOT}/tools/lint/rtl.f"

verilator --sv --lint-only \
    --top-module npu_shell \
    -Wall \
    -Wno-MULTITOP \
    -f "${FILELIST}"
