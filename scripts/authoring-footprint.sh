#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
cmake -S . -B build-asset-import -G Ninja -DX3D_CPP_BUILD_ASSET_IMPORT=ON >/dev/null
cmake --build build-asset-import --target x3d_authoring_smoke
BIN=build-asset-import/examples/asset_import/x3d_authoring_smoke
# 1) Symbol scan: no excluded-subsystem symbols may be linked in.
FORBIDDEN='SceneExtractor|X3DExecutionContext|parseDocument|parseFile|PhysicsWorld|ScriptSystem|MeshBuilder'
if nm -C "$BIN" | grep -Eq "$FORBIDDEN"; then
  echo "FAIL: authoring smoke pulled in excluded subsystem symbols:"; nm -C "$BIN" | grep -E "$FORBIDDEN"; exit 1
fi
# 2) Size baseline (text section of the stripped binary).
SIZE=$(size -A "$BIN" | awk '/\.text/{print $2; exit}')

# If writing baseline, do it and exit
if [[ "${1:-}" == "--write-baseline" ]]; then
  mkdir -p examples/asset_import/footprint
  printf 'text_bytes\t%s\n' "$SIZE" > examples/asset_import/footprint/baseline.tsv
  echo "baseline written: $SIZE bytes"
  exit 0
fi

if [ ! -f examples/asset_import/footprint/baseline.tsv ]; then
  echo "FAIL: baseline.tsv does not exist. Run with --write-baseline to create it."
  exit 1
fi

BASE=$(awk -F'\t' '/^text_bytes/{print $2}' examples/asset_import/footprint/baseline.tsv)
echo "authoring smoke .text = $SIZE (baseline $BASE)"
# Allow 10% growth
if (( SIZE > BASE * 110 / 100 )); then echo "FAIL: .text grew >10% over baseline"; exit 1; fi
echo "PASS"
