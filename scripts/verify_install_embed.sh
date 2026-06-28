#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 2 ]; then
  echo "usage: $0 <build-dir> <work-dir>" >&2
  exit 2
fi

build_dir="$1"
work_dir="$2"
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

if find "$prefix/include/x3d_cpp" \( \
    -type d \( -name tests -o -name fixtures -o -name test_support \) \
    -o -type f \( -name '*TestSupport.*' -o -name '*test_support*' \) \
  \) | grep -q .; then
  echo "installed include tree contains test/support/fixture artifacts" >&2
  find "$prefix/include/x3d_cpp" \( \
    -type d \( -name tests -o -name fixtures -o -name test_support \) \
    -o -type f \( -name '*TestSupport.*' -o -name '*test_support*' \) \
  \) >&2
  exit 1
fi

cmake -S "$product_src" -B "$product_build" -G Ninja \
  -DCMAKE_PREFIX_PATH="$prefix"
cmake --build "$product_build"
"$product_build/x3d_embed_minimal"
