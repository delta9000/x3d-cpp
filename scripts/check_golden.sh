#!/usr/bin/env bash
# Golden-drift gate.
#
# Regenerates the full C++ binding tree into a throwaway temp directory and
# diffs every *.hpp against the committed generated_cpp_bindings/. Exits 0 when
# the trees are identical and non-zero (with a readable report) on ANY drift.
#
# Codegen changes are therefore opt-in: change a template/emitter, then run
# `uv run x3d-cpp-gen --out generated_cpp_bindings` and COMMIT the new headers.
# This script (and tests/test_golden_tree.py) fail until that is done.
#
# Runnable locally (`scripts/check_golden.sh`) and from CI. The smoke test.cpp
# and test_exec are generation artifacts (gitignored) and are excluded from the
# comparison; only *.hpp headers are golden.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GOLDEN_DIR="${REPO_ROOT}/generated_cpp_bindings"

if [[ ! -d "${GOLDEN_DIR}" ]]; then
  echo "ERROR: golden dir not found: ${GOLDEN_DIR}" >&2
  exit 2
fi

TMP_DIR="$(mktemp -d)"
cleanup() { rm -rf "${TMP_DIR}"; }
trap cleanup EXIT

echo "Regenerating into ${TMP_DIR} ..."
# --no-test: the smoke test is not golden and needs a compiler we may not want
# in the drift gate. clang-format must match the one that produced the golden.
( cd "${REPO_ROOT}" && uv run x3d-cpp-gen --out "${TMP_DIR}" --no-test >/dev/null )

# Compare the FULL tree of *.hpp, in both directions (catches added/removed
# headers as well as content drift).
DRIFT=0
REPORT=""

# 1. Headers present in golden: must exist and match in the regen.
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

# 2. Headers present in regen but NOT in golden (uncommitted new output).
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

echo "Golden tree OK: regenerated *.hpp match generated_cpp_bindings/ byte-for-byte."
exit 0
