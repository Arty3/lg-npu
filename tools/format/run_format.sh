#!/usr/bin/env bash

# ============================================================================
# run_format.sh - Auto-format RTL and script files
#   Requires: verible-verilog-format (SV), shfmt (shell)
# ============================================================================

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"

if command -v verible-verilog-format &>/dev/null; then
    echo "Formatting SystemVerilog files..."
    find "${REPO_ROOT}/rtl" "${REPO_ROOT}/include" "${REPO_ROOT}/tb" \
        -name '*.sv' -o -name '*.svh' | while IFS= read -r f; do
        verible-verilog-format --inplace \
            --indentation_spaces 4 \
            --column_limit 100 \
            "${f}"
    done
    echo "  Done."
else
    echo "SKIP: verible-verilog-format not found (install Verible)"
fi

if command -v shfmt &>/dev/null; then
    echo "Formatting shell scripts..."
    find "${REPO_ROOT}/tools" "${REPO_ROOT}/sim/scripts" \
        -name '*.sh' -exec shfmt -w -i 4 -ci {} +
    echo "  Done."
else
    echo "SKIP: shfmt not found (go install mvdan.cc/sh/v3/cmd/shfmt@latest)"
fi

if command -v black &>/dev/null; then
    echo "Formatting Python files..."
    find "${REPO_ROOT}/model" "${REPO_ROOT}/tools" \
        -name '*.py' -exec black --quiet --line-length 100 {} +
    echo "  Done."
else
    echo "SKIP: black not found (pip install black)"
fi

echo "Format pass complete."