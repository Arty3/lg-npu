#!/usr/bin/env bash
# run_openlane.sh - Drive the OpenLane2 flow end-to-end for npu_shell.
#
# Prerequisites:
#   - Volare-installed sky130A under $PDK_ROOT (or $HOME/.volare).
#   - openlane v2 installed (e.g. `pip install openlane`).
#   - Synlig-capable OpenLane/Yosys image (default --dockerized path).
#
# Env overrides:
#   PDK_ROOT     defaults to $HOME/.volare
#   RUN_TAG      defaults to a timestamp
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OL_DIR="${REPO_ROOT}/asic/openlane2"
CONFIG="${OL_DIR}/config.json"

# shellcheck source=setup_sky130_env.sh
source "${REPO_ROOT}/asic/scripts/setup_sky130_env.sh"

bash "${REPO_ROOT}/asic/scripts/prep_design.sh"

# OpenLane2 needs yosys (and friends) on PATH. Source the OSS CAD Suite
# environment if present, since make's non-login shell may not have it.
OSS_CAD_ENV_CANDIDATES=(
    "${OSS_CAD_SUITE_ROOT:-}/environment"
    "${HOME}/opt/oss-cad-suite/environment"
    "/opt/oss-cad-suite/environment"
)
for env_file in "${OSS_CAD_ENV_CANDIDATES[@]}"; do
    if [[ -n "${env_file}" && -f "${env_file}" ]]; then
        # shellcheck disable=SC1090
        source "${env_file}"
        echo "run_openlane: sourced ${env_file}"
        break
    fi
done

# Activate repo-local .venv if it exists. Make's non-login shell otherwise
# doesn't inherit the user's activated venv, so the pip-installed openlane
# CLI isn't visible. Force re-activation so .venv/bin lands at the front of
# PATH even if OSS CAD Suite was sourced after a previous activation.
if [[ -f "${REPO_ROOT}/.venv/bin/activate" ]]; then
    # shellcheck disable=SC1091
    source "${REPO_ROOT}/.venv/bin/activate"
    echo "run_openlane: activated venv at ${REPO_ROOT}/.venv"
fi

# Probe known locations. Prefer .venv (OpenLane2) over any system 'openlane'
# binary that OSS CAD Suite may ship - OSS CAD ships the legacy v1 CLI.
OPENLANE_BIN=""
for cand in \
    "${REPO_ROOT}/.venv/bin/openlane" \
    "$(command -v openlane 2>/dev/null || true)" \
    "${HOME}/.local/bin/openlane"
do
    if [[ -n "${cand}" && -x "${cand}" ]]; then
        OPENLANE_BIN="${cand}"
        break
    fi
done

if [[ -z "${OPENLANE_BIN}" ]]; then
    echo "run_openlane: 'openlane' CLI not found in PATH or .venv" >&2
    echo "    install with: pip install openlane (inside .venv)" >&2
    exit 1
fi

echo "run_openlane: using openlane at ${OPENLANE_BIN}"

RUN_TAG="${RUN_TAG:-$(date +%Y%m%d_%H%M%S)}"

echo "run_openlane: starting OpenLane2 flow (tag=${RUN_TAG})"
cd "${OL_DIR}"

# By default we use --dockerized so OpenLane pulls a container image with a
# Python-enabled yosys (the bare `pip install openlane` does not ship one,
# and OSS CAD Suite's yosys lacks `--enable-python`). Set
# OPENLANE_NO_DOCKER=1 to invoke the host yosys instead - only works if a
# python-bindings yosys is on PATH.
OPENLANE_DOCKER_FLAGS=()
if [[ "${OPENLANE_NO_DOCKER:-0}" != "1" ]]; then
    OPENLANE_DOCKER_FLAGS+=("--dockerized")
fi

"${OPENLANE_BIN}" \
    "${OPENLANE_DOCKER_FLAGS[@]}" \
    --pdk-root "${PDK_ROOT}" \
    --pdk "${PDK}" \
    --run-tag "${RUN_TAG}" \
    "${CONFIG}"

echo "run_openlane: flow complete -> ${OL_DIR}/runs/${RUN_TAG}"
