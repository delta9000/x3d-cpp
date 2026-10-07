#!/usr/bin/env bash
# Bounded independent-provider proof, not complete ISO SAI conformance.
# Normal builds never fetch or depend on the independent x3d-sai repository.
set -euo pipefail
if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
  echo "usage: $0 <provider-enabled-x3d-sai-source> [empty-work-dir]" >&2
  exit 2
fi
cpp="$(cd "$(dirname "$0")/.." && pwd)"
sai="$(cd "$1" && pwd)"
if [ ! -f "$sai/include/x3d/sai/experimental/provider.hpp" ]; then
  echo "x3d-sai checkout does not contain the provider contract" >&2
  exit 2
fi
if [ "$#" -eq 2 ]; then
  mkdir -p "$2"
  work="$(cd "$2" && pwd)"
  if [ -n "$(find "$work" -mindepth 1 -maxdepth 1 -print -quit)" ]; then
    echo "work directory must be empty: $work" >&2
    exit 2
  fi
else
  work="$(mktemp -d "${TMPDIR:-/tmp}/x3d-sai-provider.XXXXXX")"
fi
echo "Independent provider proof: $work"
jobs="${CMAKE_BUILD_PARALLEL_LEVEL:-2}"
shared="${X3D_CPP_SHARED_NODES:-ON}"
source="$work/source-build"
consumer="$cpp/tests/cmake/sai_provider"
cmake -S "$consumer" -B "$source" -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_LIBDIR=lib -DX3D_CPP_SOURCE_DIR="$cpp" \
  -DX3D_SAI_SOURCE_DIR="$sai" -DX3D_CPP_SHARED_NODES="$shared"
# The one native backend owns a real RuntimeSession in live mode.
cmake --build "$source" --target sai_provider_all --parallel "$jobs"

check_native_link() {
  local build="$1"
  local target="${2:-sai_provider_native}"
  local command
  command="$(ninja -C "$build" -t commands "$target" | tail -1)"
  printf '%s\n' "$command" > "$build/${target}-link-command.txt"
  if [[ "$command" != *"-o $target"* ]]; then
    echo "could not identify the independent native executable's link command" >&2
    exit 1
  fi
  if grep -E 'sai_experimental|sai_reference|sai_metadata' "$build/${target}-link-command.txt"; then
    echo "independent native provider links the SAI reference implementation" >&2
    exit 1
  fi
}
check_native_link "$source"
check_native_link "$source" sai_provider_native_runtime
ctest --test-dir "$source" --output-on-failure -R '^sai_provider_'

prefix="$work/original-prefix"
moved="$work/relocated-prefix"
cmake --install "$source/x3d-cpp" --prefix "$prefix"
cmake --install "$source/x3d-sai" --prefix "$prefix"
cmake --install "$source/adapter" --prefix "$prefix"
for header in x3d_cpp/x3d/sai_provider.hpp x3d/sai/experimental/provider.hpp; do
  test -f "$prefix/include/$header"
done
if grep -RIlF -e "$cpp" -e "$sai" -e "$source" "$prefix/lib/cmake"; then
  echo "installed CMake package leaks a source/build path" >&2
  exit 1
fi
mv "$prefix" "$moved"
cp -R "$consumer" "$work/consumer"
cp "$cpp/adapters/sai_provider/tests/native_runtime_test.cpp" "$work/consumer/native_runtime_test.cpp"
cmake -S "$work/consumer" -B "$work/installed-build" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DSAI_PROVIDER_INSTALLED=ON \
  -DCMAKE_PREFIX_PATH="$moved" -DCMAKE_FIND_USE_PACKAGE_REGISTRY=OFF \
  -DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=OFF
cmake --build "$work/installed-build" --target sai_provider_all --parallel "$jobs"
check_native_link "$work/installed-build"
check_native_link "$work/installed-build" sai_provider_native_runtime
ctest --test-dir "$work/installed-build" --output-on-failure -R '^sai_provider_'
echo "Unified provider passed: reference/native/parity in source and relocated installed consumers ($shared shared nodes)"
