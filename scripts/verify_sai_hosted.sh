#!/usr/bin/env bash
# Opt-in hosted runtime proof; normal x3d-cpp builds never discover x3d-sai.
set -euo pipefail
if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
  echo "usage: $0 <hosted-enabled-x3d-sai-source> [empty-work-dir]" >&2
  exit 2
fi
cpp="$(cd "$(dirname "$0")/.." && pwd)"
sai="$(cd "$1" && pwd)"
if [ ! -f "$sai/include/x3d/sai/experimental/hosted.hpp" ]; then
  echo "x3d-sai checkout does not contain the hosted contract" >&2
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
  work="$(mktemp -d "${TMPDIR:-/tmp}/x3d-sai-hosted.XXXXXX")"
fi
jobs="${CMAKE_BUILD_PARALLEL_LEVEL:-2}"
if ! [[ "$jobs" =~ ^[1-9][0-9]*$ ]]; then
  echo "CMAKE_BUILD_PARALLEL_LEVEL must be a positive integer" >&2
  exit 2
fi
# Large generated-node builds must not accidentally overcommit the host.
if [ "$jobs" -gt 3 ]; then jobs=3; fi
shared="${X3D_CPP_SHARED_NODES:-OFF}"
case "$shared" in ON|OFF) ;; *) echo "X3D_CPP_SHARED_NODES must be ON or OFF" >&2; exit 2 ;; esac
echo "Hosted runtime proof: $work (shared nodes: $shared, jobs: $jobs)"
source="$work/source-build"
consumer="$cpp/tests/cmake/sai_hosted"
cmake -S "$consumer" -B "$source" -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_LIBDIR=lib -DX3D_CPP_SOURCE_DIR="$cpp" \
  -DX3D_SAI_SOURCE_DIR="$sai" -DX3D_CPP_SHARED_NODES="$shared"
# The SAI package exports its existing offline reference archive too. Build it
# for installation only; it must never enter either native executable's link.
cmake --build "$source" --target sai_hosted_all x3d_sai_reference_provider --parallel "$jobs"

check_native_link() {
  local build="$1" target="$2" command
  command="$(ninja -C "$build" -t commands "$target" | tail -1)"
  printf '%s\n' "$command" > "$build/$target-link-command.txt"
  if [[ "$command" != *"-o $target"* ]]; then
    echo "could not identify $target's link command" >&2
    exit 1
  fi
  if grep -E 'sai_experimental|sai_reference|sai_metadata' "$build/$target-link-command.txt"; then
    echo "native hosted consumer links the SAI reference/kernel/metadata implementation" >&2
    exit 1
  fi
  # Independence is a link-time property, not merely absence of an include.
  nm -C "$build/$target" > "$build/$target-symbols.txt"
  if grep -E 'x3d::sai::experimental::(browser::|execution_context::|scene_snapshot::|make_reference_|generated_type_registry)' "$build/$target-symbols.txt"; then
    echo "native hosted executable contains reference-kernel symbols" >&2
    exit 1
  fi
}
for target in sai_hosted_native sai_hosted_native_evidence; do
  check_native_link "$source" "$target"
done
ctest --test-dir "$source" --output-on-failure -R '^sai_hosted_'

prefix="$work/original-prefix"
moved="$work/relocated-prefix"
cmake --install "$source/x3d-cpp" --prefix "$prefix"
cmake --install "$source/x3d-sai" --prefix "$prefix"
cmake --install "$source/adapter" --prefix "$prefix"
for header in x3d_cpp/x3d/sai_hosted.hpp x3d/sai/experimental/hosted.hpp \
              x3d/sai/experimental/testing/hosted_fixture.hpp; do
  test -f "$prefix/include/$header"
done
if grep -RIlF -e "$cpp" -e "$sai" -e "$source" "$prefix/lib/cmake"; then
  echo "installed CMake package leaks a source/build path" >&2
  exit 1
fi
mv "$prefix" "$moved"
cp -R "$consumer" "$work/consumer"
cp "$cpp/adapters/sai_hosted/tests/hosted_native_test.cpp" "$work/consumer/native_evidence.cpp"
cmake -S "$work/consumer" -B "$work/installed-build" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DSAI_HOSTED_INSTALLED=ON \
  -DCMAKE_PREFIX_PATH="$moved" -DCMAKE_FIND_USE_PACKAGE_REGISTRY=OFF \
  -DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=OFF
cmake --build "$work/installed-build" --target sai_hosted_all --parallel "$jobs"
for target in sai_hosted_native sai_hosted_native_evidence; do
  check_native_link "$work/installed-build" "$target"
done
ctest --test-dir "$work/installed-build" --output-on-failure -R '^sai_hosted_'
echo "Hosted runtime passed: source and relocated native/reference/parity plus native evidence ($shared shared nodes)"
