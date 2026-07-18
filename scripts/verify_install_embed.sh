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

# Contract: assert every documented imported target exists against the REAL
# install prefix. embed_minimal links only two of the seven directly, so a broken
# EXPORT_NAME elsewhere would otherwise pass -- the in-tree ALIAS resolves while
# the installed name silently differs. Configure-only.
targets_src="$(cd "$(dirname "$0")/.." && pwd)/tests/cmake/installed_targets"
echo "Checking installed target contract ..."
cmake -S "$targets_src" -B "$work_dir/installed-targets" -G "$generator" \
  -DCMAKE_PREFIX_PATH="$prefix" >/dev/null

cmake -S "$product_src" -B "$product_build" -G "$generator" \
  -DCMAKE_PREFIX_PATH="$prefix"
cmake --build "$product_build"
# The two halves of the README quickstart, chained: the authoring hello world
# writes hello.x3d into the scratch work dir, the consumer parses it back and
# must extract real mesh data (author -> serialize -> parse -> extract).
(cd "$work_dir" && "$product_build/x3d_embed_authoring")
[ -s "$work_dir/hello.x3d" ] || { echo "authoring example wrote no hello.x3d" >&2; exit 1; }
(cd "$work_dir" && "$product_build/x3d_embed_minimal")

# ── Relocatability: the package must work from a MOVED copy of the prefix ─────
# Static half: no absolute source- or build-tree path may be baked into any
# installed CMake file (prefix-relative _IMPORT_PREFIX only). A hit here is a
# relocatability bug even when the dynamic half below happens to pass.
repo_root="$(cd "$(dirname "$0")/.." && pwd)"
build_dir_abs="$(cd "$build_dir" && pwd)"
leaks="$(grep -RIl -e "$repo_root" -e "$build_dir_abs" "$prefix"/lib*/cmake 2>/dev/null || true)"
if [ -n "$leaks" ]; then
  echo "installed CMake files embed absolute source/build paths (not relocatable):" >&2
  echo "$leaks" >&2
  exit 1
fi

# Dynamic half: copy the prefix, HIDE the original, then configure + build +
# run the embed pair against the copy from a fresh build dir — proving no
# configure, link, or runtime step still reaches into the original install
# location (e.g. via a baked absolute RPATH or cached import path).
moved="$work_dir/prefix-moved"
moved_build="$work_dir/product-build-moved"
rm -rf "$moved" "$moved_build"
cp -R "$prefix" "$moved"
mv "$prefix" "$prefix.hidden"
trap 'mv -f "$prefix.hidden" "$prefix" 2>/dev/null || true' EXIT
cmake -S "$product_src" -B "$moved_build" -G "$generator" \
  -DCMAKE_PREFIX_PATH="$moved" >/dev/null
cmake --build "$moved_build" >/dev/null
(cd "$work_dir" && "$moved_build/x3d_embed_authoring")
(cd "$work_dir" && "$moved_build/x3d_embed_minimal")
mv "$prefix.hidden" "$prefix"
trap - EXIT
echo "relocatable: embed pair configured, built, and ran against a moved prefix with the original hidden"
