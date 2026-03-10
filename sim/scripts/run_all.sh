#!/usr/bin/env bash

# ============================================================================
# run_all.sh - Run a regression list (or all levels if no list given)
#   Usage: run_all.sh [regression-list]
# ============================================================================

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
REGLIST="${1:-}"

if [ -z "${REGLIST}" ]; then
    echo "=== Running all test levels ==="
    FAIL=0
    bash "${REPO_ROOT}/sim/scripts/run_unit.sh"        || FAIL=$((FAIL + $?))
    bash "${REPO_ROOT}/sim/scripts/run_block.sh"       || FAIL=$((FAIL + $?))
    bash "${REPO_ROOT}/sim/scripts/run_integration.sh" || FAIL=$((FAIL + $?))
    exit "${FAIL}"
fi

if [ ! -f "${REGLIST}" ]; then
    echo "ERROR: regression list not found: ${REGLIST}"
    exit 1
fi

echo "=== Running regression: ${REGLIST} ==="
PASS=0; FAIL=0; SKIP=0

while IFS= read -r line || [ -n "${line}" ]; do
    # Skip blank lines and comments
    line="${line%%#*}"
    line="$(echo "${line}" | xargs)"
    [ -z "${line}" ] && continue

    LEVEL="${line%%/*}"
    TB_ENTRY="${line}"

    case "${LEVEL}" in
        unit)        SCRIPT="${REPO_ROOT}/sim/scripts/run_unit.sh" ;;
        block)       SCRIPT="${REPO_ROOT}/sim/scripts/run_block.sh" ;;
        integration) SCRIPT="${REPO_ROOT}/sim/scripts/run_integration.sh" ;;
        *)
            echo "[SKIP] Unknown level: ${LEVEL}"
            SKIP=$((SKIP + 1))
            continue
            ;;
    esac

    bash "${SCRIPT}" && PASS=$((PASS + 1)) || FAIL=$((FAIL + 1))
done < "${REGLIST}"

echo ""
echo "Regression results: ${PASS} passed, ${FAIL} failed, ${SKIP} skipped"
exit "${FAIL}"
