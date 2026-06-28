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
# pipeline headlessly under Xvfb + mesa software GL (llvmpipe) — mirroring the
# sibling ../x3d-render validation path (GLFW offscreen + glReadPixels). It is the
# single source of truth shared by `mise run validate-examples` and CI.
set -euo pipefail
cd "$(dirname "$0")/.."

SCENE=examples/cpu_raster/assets/raster_smoke.x3d
SHOT="$(mktemp -d)/poc_smoke.ppm"

echo "== build cpu_raster (X3D_CPP_BUILD_CPURASTER=ON) =="
cmake -S . -B build-cpuraster -G Ninja -DX3D_CPP_BUILD_CPURASTER=ON -DX3D_CPP_BUILD_STB=ON >/dev/null
cmake --build build-cpuraster --target x3d_cpu_raster
CPU=build-cpuraster/examples/cpu_raster/x3d_cpu_raster

echo "== build poc_renderer (X3D_CPP_BUILD_POC=ON; FetchContent GLFW) =="
cmake -S . -B build-poc -G Ninja -DX3D_CPP_BUILD_POC=ON >/dev/null
cmake --build build-poc --target x3d_poc_renderer
POC=build-poc/examples/poc_renderer/x3d_poc_renderer

echo "== cpu_raster --headless (no GL) =="
"$CPU" "$SCENE" --headless

echo "== poc --headless (no GL) =="
"$POC" --headless "$SCENE"

echo "== poc --screenshot under Xvfb + mesa software GL (the real GL pipeline) =="
# Force mesa/llvmpipe so this is reproducible on a GPU-less box: NVIDIA's GLX
# ignores LIBGL_ALWAYS_SOFTWARE, so we also steer GLVND to the mesa vendor.
xvfb-run -a env \
  LIBGL_ALWAYS_SOFTWARE=1 GALLIUM_DRIVER=llvmpipe __GLX_VENDOR_LIBRARY_NAME=mesa \
  "$POC" --screenshot "$SHOT" "$SCENE"

if [ ! -s "$SHOT" ]; then
  echo "FAIL: GL screenshot missing or empty: $SHOT" >&2
  exit 1
fi
echo "OK: GL pipeline produced a $(stat -c%s "$SHOT")-byte frame ($SHOT)"
echo "== examples validated: cpu_raster + poc_renderer compile and run headless =="
