#!/usr/bin/env bash
# Authoring slim-target gate: the authoring hello world
# (examples/embed_minimal/authoring.cpp, linked against x3d_cpp::authoring
# only) must not pull in any runtime subsystem.
set -euo pipefail
cd "$(dirname "$0")/.."
cmake --preset dev >/dev/null
cmake --build --preset dev --target x3d_authoring_link_contract
BIN=build/x3d_authoring_link_contract
# Symbol scan: no excluded-subsystem symbols may be linked in.
FORBIDDEN='SceneExtractor|X3DExecutionContext|parseDocument|parseFile|PhysicsWorld|ScriptSystem|MeshBuilder'
if nm -C "$BIN" | grep -E "$FORBIDDEN"; then
  echo "FAIL: authoring link contract pulled in excluded subsystem symbols"; exit 1
fi
echo "PASS: authoring link contract excludes runtime subsystem symbols"
