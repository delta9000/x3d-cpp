#!/usr/bin/env bash
# Validate the out-of-SDK renderer CONSUMERS that `mise run ci` does NOT build.
#
# `mise run ci` compiles the SDK + tests only. The two example renderers —
# examples/cpu_raster/ (headless CPU raster) and examples/poc_renderer/ (OpenGL) —
# are OFF by default and built only via `mise run cpuraster` / `mise run poc`, so
# an SDK-wide refactor (e.g. ADR-0039's x3d::core / x3d::nodes namespace move)
# silently breaks them and nothing notices until someone runs an example by hand.
#
# This gate builds BOTH, runs their --headless probes, and exercises the real GL
# pipeline headlessly under Xvfb — requesting mesa software GL (llvmpipe) so the
# run is reproducible on a GPU-less box — mirroring the sibling ../x3d-render
# validation path (GLFW offscreen + glReadPixels). It captures and checks the
# renderer's own GL_VERSION/GL_RENDERER banner rather than assuming the software
# request was honored, since it is known to be ignored on some GLX driver paths.
# It is the single source of truth shared by `mise run validate-examples` and CI.
set -euo pipefail
cd "$(dirname "$0")/.."

SCENE=examples/cpu_raster/assets/raster_smoke.x3d
SHOT="$(mktemp -d)/poc_smoke.ppm"

echo "== build cpu_raster (X3D_CPP_BUILD_CPURASTER=ON) =="
cmake -S . -B build-cpuraster -G Ninja -DX3D_CPP_BUILD_CPURASTER=ON -DX3D_CPP_BUILD_STB=ON >/dev/null
cmake --build build-cpuraster --target x3d_cpu_raster
CPU=build-cpuraster/examples/cpu_raster/x3d_cpu_raster

echo "== build x3d2svg (X3D_CPP_BUILD_X3D2SVG=ON; façade-only, no deps) =="
cmake -S . -B build-x3d2svg -G Ninja -DX3D_CPP_BUILD_X3D2SVG=ON >/dev/null
cmake --build build-x3d2svg --target x3d_x3d2svg x3d_x3d2svg_project
SVG=build-x3d2svg/examples/x3d2svg/x3d_x3d2svg

echo "== build poc_renderer (X3D_CPP_BUILD_POC=ON; FetchContent GLFW) =="
cmake -S . -B build-poc -G Ninja -DX3D_CPP_BUILD_POC=ON >/dev/null
cmake --build build-poc --target x3d_poc_renderer
POC=build-poc/examples/poc_renderer/x3d_poc_renderer

echo "== cpu_raster --headless (no GL) =="
"$CPU" "$SCENE" --headless

echo "== x3d2svg --headless (façade-only extract + project) =="
"$SVG" examples/x3d2svg/assets/smoke.x3d --headless
ctest --test-dir build-x3d2svg -R x3d_x3d2svg --output-on-failure

echo "== poc --headless (no GL) =="
"$POC" --headless "$SCENE"

echo "== poc --screenshot under Xvfb + mesa software GL (the real GL pipeline) =="
# We ask for mesa/llvmpipe so this is reproducible on a GPU-less box: NVIDIA's
# GLX is known to ignore LIBGL_ALWAYS_SOFTWARE on some setups, so we also steer
# GLVND to the mesa vendor. That is a request, not a guarantee -- so rather than
# asserting "software Mesa" happened, capture the renderer's own GL_VERSION/
# GL_RENDERER stderr banner (main.cpp logs it right after context creation) and
# check what actually rendered.
GL_LOG="$(mktemp -d)/poc_gl.log"
# Pipe through tee rather than a plain `2>file` redirect: xvfb-run backgrounds Xvfb
# and execs the wrapped command through an extra process layer (env), and on at
# least one real CI runner that chain did not reliably propagate a bare stderr
# redirect all the way through to the file even though the same command's output
# still reached the terminal -- a pipe is the standard, well-tested mechanism and
# also gives us the pipefail exit code from $POC itself, not from tee.
xvfb-run -a env \
  LIBGL_ALWAYS_SOFTWARE=1 GALLIUM_DRIVER=llvmpipe __GLX_VENDOR_LIBRARY_NAME=mesa \
  "$POC" --screenshot "$SHOT" "$SCENE" 2>&1 | tee "$GL_LOG" >&2

if [ ! -s "$SHOT" ]; then
  echo "FAIL: GL screenshot missing or empty: $SHOT" >&2
  exit 1
fi

GL_RENDERER_LINE="$(grep -m1 '^\[poc\] GL_RENDERER' "$GL_LOG" || true)"
GL_VERSION_LINE="$(grep -m1 '^\[poc\] GL_VERSION' "$GL_LOG" || true)"
if [ -z "$GL_RENDERER_LINE" ]; then
  echo "FAIL: poc did not log a [poc] GL_RENDERER line to stderr ($GL_LOG) -- cannot verify what actually rendered" >&2
  exit 1
fi

if echo "$GL_RENDERER_LINE" | grep -qiE 'llvmpipe|softpipe|swrast|software rasterizer'; then
  echo "OK: GL pipeline produced a $(stat -c%s "$SHOT")-byte frame on software Mesa (confirmed -- $GL_RENDERER_LINE)"
else
  echo "NOTE: GL pipeline produced a $(stat -c%s "$SHOT")-byte frame, but it did NOT render on software Mesa/llvmpipe -- LIBGL_ALWAYS_SOFTWARE/GLVND overrides were evidently not honored on this box's GLX path."
  echo "NOTE: actual renderer -- $GL_VERSION_LINE / $GL_RENDERER_LINE"
  echo "NOTE: the real GL pipeline was still exercised end-to-end on real hardware; what is NOT proven here is the GPU-less/software-Mesa reproducibility this gate also aims for."
fi

echo "== build asset_import (cgltf default, no assimp) =="
cmake -S . -B build-asset-import -G Ninja -DX3D_CPP_BUILD_ASSET_IMPORT=ON -DX3D_CPP_BUILD_STB=ON -DX3D_CPP_BUILD_CGLTF=ON -DX3D_CPP_BUILD_ASSIMP=OFF >/dev/null
cmake --build build-asset-import --target x3d_asset_import x3d_assetimport_cgltf

TMP_DIR="$(mktemp -d)"
AI=build-asset-import/examples/asset_import/x3d_asset_import

echo "== run asset_import (fixture) =="
"$AI" fixture:cube -o "$TMP_DIR/cube.x3d" --verify --stats

if [ ! -s "$TMP_DIR/cube.x3d" ]; then
  echo "FAIL: cube.x3d missing or empty" >&2
  exit 1
fi

echo "== run asset_import (cgltf glTF, default lightweight path) =="
ctest --test-dir build-asset-import -R x3d_assetimport_cgltf --output-on-failure
"$AI" examples/asset_import/assets/fixtures/twobox.glb -o "$TMP_DIR/twobox.x3d" --assets-dir "$TMP_DIR" --verify --stats
if [ ! -s "$TMP_DIR/twobox.x3d" ]; then
  echo "FAIL: twobox.x3d missing or empty" >&2
  exit 1
fi

echo "== run authoring footprint gate =="
bash scripts/authoring-footprint.sh

# Guard the assimp path
echo "== probe assimp availability =="
HAS_ASSIMP=0
if pkg-config --exists assimp; then
  HAS_ASSIMP=1
fi

if [ "$HAS_ASSIMP" -eq 1 ]; then
  echo "assimp available, building with cgltf + assimp..."
  cmake -S . -B build-asset-import -G Ninja -DX3D_CPP_BUILD_ASSET_IMPORT=ON -DX3D_CPP_BUILD_STB=ON -DX3D_CPP_BUILD_CGLTF=ON -DX3D_CPP_BUILD_ASSIMP=ON >/dev/null
  cmake --build build-asset-import --target x3d_asset_import x3d_assetimport_backend_swap

  echo "== run asset_import (assimp) =="
  "$AI" examples/asset_import/assets/fixtures/tri.obj -o "$TMP_DIR/tri.x3d" --verify --stats
  if [ ! -s "$TMP_DIR/tri.x3d" ]; then
    echo "FAIL: tri.x3d missing or empty" >&2
    exit 1
  fi

  echo "== run backend swap-test (cgltf vs assimp — ImportSource seam genericity) =="
  ctest --test-dir build-asset-import -R x3d_assetimport_backend_swap --output-on-failure
else
  echo "assimp not available; skipping assimp job"
fi

echo "== examples validated: cpu_raster + x3d2svg + poc_renderer + asset_import compile and run headless =="

