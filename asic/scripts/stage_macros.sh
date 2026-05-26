#!/usr/bin/env bash
# stage_macros.sh - Stage the sky130 OpenRAM SRAM macros into
# asic/openlane2/macros/ so OpenLane2 can find their LEF / LIB / GDS / V.
#
# Re-running is a no-op once every target file is already present.
#
# Usage:
#   bash asic/scripts/stage_macros.sh
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=versions.env
source "${SCRIPT_DIR}/versions.env"

REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
MACROS_DST="${REPO_ROOT}/asic/openlane2/macros"
: "${MACROS_SRC:=/tmp/sky130_sram_macros}"

mkdir -p "${MACROS_DST}"

if [[ ! -d "${MACROS_SRC}/.git" ]]; then
    echo "Cloning sky130_sram_macros (${SRAM_MACROS_REF}) -> ${MACROS_SRC}"
    git clone --depth 1 --branch "${SRAM_MACROS_REF}" \
        https://github.com/efabless/sky130_sram_macros.git "${MACROS_SRC}"
fi

missing=0
for macro in ${SRAM_MACROS}; do
    src_dir="${MACROS_SRC}/${macro}"
    if [[ ! -d "${src_dir}" ]]; then
        echo "ERROR: source dir not found: ${src_dir}" >&2
        missing=1
        continue
    fi

    for ext in lef gds v; do
        src="${src_dir}/${macro}.${ext}"
        dst="${MACROS_DST}/${macro}.${ext}"
        if [[ ! -f "${src}" ]]; then
            echo "ERROR: missing ${src}" >&2
            missing=1
            continue
        fi
        cp -f "${src}" "${dst}"
    done

    lib_src="${src_dir}/${macro}_${SRAM_LIB_CORNER}.lib"
    lib_dst="${MACROS_DST}/${macro}_${SRAM_LIB_CORNER}.lib"
    if [[ ! -f "${lib_src}" ]]; then
        echo "ERROR: missing ${lib_src}" >&2
        missing=1
    else
        cp -f "${lib_src}" "${lib_dst}"
    fi
done

if [[ "${missing}" -ne 0 ]]; then
    exit 1
fi

echo ""
echo "Staged macros under ${MACROS_DST}:"
ls -1 "${MACROS_DST}" | grep -E '\.(lef|gds|lib|v)$'
