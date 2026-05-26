#!/usr/bin/env bash
# install_sv2v.sh - Build and install sv2v from source.
#
# Mirrors the CI step in .github/workflows/ci.yml so local and CI use the
# same toolchain. Requires haskell-stack to be available in PATH.
#
# Usage:
#   bash asic/scripts/install_sv2v.sh                 # install into ~/.local/bin
#   PREFIX=/usr/local sudo bash asic/scripts/install_sv2v.sh
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=versions.env
source "${SCRIPT_DIR}/versions.env"

: "${PREFIX:=${HOME}/.local}"
: "${SV2V_SRC:=/tmp/sv2v}"

mkdir -p "${PREFIX}/bin"

if command -v sv2v >/dev/null 2>&1; then
    echo "sv2v already installed: $(sv2v --numeric-version)"
    exit 0
fi

if ! command -v stack >/dev/null 2>&1; then
    cat >&2 <<EOF
haskell-stack not found in PATH.
    Debian/Ubuntu : sudo apt-get install -y haskell-stack
    macOS         : brew install haskell-stack
EOF
    exit 1
fi

if [[ ! -d "${SV2V_SRC}/.git" ]]; then
    git clone --depth 1 --branch "${SV2V_REF}" https://github.com/zachjs/sv2v.git "${SV2V_SRC}"
fi

cd "${SV2V_SRC}"
stack --no-terminal install --local-bin-path "${PREFIX}/bin"

echo ""
echo "sv2v installed to ${PREFIX}/bin"
"${PREFIX}/bin/sv2v" --numeric-version
