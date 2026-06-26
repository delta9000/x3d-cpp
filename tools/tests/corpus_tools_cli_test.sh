#!/usr/bin/env bash
# tools/tests/corpus_tools_cli_test.sh
# ─────────────────────────────────────────────────────────────────────────────
# CLI regression test for corpus_sweep and corpus_audit.
# Verifies: graceful handling of bad numeric args (no abort), empty dirs,
# missing positional args, and usage-text completeness.
# ─────────────────────────────────────────────────────────────────────────────
set -euo pipefail

SWEEP="$1"
AUDIT="$2"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

failures=0
check() {
    local name="$1"
    local got="$2"
    local want="$3"
    if [[ "$got" != "$want" ]]; then
        echo "FAIL: $name (got=$got want=$want)"
        ((failures++))
    else
        echo "ok: $name"
    fi
}

# Run a command swallowing its output, print its exit code.
# This function itself always succeeds so callers can safely use command
# substitution without tripping set -e.
run() {
    "$@" >/dev/null 2>&1
    echo $?
}

# 1. Bad numeric args must trigger a clean usage exit (64), NOT abort (134).
check "sweep bad limit"           "$(run "$SWEEP" /tmp/nonexistent --limit abc)"           "64"
check "sweep bad ratio"           "$(run "$SWEEP" /tmp/nonexistent --min-success-ratio abc)" "64"
check "sweep bad list-errors"     "$(run "$SWEEP" /tmp/nonexistent --list-errors abc)"     "64"
check "audit bad limit"           "$(run "$AUDIT" /tmp/nonexistent --limit abc)"           "64"
check "audit bad roundtrip-limit" "$(run "$AUDIT" /tmp/nonexistent --roundtrip-limit abc)" "64"

# 2. Numeric overflow must be caught (stol throws out_of_range on 64-bit).
check "sweep limit overflow" "$(run "$SWEEP" /tmp/nonexistent --limit 999999999999999999999)" "64"
check "audit limit overflow" "$(run "$AUDIT" /tmp/nonexistent --limit 999999999999999999999)" "64"

# 3. Missing positional argument.
check "sweep no args"             "$(run "$SWEEP")" "64"
check "audit no args"             "$(run "$AUDIT")" "64"

# 4. Empty directories are handled gracefully (exit 0).
mkdir -p "$TMPDIR/empty_sweep" "$TMPDIR/empty_audit"
check "sweep empty dir"           "$(run "$SWEEP" "$TMPDIR/empty_sweep")"                    "0"
check "audit empty dir"           "$(run "$AUDIT" "$TMPDIR/empty_audit" --out "$TMPDIR/out.jsonl")" "0"

# 5. Usage text for sweep must mention --trace.
usage=$("$SWEEP" 2>&1 || true)
if [[ "$usage" == *"--trace"* ]]; then
    echo "ok: sweep usage mentions --trace"
else
    echo "FAIL: sweep usage omits --trace"
    ((failures++))
fi

if (( failures )); then
    echo ""
    echo "corpus_tools_cli_test: $failures failure(s)"
    exit 1
fi
echo "corpus_tools_cli_test: all checks passed"
exit 0
