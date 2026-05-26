#!/usr/bin/env bash
# prep_design.sh - Stage SystemVerilog RTL for OpenLane2 Synlig frontend.
#
# Inputs : tools/lint/rtl.f (filelist used by sim & lint)
# Output : asic/openlane2/src/rtl/*.sv
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
FILELIST="${REPO_ROOT}/tools/lint/rtl.f"
OUT_DIR="${REPO_ROOT}/asic/openlane2/src"
STAGE_DIR="${OUT_DIR}/rtl"
INCLUDE_STAGE_DIR="${OUT_DIR}/include"

mkdir -p "${STAGE_DIR}"
mkdir -p "${INCLUDE_STAGE_DIR}"

# Collect .sv files from the filelist in order, skipping comments and -I lines.
# Also exclude RTL-side macro blackbox declarations: OpenLane provides its own
# blackbox stubs from the macro .lib/.lef, so including ours produces MODDUP.
# Exclude formal/property bind files from ASIC synthesis inputs.
mapfile -t SV_FILES < <(awk '
    /^[[:space:]]*\/\// { next }
    /^[[:space:]]*-I/   { next }
    /sky130_sram_blackboxes\.sv/ { next }
    /(^|\/)formal\// { next }
    /_props\.sv$/ { next }
    /bindings\.sv$/ { next }
    /\.sv[[:space:]]*$/ { print }
' "${FILELIST}")

if [[ ${#SV_FILES[@]} -eq 0 ]]; then
    echo "prep_design: no SystemVerilog sources found in ${FILELIST}" >&2
    exit 1
fi

find "${STAGE_DIR}" -maxdepth 1 -type f -name '*.sv' -delete

rm -rf "${INCLUDE_STAGE_DIR}/pkg" "${INCLUDE_STAGE_DIR}/defines"
cp -r "${REPO_ROOT}/include/pkg" "${INCLUDE_STAGE_DIR}/pkg"
cp -r "${REPO_ROOT}/include/defines" "${INCLUDE_STAGE_DIR}/defines"

idx=0
for rel in "${SV_FILES[@]}"; do
    src="${REPO_ROOT}/${rel}"
    if [[ ! -f "${src}" ]]; then
        echo "prep_design: missing source listed in rtl.f: ${rel}" >&2
        exit 1
    fi

    idx=$((idx + 1))
    staged="${STAGE_DIR}/$(printf '%04d' "${idx}")__$(basename "${rel}")"
    cp "${src}" "${staged}"
done

echo "prep_design: staged ${idx} SystemVerilog files in ${STAGE_DIR}"
echo "prep_design: staged include trees in ${INCLUDE_STAGE_DIR}"
