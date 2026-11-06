#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
cmake -S . -B build-asset-import -G Ninja -DX3D_CPP_BUILD_ASSET_IMPORT=ON >/dev/null
cmake --build build-asset-import --target x3d_authoring_smoke
BIN=build-asset-import/examples/asset_import/x3d_authoring_smoke
# 1) Symbol scan: no excluded-subsystem symbols may be linked in.
FORBIDDEN='SceneExtractor|X3DExecutionContext|parseDocument|parseFile|PhysicsWorld|ScriptSystem|MeshBuilder'
if nm -C "$BIN" | grep -E "$FORBIDDEN"; then
  echo "FAIL: authoring smoke pulled in excluded subsystem symbols"; exit 1
fi
echo "PASS: authoring smoke excludes runtime subsystem symbols"
