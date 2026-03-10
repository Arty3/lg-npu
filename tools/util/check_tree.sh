#!/usr/bin/env bash

# ============================================================================
# check_tree.sh - Validate that expected directories and key files exist
# ============================================================================

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
ERR=0

check_dir() {
    if [ ! -d "${REPO_ROOT}/$1" ]; then
        echo "MISSING dir:  $1"
        ERR=$((ERR + 1))
    fi
}

check_file() {
    if [ ! -f "${REPO_ROOT}/$1" ]; then
        echo "MISSING file: $1"
        ERR=$((ERR + 1))
    fi
}

echo "Checking project tree..."

# Top-level
check_file "Makefile"
check_file "README.md"
check_file ".editorconfig"
check_file "tools/lint/rtl.f"

# Packages
for pkg in npu_types_pkg npu_cfg_pkg npu_cmd_pkg npu_addrmap_pkg \
           npu_tensor_pkg npu_perf_pkg; do
    check_file "include/pkg/${pkg}.sv"
done
check_file "include/defines/build_defs.svh"

# RTL directories
for d in rtl/common rtl/control rtl/core rtl/ifaces rtl/memory \
         rtl/backends/conv rtl/platform; do
    check_dir "${d}"
done

# Key RTL files
check_file "rtl/core/npu_shell.sv"
check_file "rtl/core/npu_core.sv"
check_file "rtl/backends/conv/conv_backend.sv"

# Testbench directories
for d in tb/unit tb/block tb/integration tb/perf tb/models \
         tb/common tb/vectors; do
    check_dir "${d}"
done

# Docs
for f in docs/arch/overview.md docs/arch/conv_dataflow.md; do
    check_file "${f}"
done

# Sim infrastructure
check_dir  "sim/scripts"
check_dir  "sim/regressions"
check_file "sim/scripts/run_unit.sh"
check_file "sim/scripts/run_block.sh"
check_file "sim/scripts/run_integration.sh"
check_file "sim/scripts/run_all.sh"

# Tools
check_file "tools/lint/run_verilator.sh"
check_file "tools/format/run_format.sh"
check_file "tools/gen/gen_addrmap.py"
check_file "tools/gen/gen_headers.py"

if [ "${ERR}" -eq 0 ]; then
    echo "All checks passed."
else
    echo "${ERR} issue(s) found."
fi
exit "${ERR}"