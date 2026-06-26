#!/usr/bin/env bash
# tools/x3d-cli/gen_golden.sh
# ─────────────────────────────────────────────────────────────────────────────
# Golden-gen: run X3DJSAIL -validate over a curated corpus subset and produce:
#   tools/x3d-cli/goldens/subset.txt            — committed, relative paths
#   tools/x3d-cli/goldens/validate-verdicts.tsv — committed, verdict fixture
#
# Usage:
#   ./tools/x3d-cli/gen_golden.sh [CORPUS_ROOT] [JAR_DIR]
#
# Defaults:
#   CORPUS_ROOT  $X3D_CORPUS_DIR (a local X3D conformance archive checkout)
#   JAR_DIR      tools/x3d-cli/.javacache
#
# The jar must be named X3DJSAIL.jar inside JAR_DIR (the known-working
# invocation is `cd <jar_dir> && java -cp X3DJSAIL.jar org.web3d.x3d.jsail.CommandLine <abs_path> -validate`).
#
# X3DJSAIL can only process XML (.x3d) files — it throws X3DException on
# .wrl / .x3dv / .json input.  The validate-differential therefore restricts
# the golden subset to .x3d files only.  Non-XML files are excluded so they
# cannot pollute the agreement rate with false INVALID verdicts.
#
# X3DJSAIL exit codes are NOT reliable; we normalise via output text:
#   VALID   iff output contains "validate results: success"
#   INVALID otherwise — capture the first actionable reason line:
#            priority 1: WARNING_PROTOINSTANCE_NOT_FOUND / WARNING_MESSAGE line
#            priority 2: InvalidFieldException / InvalidFieldValueException line
#            priority 3: first "validate results:" line that isn't "success"
#            priority 4: first non-boilerplate stderr line
#
# Subset selection strategy (deterministic, reproducible):
#   From the www.web3d.org tree of the corpus archive:
#   - .x3d files only (X3DJSAIL is XML-only)
#   - stride 80 → ~200 files
#   Sort → commit as subset.txt.
# ─────────────────────────────────────────────────────────────────────────────
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

CORPUS_ROOT="${1:-${X3D_CORPUS_DIR:-}}"
JAR_DIR="${2:-$REPO_ROOT/tools/x3d-cli/.javacache}"
JAR="$JAR_DIR/X3DJSAIL.jar"

GOLDENS_DIR="$REPO_ROOT/tools/x3d-cli/goldens"
SUBSET_TXT="$GOLDENS_DIR/subset.txt"
VERDICTS_TSV="$GOLDENS_DIR/validate-verdicts.tsv"

# ── pre-flight ────────────────────────────────────────────────────────────────
if [[ ! -d "$CORPUS_ROOT" ]]; then
    echo "ERROR: corpus root not found: $CORPUS_ROOT" >&2
    exit 1
fi
if [[ ! -f "$JAR" ]]; then
    echo "INFO: X3DJSAIL.jar not found at $JAR; attempting download..."
    mkdir -p "$JAR_DIR"
    curl -sL -o "$JAR" "https://www.web3d.org/specifications/java/jars/X3DJSAIL.4.0.full.jar" || {
        echo "ERROR: download failed. Place X3DJSAIL.4.0.full.jar at $JAR" >&2
        exit 1
    }
fi
java -version >/dev/null 2>&1 || { echo "ERROR: java not found on PATH" >&2; exit 1; }

mkdir -p "$GOLDENS_DIR"

# ── subset selection ─────────────────────────────────────────────────────────
echo "=== Selecting subset (XML .x3d files only) ==="
CONTENT_ROOT="$CORPUS_ROOT/x3d-code/www.web3d.org/x3d/content/examples"

# Collect .x3d file list (sorted, absolute paths)
mapfile -t x3d_files < <(find "$CONTENT_ROOT" -type f -name "*.x3d" | sort)

# Stride-sample to hit ~200 files
# .x3d: ~16314 → stride 80 → ~204
sample_array() {
    local stride="$1"
    shift
    local -a arr=("$@")
    local -a out=()
    for i in "${!arr[@]}"; do
        if (( i % stride == 0 )); then
            out+=("${arr[$i]}")
        fi
    done
    printf '%s\n' "${out[@]}"
}

sample_array 80 "${x3d_files[@]}" | sort -u > /tmp/x3d_subset_abs.txt

TOTAL=$(wc -l < /tmp/x3d_subset_abs.txt)
echo "Subset size: $TOTAL files"

# Write subset.txt as paths relative to CORPUS_ROOT
while IFS= read -r f; do
    echo "${f#"$CORPUS_ROOT/"}"
done < /tmp/x3d_subset_abs.txt | sort > "$SUBSET_TXT"

echo "Written: $SUBSET_TXT"

# ── run X3DJSAIL validate over each subset file ────────────────────────────
echo "=== Running X3DJSAIL -validate (this may take a few minutes) ==="

{
    echo -e "relpath\tverdict\tnote"
} > "$VERDICTS_TSV"

ok=0
invalid=0
error=0

while IFS= read -r relpath; do
    abs="$CORPUS_ROOT/$relpath"
    if [[ ! -f "$abs" ]]; then
        echo -e "$relpath\tINVALID\tfile-not-found" >> "$VERDICTS_TSV"
        (( error++ )) || true
        continue
    fi

    # X3DJSAIL requires cwd = jar dir so it can find itself by literal name on
    # CLASSPATH (known quirk; documented in the spec design doc).
    output=$(cd "$JAR_DIR" && java -cp X3DJSAIL.jar org.web3d.x3d.jsail.CommandLine \
        "$abs" -validate 2>&1 || true)

    if echo "$output" | grep -q "validate results: success"; then
        echo -e "$relpath\tVALID\t" >> "$VERDICTS_TSV"
        (( ok++ )) || true
    else
        # Extract the most informative one-line reason, in priority order:
        # 1. WARNING_PROTOINSTANCE_NOT_FOUND or WARNING_MESSAGE (semantic warnings)
        reason=$(echo "$output" | grep -E "WARNING_PROTOINSTANCE_NOT_FOUND|WARNING_MESSAGE" \
            | head -1 | sed 's/^[[:space:]]*//' | cut -c1-200 || true)
        # 2. InvalidFieldException / InvalidFieldValueException
        if [[ -z "$reason" ]]; then
            reason=$(echo "$output" | grep -E "InvalidField(Value)?Exception:" \
                | head -1 | sed 's/^[[:space:]]*//' | cut -c1-200 || true)
        fi
        # 3. [exception] during validation
        if [[ -z "$reason" ]]; then
            reason=$(echo "$output" | grep -E "\[exception\] during validation:" \
                | head -1 | sed 's/^[[:space:]]*//' | cut -c1-200 || true)
        fi
        # 4. ERROR_* / line following "validate results:" on its own line
        if [[ -z "$reason" ]]; then
            reason=$(echo "$output" | grep -E "^ERROR_" \
                | head -1 | sed 's/^[[:space:]]*//' | cut -c1-200 || true)
        fi
        # 5. First "validate results:" line with content after the colon
        if [[ -z "$reason" ]]; then
            reason=$(echo "$output" | grep "validate results:" \
                | grep -v "^validate results:$" | grep -v "success" \
                | head -1 | sed 's/^[[:space:]]*//' | cut -c1-200 || true)
        fi
        # 6. First non-boilerplate line
        if [[ -z "$reason" ]]; then
            reason=$(echo "$output" | grep -v "^parameter:\|^CommandLine\|^Parsing\|^validate results:$\|^$" \
                | head -1 | sed 's/^[[:space:]]*//' | cut -c1-200 || true)
        fi
        [[ -z "$reason" ]] && reason="(no output)"
        # Escape tabs in reason
        reason="${reason//$'\t'/ }"
        echo -e "$relpath\tINVALID\t$reason" >> "$VERDICTS_TSV"
        (( invalid++ )) || true
    fi
done < "$SUBSET_TXT"

echo ""
echo "=== Golden-gen complete ==="
echo "  Total:   $TOTAL"
echo "  VALID:   $ok"
echo "  INVALID: $invalid"
echo "  Errors:  $error"
echo ""
echo "Written: $VERDICTS_TSV"
echo ""
echo "Next: run 'mise run cli-gate' to compare our validator against these verdicts."
