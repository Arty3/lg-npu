#!/usr/bin/env bash
# setup_sky130_env.sh - Validate / export Sky130 PDK environment.
#
# If PDK_ROOT is unset, fall back to the Volare default ($HOME/.volare),
# which is the canonical install location used by OpenLane2.
#
# Usage: source asic/scripts/setup_sky130_env.sh   (export vars to caller)
#        bash   asic/scripts/setup_sky130_env.sh   (validate only)
set -euo pipefail

: "${PDK_ROOT:=${HOME}/.volare}"

if [[ ! -d "${PDK_ROOT}/sky130A" ]]; then
    cat >&2 <<EOF
Sky130 PDK not found.
    Looked for: ${PDK_ROOT}/sky130A

Bootstrap the full ASIC toolchain (OpenLane2 + PDK + macros):
    bash asic/scripts/bootstrap.sh

Or install just the PDK at the pinned commit:
    pip install -r asic/scripts/requirements.txt
    bash asic/scripts/install_pdk.sh

Or set PDK_ROOT explicitly to an existing install:
    export PDK_ROOT=/path/to/pdk
EOF
    exit 1
fi

export PDK_ROOT
export PDK="sky130A"
export STD_CELL_LIBRARY="sky130_fd_sc_hd"
export SKY130A="${PDK_ROOT}/sky130A"

echo "Sky130 environment configured"
echo "  PDK_ROOT          = ${PDK_ROOT}"
echo "  PDK               = ${PDK}"
echo "  STD_CELL_LIBRARY  = ${STD_CELL_LIBRARY}"
