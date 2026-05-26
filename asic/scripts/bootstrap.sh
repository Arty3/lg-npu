#!/usr/bin/env bash
# bootstrap.sh - One-shot setup for the Sky130 ASIC flow.
#
# Runs, in order:
#   1. pip install -r asic/scripts/requirements.txt   (openlane + volare)
#   2. install_pdk.sh                                 (volare enable sky130)
#   3. stage_macros.sh                                (copy OpenRAM macros)
#   4. setup_sky130_env.sh                            (validate env)
#
# Idempotent: every step is a no-op if its output is already in place.
#
# Usage:
#   bash asic/scripts/bootstrap.sh
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if ! command -v python3 >/dev/null 2>&1; then
    echo "ERROR: python3 not found in PATH" >&2
    exit 1
fi

if ! python3 -c 'import ssl' >/dev/null 2>&1; then
    cat >&2 <<'EOF'
ERROR: this python3 was built without the ssl module, so pip cannot reach PyPI.
       Diagnose:  python3 -c 'import ssl; print(ssl.OPENSSL_VERSION)'
       Fix on Ubuntu/WSL (use the distro python):
         sudo apt-get install -y python3 python3-venv python3-pip
         deactivate 2>/dev/null || true
         rm -rf .venv
         /usr/bin/python3 -m venv .venv
         source .venv/bin/activate
       Or, if using pyenv/source builds, install libssl-dev first and rebuild:
         sudo apt-get install -y libssl-dev libffi-dev zlib1g-dev
EOF
    exit 1
fi

echo "[1/4] Installing Python packages (openlane, volare)"
if ! python3 -m pip --version >/dev/null 2>&1; then
    echo "      pip missing - bootstrapping via ensurepip"
    if ! python3 -m ensurepip --upgrade >/dev/null 2>&1; then
        cat >&2 <<'EOF'
ERROR: python3 has neither pip nor ensurepip available.
       On Debian/Ubuntu install one of:
         sudo apt-get install -y python3-pip python3-venv
       Then recreate the venv:
         rm -rf .venv && python3 -m venv .venv && source .venv/bin/activate
EOF
        exit 1
    fi
fi
python3 -m pip install --upgrade pip
python3 -m pip install -r "${SCRIPT_DIR}/requirements.txt"

echo "[2/4] Installing sky130 PDK via Volare"
bash "${SCRIPT_DIR}/install_pdk.sh"

echo "[3/4] Staging OpenRAM SRAM macros"
bash "${SCRIPT_DIR}/stage_macros.sh"

echo "[4/4] Validating environment"
bash "${SCRIPT_DIR}/setup_sky130_env.sh"

echo ""
echo "Bootstrap complete. Next:"
echo "  source asic/scripts/setup_sky130_env.sh"
echo "  make asic-sky130-flow"
