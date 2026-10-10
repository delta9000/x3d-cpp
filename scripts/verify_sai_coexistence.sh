#!/usr/bin/env bash
# Opt-in source + relocated installed-package composition check. Requires a
# separate x3d-sai checkout; normal x3d-cpp builds never fetch or depend on it.
set -euo pipefail

if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
  echo "usage: $0 <x3d-sai-source> [empty-work-dir]" >&2
  exit 2
fi
repo_root="$(cd "$(dirname "$0")/.." && pwd)"
sai_root="$(cd "$1" && pwd)"
if [ ! -f "$sai_root/CMakeLists.txt" ]; then
  echo "missing x3d-sai CMakeLists.txt: $sai_root" >&2
  exit 2
fi
if [ "$#" -eq 2 ]; then
  mkdir -p "$2"
  work_dir="$(cd "$2" && pwd)"
  if [ -n "$(find "$work_dir" -mindepth 1 -maxdepth 1 -print -quit)" ]; then
    echo "work directory must be empty: $work_dir" >&2
    exit 2
  fi
else
  work_dir="$(mktemp -d "${TMPDIR:-/tmp}/x3d-sai-coexistence.XXXXXX")"
fi
echo "SAI coexistence work directory: $work_dir"
generator="${CMAKE_GENERATOR:-Ninja}"
jobs="${CMAKE_BUILD_PARALLEL_LEVEL:-2}"
shared="${X3D_CPP_SHARED_NODES:-ON}"
consumer="$repo_root/tests/cmake/sai_coexistence"
source_build="$work_dir/source-build"
prefix="$work_dir/prefix"
moved="$work_dir/relocated-prefix"

cmake -S "$consumer" -B "$source_build" -G "$generator" \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_LIBDIR=lib \
  -DX3D_CPP_SOURCE_DIR="$repo_root" -DX3D_SAI_SOURCE_DIR="$sai_root" \
  -DX3D_CPP_SHARED_NODES="$shared"
cmake --build "$source_build" --config Release --target sai_coexistence_all --parallel "$jobs"
ctest --test-dir "$source_build" -C Release --output-on-failure -R '^sai_coexistence_'

# Install both packages into one prefix, then remove the original location.
# Reconfigure a copied consumer so it cannot reach either source via relatives.
cmake --install "$source_build/x3d-cpp" --config Release --prefix "$prefix"
cmake --install "$source_build/x3d-sai" --config Release --prefix "$prefix"
if [ ! -f "$prefix/include/x3d_cpp/x3d/sdk.hpp" ] || \
   [ ! -f "$prefix/include/x3d/sai/experimental/kernel.hpp" ]; then
  echo "one of the public package entry points was not installed" >&2
  exit 1
fi
if [ -e "$prefix/include/x3d/core" ] || [ -e "$prefix/include/x3d/nodes" ] || \
   [ -e "$prefix/include/x3d_sai" ]; then
  echo "SAI private generated headers escaped into the install prefix" >&2
  exit 1
fi
if grep -RIlF -e "$repo_root" -e "$sai_root" -e "$source_build" \
    "$prefix/lib/cmake"; then
  echo "installed CMake exports retain source/build paths" >&2
  exit 1
fi
mv "$prefix" "$moved"
cp -R "$consumer" "$work_dir/consumer"
cmake -S "$work_dir/consumer" -B "$work_dir/installed-build" -G "$generator" \
  -DCMAKE_BUILD_TYPE=Release -DSAI_COEXISTENCE_INSTALLED=ON \
  -DCMAKE_PREFIX_PATH="$moved" -DCMAKE_FIND_USE_PACKAGE_REGISTRY=OFF \
  -DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=OFF
cmake --build "$work_dir/installed-build" --config Release --target sai_coexistence_all --parallel "$jobs"
ctest --test-dir "$work_dir/installed-build" -C Release --output-on-failure -R '^sai_coexistence_'
echo "SAI coexistence passed: four source and four relocated installed consumers ($shared shared nodes)"
