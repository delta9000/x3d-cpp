#!/usr/bin/env bash
# tools/x3d-cli/gen_canon_goldens.sh
# ─────────────────────────────────────────────────────────────────────────────
# Generate X3DJSAIL -canonical reference fixtures for the canonical tiered gate.
#
# For each .x3d file in tools/x3d-cli/goldens/subset.txt, runs X3DJSAIL
# -canonical (JDK 25) and captures the canonical output at:
#   tools/x3d-cli/goldens/canonical-goldens/<relpath-stem>Canonical.xml
#
# These committed fixtures enable the Java-free tier-2/3 differential gate
# (canon_gate) to compare our canonical output without requiring JDK at CI time.
#
# Usage:
#   ./tools/x3d-cli/gen_canon_goldens.sh [CORPUS_ROOT] [JAR_DIR]
#
# Defaults:
#   CORPUS_ROOT  $X3D_CORPUS_DIR (a local X3D conformance archive checkout)
#   JAR_DIR      tools/x3d-cli/.javacache
#
# Requires: a JDK >= 25 (the minimum that supports X3DJSAIL -canonical). The
#           `java` binary is resolved in this order: $X3D_JDK_BIN, then
#           $JAVA_HOME/bin/java, then `mise which java`, then `java` on PATH.
#           Install one with e.g. `mise install java@temurin-25.0.3+9.0.LTS`.
# ─────────────────────────────────────────────────────────────────────────────
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

CORPUS_ROOT="${1:-${X3D_CORPUS_DIR:-}}"
JAR_DIR="${2:-$REPO_ROOT/tools/x3d-cli/.javacache}"
JAR="$JAR_DIR/X3DJSAIL.jar"

# Resolve a JDK >= 25 portably (X3DJSAIL -canonical needs it). Override with
# X3D_JDK_BIN; otherwise try JAVA_HOME, then mise, then PATH.
if [[ -n "${X3D_JDK_BIN:-}" ]]; then
    JDK25_BIN="$X3D_JDK_BIN"
elif [[ -n "${JAVA_HOME:-}" && -x "$JAVA_HOME/bin/java" ]]; then
    JDK25_BIN="$JAVA_HOME/bin/java"
elif command -v mise >/dev/null 2>&1 && mise which java >/dev/null 2>&1; then
    JDK25_BIN="$(mise which java)"
else
    JDK25_BIN="$(command -v java || true)"
fi

GOLDENS_DIR="$REPO_ROOT/tools/x3d-cli/goldens"
SUBSET_TXT="$GOLDENS_DIR/subset.txt"
CANON_GOLDENS_DIR="$GOLDENS_DIR/canonical-goldens"

# ── pre-flight ────────────────────────────────────────────────────────────────
if [[ ! -f "$JAR" ]]; then
    echo "ERROR: X3DJSAIL.jar not found at $JAR" >&2
    exit 1
fi
if [[ -z "$JDK25_BIN" || ! -x "$JDK25_BIN" ]]; then
    echo "ERROR: no java found (X3DJSAIL -canonical needs JDK >= 25)." >&2
    echo "Set X3D_JDK_BIN/JAVA_HOME, or: mise install java@temurin-25.0.3+9.0.LTS" >&2
    exit 1
fi
jdk_major="$("$JDK25_BIN" -version 2>&1 | sed -nE 's/.*version "([0-9]+).*/\1/p' | head -1)"
if [[ -z "$jdk_major" || "$jdk_major" -lt 25 ]]; then
    echo "ERROR: $JDK25_BIN is JDK '${jdk_major:-unknown}'; X3DJSAIL -canonical needs >= 25." >&2
    echo "Set X3D_JDK_BIN/JAVA_HOME, or: mise install java@temurin-25.0.3+9.0.LTS" >&2
    exit 1
fi
if [[ ! -f "$SUBSET_TXT" ]]; then
    echo "ERROR: subset.txt not found at $SUBSET_TXT" >&2
    echo "Run 'mise run cli-golden-gen' first." >&2
    exit 1
fi

mkdir -p "$CANON_GOLDENS_DIR"

# ── generate ─────────────────────────────────────────────────────────────────
echo "=== Generating X3DJSAIL canonical fixtures ==="
echo "  Subset: $SUBSET_TXT"
echo "  Output: $CANON_GOLDENS_DIR"
echo ""

ok=0
fail=0
skip=0
notdtd=0

TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

while IFS= read -r relpath; do
    # XML-only (JSAIL canonical only works on .x3d files).
    case "$relpath" in
        *.x3d) ;;
        *) (( skip++ )) || true; continue ;;
    esac

    abs="$CORPUS_ROOT/$relpath"
    if [[ ! -f "$abs" ]]; then
        (( skip++ )) || true
        continue
    fi

    # Work in a temp dir to avoid littering the source tree.
    fname="$(basename "$abs")"
    stemname="${fname%.x3d}"
    tmpfile="$TMPDIR/$fname"
    cp "$abs" "$tmpfile"

    # Run X3DJSAIL -canonical. Writes <tmpdir>/<stem>Canonical.xml.
    canon_out="$TMPDIR/${stemname}Canonical.xml"
    jsail_output=$(cd "$JAR_DIR" && "$JDK25_BIN" -cp X3DJSAIL.jar \
        org.web3d.x3d.jsail.CommandLine "$tmpfile" -canonical 2>&1 || true)

    if [[ -f "$canon_out" ]]; then
        # Construct the output path mirroring the relpath structure.
        rel_dir="$(dirname "$relpath")"
        out_dir="$CANON_GOLDENS_DIR/$rel_dir"
        mkdir -p "$out_dir"
        cp "$canon_out" "$out_dir/${stemname}Canonical.xml"
        rm -f "$canon_out"
        (( ok++ )) || true
    else
        # JSAIL failed (no DOCTYPE, parse error, etc.).
        if echo "$jsail_output" | grep -q "No X3D DOCTYPE found"; then
            (( notdtd++ )) || true
        else
            (( fail++ )) || true
        fi
    fi

    # Clean up the temp copy.
    rm -f "$tmpfile"

done < "$SUBSET_TXT"

echo ""
echo "=== Canon golden-gen complete ==="
echo "  Generated:        $ok"
echo "  No-DTD (skipped): $notdtd"
echo "  Other failures:   $fail"
echo "  Non-XML (skipped):$skip"
echo ""
echo "Commit: git add $CANON_GOLDENS_DIR && git commit -m 'test: X3DJSAIL canonical goldens'"
