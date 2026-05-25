#!/usr/bin/env bash
set -euo pipefail

if [[ -z "${PDK_ROOT:-}" ]]; then
    echo "PDK_ROOT is not set"
    echo "Expected sky130A under: <PDK_ROOT>/sky130A"
    exit 1
fi

if [[ ! -d "${PDK_ROOT}/sky130A" ]]; then
    echo "Missing ${PDK_ROOT}/sky130A"
    echo "Install Sky130 PDK first, then rerun."
    exit 1
fi

export SKY130A="${PDK_ROOT}/sky130A"
export STD_CELL_LIB="sky130_fd_sc_hd"

echo "Sky130 environment configured"
echo "PDK_ROOT=${PDK_ROOT}"
echo "SKY130A=${SKY130A}"
echo "STD_CELL_LIB=${STD_CELL_LIB}"
