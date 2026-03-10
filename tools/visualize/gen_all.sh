#!/usr/bin/env bash
# ============================================================================
# Generate all architecture diagrams into docs/diagrams/ using
# sv2v (SV->Verilog) + Yosys (elaboration) + netlistsvg (rendering).
#
# Prerequisites: sv2v, yosys, netlistsvg (npm install -g netlistsvg)
# ============================================================================
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT_DIR="${REPO_ROOT}/docs/diagrams"
VIZ_DIR="${REPO_ROOT}/tools/visualize"

mkdir -p "${OUT_DIR}/components"

echo "=== Generating RTL visualisations via sv2v + Yosys + netlistsvg ==="
echo ""

echo "--- Top-level view (npu_shell, 1 level deep) ---"
python3 "${VIZ_DIR}/gen_top_view.py" --out-dir "${OUT_DIR}"
echo ""

echo "--- Per-component views ---"
python3 "${VIZ_DIR}/gen_component_views.py" --out-dir "${OUT_DIR}/components"
echo ""

echo "All SVG diagrams written to ${OUT_DIR}/"
