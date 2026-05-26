#!/usr/bin/env bash
# install_pdk.sh - Install the sky130A PDK at the pinned commit via Volare.
#
# Volare caches the PDK under $PDK_ROOT (defaults to $HOME/.volare). Re-running
# this script is a no-op once the pinned commit is already present.
#
# Usage:
#   bash asic/scripts/install_pdk.sh
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=versions.env
source "${SCRIPT_DIR}/versions.env"

: "${PDK_ROOT:=${HOME}/.volare}"
export PDK_ROOT

if ! command -v volare >/dev/null 2>&1; then
    cat >&2 <<EOF
volare not found in PATH.
    pip install -r asic/scripts/requirements.txt
EOF
    exit 1
fi

if ! command -v gh >/dev/null 2>&1 || ! [[ -x "$(command -v gh)" ]]; then
    cat >&2 <<EOF
ERROR: GitHub CLI ('gh') is required by Volare but is missing or not executable.
       Diagnose:  which -a gh && ls -l "\$(which gh 2>/dev/null)"
       Install on Ubuntu/WSL:
         sudo apt-get install -y gh
       If 'gh' exists but is not executable:
         chmod +x "\$(which gh)"
EOF
    exit 1
fi

if [[ -d "${PDK_ROOT}/sky130/versions/${SKY130_PDK_COMMIT}" ]]; then
    echo "sky130 PDK already installed at commit ${SKY130_PDK_COMMIT}"
else
    echo "Installing sky130 PDK at commit ${SKY130_PDK_COMMIT}"
    volare enable --pdk sky130 "${SKY130_PDK_COMMIT}"
fi

if [[ ! -d "${PDK_ROOT}/sky130A" ]]; then
    echo "ERROR: ${PDK_ROOT}/sky130A missing after install" >&2
    exit 1
fi

echo ""
echo "sky130 PDK ready"
echo "  PDK_ROOT = ${PDK_ROOT}"
echo "  commit   = ${SKY130_PDK_COMMIT}"
