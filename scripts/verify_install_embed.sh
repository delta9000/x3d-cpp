#!/usr/bin/env bash
# Installed-package embed smoke. Installs the given build into a throwaway prefix,
# asserts the installed include tree is clean (façade present, no private
# artifacts), then configures/builds/runs examples/embed_minimal against it via
# find_package(x3d_cpp) — proving the actual downstream embedder path.
set -euo pipefail

if [ "$#" -lt 2 ] || [ "$#" -gt 3 ]; then
  echo "usage: $0 <build-dir> <work-dir> [generator]" >&2
  exit 2
fi

build_dir="$1"
work_dir="$2"
# Default to Ninja (every CMake preset uses it); the caller forwards the parent
# build's CMAKE_GENERATOR so the downstream configure can't pick a different one.
generator="${3:-Ninja}"

if [ -z "$build_dir" ] || [ -z "$work_dir" ]; then
  echo "build-dir and work-dir must both be non-empty" >&2
  exit 2
fi

prefix="$work_dir/prefix"
product_src="$work_dir/product-src"
product_build="$work_dir/product-build"
example_src="$(cd "$(dirname "$0")/.." && pwd)/examples/embed_minimal"

rm -rf "$prefix" "$product_src" "$product_build"
mkdir -p "$work_dir"
if [ ! -d "$example_src" ]; then
  echo "missing embed example: $example_src" >&2
  exit 1
fi
cp -R "$example_src" "$product_src"

cmake --install "$build_dir" --prefix "$prefix"

# Positively assert the install produced the façade umbrella header, so a
# regression that drops the header tree fails HERE instead of slipping past the
# leak scan vacuously (find over a missing dir prints nothing).
if [ ! -f "$prefix/include/x3d_cpp/x3d/sdk.hpp" ]; then
  echo "install did not produce include/x3d_cpp/x3d/sdk.hpp under $prefix" >&2
  exit 1
fi

# Leak gate — kept identical to _x3d_install_excludes in CMakeLists.txt: no
# test/support/fixture/vendor artifact may reach the public include tree.
leaked="$(find "$prefix/include/x3d_cpp" \( \
    -type d \( -name tests -o -name fixtures -o -name test_support -o -name vendor \) \
    -o -type f \( -name '*TestSupport.*' -o -name '*test_support*' \) \
  \) -print)"
if [ -n "$leaked" ]; then
  echo "installed include tree contains test/support/fixture/vendor artifacts:" >&2
  echo "$leaked" >&2
  exit 1
fi

# IO-free gate — kept in sync with _x3d_runtime_backend_excludes in CMakeLists.txt:
# the SDK ships seam interfaces + core, never a concrete/reference backend.
backends="$(find "$prefix/include/x3d_cpp" \( \
    -type d \( -name miniaudio -o -name dsp -o -name io -o -name physics -o -name ext \) \
    -o -type f -name 'RecordingBackend.hpp' \
  \) -print)"
if [ -n "$backends" ]; then
  echo "installed include tree contains reference/IO backends (SDK must be IO-free):" >&2
  echo "$backends" >&2
  exit 1
fi

cmake -S "$product_src" -B "$product_build" -G "$generator" \
  -DCMAKE_PREFIX_PATH="$prefix"
cmake --build "$product_build"
"$product_build/x3d_embed_minimal"
