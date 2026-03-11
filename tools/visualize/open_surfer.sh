#!/usr/bin/env bash

# ============================================================================
# open_surfer.sh - Launch Surfer waveform viewer on simulation traces
#
# Usage:
#   bash tools/visualize/open_surfer.sh [path/to/file.vcd|.fst]
#
# If no argument is given, opens the most recent .vcd or .fst file
# found in sim/waves/.
# ============================================================================

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
WAVE_DIR="${REPO_ROOT}/sim/waves"

# Check surfer is installed
if ! command -v surfer &>/dev/null; then
    echo "ERROR: 'surfer' not found in PATH."
    echo ""
    echo "Install Surfer (https://surfer-project.org/):"
    echo ""
    echo "  cargo install --locked surfer"
    echo ""
    echo "Or download a pre-built binary from:"
    echo "  https://gitlab.com/surfer-project/surfer/-/releases"
    exit 1
fi

if [[ $# -ge 1 ]]; then
    WAVE_FILE="$1"
else
    # Find the most recently modified .vcd or .fst file
    WAVE_FILE=$(find "${WAVE_DIR}" -maxdepth 1 \( -name '*.vcd' -o -name '*.fst' \) \
        -printf '%T@ %p\n' 2>/dev/null | sort -rn | head -1 | cut -d' ' -f2-)

    if [[ -z "${WAVE_FILE}" ]]; then
        echo "No .vcd or .fst files found in ${WAVE_DIR}/"
        echo "Run a simulation first (e.g. 'make sim-unit') to generate traces."
        exit 1
    fi
fi

if [[ ! -f "${WAVE_FILE}" ]]; then
    echo "File not found: ${WAVE_FILE}"
    exit 1
fi

echo "Opening ${WAVE_FILE} in Surfer ..."

# Surfer may exit non-zero under WSLg due to X11 clipboard teardown errors.
# This is cosmetic — the viewer works fine — so we suppress the exit code
# and filter out the clipboard noise on stderr.
surfer "${WAVE_FILE}" 2> >(grep -v -E 'clipboard|Broken pipe|panicked|ExitFailure|RUST_BACKTRACE' >&2) || true
