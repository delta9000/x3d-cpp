#!/usr/bin/env bash
# Fetch the Web3D X3D Conformance (NIST) example set for the poc-renderer sweep
# (scripts/poc_conformance_sweep.py). Downloads the official bundle from web3d.org
# and extracts the .x3d files (+ textures with --textures) into .x3d-corpus/x3d-code/
# (gitignored), matching the corpus path convention used by fetch_corpus.sh.
#
# Files are open-source under the Web3D Consortium BSD-style license; we download,
# never redistribute. ~366 MB download; the .x3d files alone are a few MB.
#
# Usage:  scripts/fetch_conformance_nist.sh [--textures]
set -euo pipefail
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CORPUS_DIR="${X3D_CORPUS_DIR:-${REPO_ROOT}/.x3d-corpus}"
DEST="${CORPUS_DIR}/x3d-code"
ZIP_URL="https://www.web3d.org/x3d/content/examples/X3dExamplesConformanceNist.zip"
WANT_TEXTURES="${1:-}"

command -v curl  >/dev/null || { echo "ERROR: curl not found"  >&2; exit 2; }
command -v unzip >/dev/null || { echo "ERROR: unzip not found" >&2; exit 2; }

TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
echo "downloading $ZIP_URL ..."
curl -fSL -o "$TMP/conf.zip" "$ZIP_URL"
mkdir -p "$DEST"
echo "extracting .x3d ..."
unzip -o -q "$TMP/conf.zip" "*.x3d" -d "$DEST"
if [ "$WANT_TEXTURES" = "--textures" ]; then
  echo "extracting textures (.png/.jpg) for the GL pass ..."
  unzip -o -q "$TMP/conf.zip" "*.png" "*.jpg" -d "$DEST"
fi
ROOT="$DEST/www.web3d.org/x3d/content/examples/ConformanceNist"
echo "done: $(find "$ROOT" -name '*.x3d' | wc -l) .x3d files under"
echo "  $ROOT"
