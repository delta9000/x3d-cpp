#!/usr/bin/env bash
set -euo pipefail
CLI="$1"
GATE="$2"
TD=$(mktemp -d)
trap 'rm -rf "$TD"' EXIT
mkdir -p "$TD/corpus" "$TD/goldens"
cat > "$TD/corpus/case.x3d" <<'XML'
<X3D profile="Interchange" version="4.0"><Scene><Shape><Box/></Shape></Scene></X3D>
XML
printf 'case.x3d\n' > "$TD/goldens/subset.txt"
BASELINE="$TD/goldens/canon-gate-baseline.tsv"
printf 'check_key\tresult\ncanon-t2:case.x3d\tPASS\n' > "$BASELINE"
cp "$BASELINE" "$TD/before.tsv"
if "$GATE" "$CLI" "$TD/corpus" "$TD/goldens" --write-baseline > "$TD/write.log" 2>&1; then
    echo "FAIL: Tier-1-only refresh erased a Tier-2 baseline" >&2
    exit 1
fi
cmp "$TD/before.tsv" "$BASELINE"
echo "ok: Tier-1-only refresh preserves an existing Tier-2 baseline"

rm "$BASELINE"
"$GATE" "$CLI" "$TD/corpus" "$TD/goldens" --write-baseline > "$TD/write.log" 2>&1
"$GATE" "$CLI" "$TD/corpus" "$TD/goldens" --gate > "$TD/gate.log" 2>&1
echo "ok: Tier-1-only write then gate accepts a header-only baseline"

mkdir "$TD/goldens/canonical-goldens"
"$CLI" canonicalize "$TD/corpus/case.x3d" > "$TD/goldens/canonical-goldens/caseCanonical.xml"
rm "$BASELINE"
if "$GATE" "$CLI" "$TD/corpus" "$TD/goldens" --gate > "$TD/gate.log" 2>&1; then
    echo "FAIL: Tier-2 checks accepted a missing baseline" >&2
    exit 1
fi
printf '# canonical baseline\n# no differential checks\ncheck_key\tresult\n' > "$BASELINE"
if "$GATE" "$CLI" "$TD/corpus" "$TD/goldens" --gate > "$TD/gate.log" 2>&1; then
    echo "FAIL: Tier-2 checks accepted an empty baseline" >&2
    exit 1
fi
echo "ok: Tier-2 checks reject missing and empty baselines"

"$GATE" "$CLI" "$TD/corpus" "$TD/goldens" --write-baseline > "$TD/write.log" 2>&1
"$GATE" "$CLI" "$TD/corpus" "$TD/goldens" --gate > "$TD/gate.log" 2>&1
printf '<X3D><Scene/></X3D>\n' > "$TD/goldens/canonical-goldens/caseCanonical.xml"
if "$GATE" "$CLI" "$TD/corpus" "$TD/goldens" --gate > "$TD/gate.log" 2>&1; then
    echo "FAIL: Tier-2 baseline regression was accepted" >&2
    exit 1
fi
if "$GATE" "$CLI" "$TD/missing-corpus" "$TD/goldens" --gate > "$TD/gate.log" 2>&1; then
    echo "FAIL: a run with zero checks was accepted" >&2
    exit 1
fi
echo "ok: Tier-2 regressions and zero-check runs fail"
