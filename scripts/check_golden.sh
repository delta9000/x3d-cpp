#!/usr/bin/env bash
# Golden-drift gate.
#
# Regenerates the full generated C++ source tree into a throwaway temp directory
# and diffs every generated source file (*.hpp and *.cpp) against the committed
# generated_cpp_bindings/. Exits 0 when the trees are identical and non-zero
# (with a readable report) on ANY drift.
#
# Codegen changes are therefore opt-in: change a template/emitter, then run
# `uv run x3d-cpp-gen --out generated_cpp_bindings` and COMMIT the new sources.
# This script (and tests/test_golden_tree.py) fail until that is done.
#
# Runnable locally (`scripts/check_golden.sh`) and from CI. The smoke test.cpp
# and test_exec are generation artifacts (gitignored) and are excluded from the
# comparison; every other *.hpp and *.cpp is golden.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GOLDEN_DIR="${REPO_ROOT}/generated_cpp_bindings"

if [[ ! -d "${GOLDEN_DIR}" ]]; then
  echo "ERROR: golden dir not found: ${GOLDEN_DIR}" >&2
  exit 2
fi

# The golden tree is byte-exact, so the formatter is part of the contract and
# the FULL version is what we pin: clang-format's default style can shift in any
# release, not just a major. A mismatch would surface as "golden drift" that is
# really a toolchain mismatch -- fail loudly with the real reason instead. Keep
# this in sync with mise.toml [tools] and tests/test_formatter_pin.py.
EXPECTED_CLANG_FORMAT_VERSION=22.1.8
# Exported so the generator below formats with the SAME binary we just verified
# (it reads CLANG_FORMAT from the environment); otherwise this check could pass
# while generation silently used a different formatter off PATH.
export CLANG_FORMAT="${CLANG_FORMAT:-clang-format}"

if ! command -v "${CLANG_FORMAT}" >/dev/null 2>&1; then
  echo "ERROR: '${CLANG_FORMAT}' not found. The golden tree is byte-exact and" >&2
  echo "cannot be verified without the pinned formatter. Install it with:" >&2
  echo "  mise install clang-format@${EXPECTED_CLANG_FORMAT_VERSION}" >&2
  exit 2
fi

CLANG_FORMAT_RAW="$("${CLANG_FORMAT}" --version 2>/dev/null || true)"
echo "Formatter: ${CLANG_FORMAT_RAW}"
CLANG_FORMAT_VERSION="$(printf '%s' "${CLANG_FORMAT_RAW}" | sed -n 's/.*version \([0-9]*\.[0-9]*\.[0-9]*\).*/\1/p')"
if [[ "${CLANG_FORMAT_VERSION}" != "${EXPECTED_CLANG_FORMAT_VERSION}" ]]; then
  echo "ERROR: clang-format '${CLANG_FORMAT_VERSION}' != pinned ${EXPECTED_CLANG_FORMAT_VERSION}." >&2
  echo "The golden tree is byte-exact, so the full version is the contract;" >&2
  echo "refusing to run rather than report a toolchain mismatch as drift." >&2
  echo "Install the pin with: mise install clang-format@${EXPECTED_CLANG_FORMAT_VERSION}" >&2
  exit 2
fi

TMP_DIR="$(mktemp -d)"
cleanup() { rm -rf "${TMP_DIR}"; }
trap cleanup EXIT

echo "Regenerating into ${TMP_DIR} ..."
# --no-test: the smoke test is not golden and needs a compiler we may not want
# in the drift gate. clang-format must match the one that produced the golden.
( cd "${REPO_ROOT}" && uv run x3d-cpp-gen --out "${TMP_DIR}" --no-test >/dev/null )

# Compare the FULL generated source tree (*.hpp + *.cpp), in both directions
# (catches added/removed files as well as content drift).
DRIFT=0
REPORT=""

# 1. Generated sources present in golden: must exist and match in the regen.
while IFS= read -r -d '' g; do
  rel="${g#"${GOLDEN_DIR}/"}"
  r="${TMP_DIR}/${rel}"
  if [[ ! -f "${r}" ]]; then
    REPORT+=$'\n'"MISSING in regen: ${rel}"
    DRIFT=1
    continue
  fi
  if ! diff -q "${g}" "${r}" >/dev/null; then
    # `|| true` so a nonzero diff status never trips `set -e`/pipefail here.
    delta="$(diff -u "${g}" "${r}" | head -n 40 || true)"
    REPORT+=$'\n'"DRIFT: ${rel}"$'\n'"${delta}"
    DRIFT=1
  fi
done < <(find "${GOLDEN_DIR}" \( -name '*.hpp' -o -name '*.cpp' \) ! -name 'test.cpp' -print0)

# 2. Generated sources present in regen but NOT in golden (uncommitted output).
while IFS= read -r -d '' r; do
  rel="${r#"${TMP_DIR}/"}"
  if [[ ! -f "${GOLDEN_DIR}/${rel}" ]]; then
    REPORT+=$'\n'"EXTRA (not committed): ${rel}"
    DRIFT=1
  fi
done < <(find "${TMP_DIR}" \( -name '*.hpp' -o -name '*.cpp' \) ! -name 'test.cpp' -print0)

if [[ "${DRIFT}" -ne 0 ]]; then
  echo "GOLDEN DRIFT DETECTED:" >&2
  echo "${REPORT}" >&2
  echo "" >&2
  echo "Regenerate and commit intentional codegen changes:" >&2
  echo "  uv run x3d-cpp-gen --out generated_cpp_bindings" >&2
  exit 1
fi

echo "Golden tree OK: generated *.hpp/*.cpp match generated_cpp_bindings/ byte-for-byte."
exit 0
