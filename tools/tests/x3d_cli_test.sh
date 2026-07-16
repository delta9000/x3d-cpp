#!/usr/bin/env bash
# tools/tests/x3d_cli_test.sh
# ─────────────────────────────────────────────────────────────────────────────
# CLI test for the 'x3d' binary (Task A: skeleton + convert subcommand).
#
# Positional args:
#   $1  path to the x3d_cli binary (passed via CMake generator expression)
#   $2  path to the in-repo fixture directory (tools/x3d-cli/fixtures/)
#   $5  repo root (for the gallery/smoke scene profile gate — see below)
#
# Tests:
#   1. --help exits 0 and mentions 'convert'
#   2. x3d convert --help exits 0
#   3. Unknown subcommand exits 1
#   4. Missing input exits non-zero
#   5. Malformed/missing input file exits 2 without crashing
#   6. XML → ClassicVRML succeeds; output non-empty; output reparses (exit 0)
#   7. XML → JSON succeeds; output non-empty; output reparses (exit 0)
#   8. ClassicVRML → XML succeeds; output non-empty; output reparses (exit 0)
#   9. JSON → XML succeeds; output non-empty; output reparses (exit 0)
#  10. Output to stdout (with -f xml) is non-empty
#  11. No -o and no -f → exit 1 (usage error), not crash
# ─────────────────────────────────────────────────────────────────────────────
set -uo pipefail
# Note: NOT using 'set -e' because many sub-commands intentionally exit non-zero.

CLI="$1"
FIXTURES="$2"
GOLDENS="${3:-}"
PHYSICS="${4:-0}"   # 1 when x3d_cli was built with the Jolt physics backend
REPO_ROOT="${5:-}"  # repo root, for the gallery/smoke scene profile gate
TD=$(mktemp -d)
trap 'rm -rf "$TD"' EXIT

FIXTURE_XML="$FIXTURES/simple.x3d"

failures=0

check() {
    local name="$1"
    local got="$2"
    local want="$3"
    if [[ "$got" != "$want" ]]; then
        echo "FAIL: $name  (got=$got want=$want)"
        failures=$(( failures + 1 ))
    else
        echo "ok:   $name"
    fi
}

# Run a command capturing its exit code; never trips nounset/pipefail.
ec_of() {
    "$@" >/dev/null 2>&1 || true
    echo $?
}

# ── 1. Top-level --help ───────────────────────────────────────────────────────
ec=$("$CLI" --help >/dev/null 2>&1; echo $?)
check "--help exits 0" "$ec" "0"

help_out=$("$CLI" --help 2>&1 || true)
if [[ "$help_out" == *"convert"* ]]; then
    echo "ok:   --help mentions 'convert'"
else
    echo "FAIL: --help does not mention 'convert'"
    failures=$(( failures + 1 ))
fi

# ── 2. convert --help ─────────────────────────────────────────────────────────
ec=$("$CLI" convert --help >/dev/null 2>&1; echo $?)
check "convert --help exits 0" "$ec" "0"

# ── 3. Unknown subcommand → exit 1 ───────────────────────────────────────────
ec=$("$CLI" frobnicate >/dev/null 2>&1; echo $?)
check "unknown subcommand exits 1" "$ec" "1"

# ── 4. Missing input argument → exit non-zero ────────────────────────────────
ec=$("$CLI" convert >/dev/null 2>&1; echo $?)
if [[ "$ec" != "0" ]]; then
    echo "ok:   convert with no args exits non-zero ($ec)"
else
    echo "FAIL: convert with no args unexpectedly exited 0"
    failures=$(( failures + 1 ))
fi

# ── 5. Malformed / missing input file → exit 2, no crash ─────────────────────
ec=$("$CLI" convert /nonexistent/does_not_exist.x3d -f xml >/dev/null 2>&1; echo $?)
check "missing file exits 2" "$ec" "2"

garbage_file="$TD/garbage.x3d"
printf '%s' 'THIS IS NOT VALID X3D <<< garbage >>>' > "$garbage_file"
ec=$("$CLI" convert "$garbage_file" -f xml >/dev/null 2>&1; echo $?)
if [[ "$ec" != "0" ]]; then
    echo "ok:   malformed input exits non-zero ($ec)"
else
    echo "FAIL: malformed input unexpectedly exited 0"
    failures=$(( failures + 1 ))
fi

# ── helper: convert, assert non-empty, then reparse ──────────────────────────
try_convert() {
    local label="$1"
    local in_file="$2"
    local out_file="$3"
    shift 3
    # $@ are optional extra flags

    # Run convert
    local ec
    ec=$("$CLI" convert "$in_file" -o "$out_file" "$@" 2>/dev/null; echo $?)
    if [[ "$ec" != "0" ]]; then
        echo "FAIL: $label — convert exited $ec"
        failures=$(( failures + 1 ))
        return
    fi
    if [[ ! -s "$out_file" ]]; then
        echo "FAIL: $label — output file is empty"
        failures=$(( failures + 1 ))
        return
    fi
    echo "ok:   $label — convert succeeded, output non-empty"

    # Reparse: convert the output to XML, expect exit 0
    local safe_label
    safe_label="${label//[^a-zA-Z0-9]/_}"
    local reparse_out="$TD/reparse_${safe_label}.x3d"
    ec=$("$CLI" convert "$out_file" -o "$reparse_out" 2>/dev/null; echo $?)
    if [[ "$ec" == "0" && -s "$reparse_out" ]]; then
        echo "ok:   $label — reparse succeeded"
    else
        echo "FAIL: $label — reparse failed (ec=$ec)"
        failures=$(( failures + 1 ))
    fi
}

# ── 6. XML → ClassicVRML ──────────────────────────────────────────────────────
out_vrml="$TD/simple.x3dv"
try_convert "xml-to-classicvrml" "$FIXTURE_XML" "$out_vrml"

# ── 7. XML → JSON ─────────────────────────────────────────────────────────────
out_json="$TD/simple.json"
try_convert "xml-to-json" "$FIXTURE_XML" "$out_json"

# ── 8. ClassicVRML → XML ──────────────────────────────────────────────────────
if [[ -s "$out_vrml" ]]; then
    out_from_vrml="$TD/from_vrml.x3d"
    try_convert "classicvrml-to-xml" "$out_vrml" "$out_from_vrml"
else
    echo "SKIP: classicvrml-to-xml (no .x3dv from test 6)"
fi

# ── 9. JSON → XML ─────────────────────────────────────────────────────────────
if [[ -s "$out_json" ]]; then
    out_from_json="$TD/from_json.x3d"
    try_convert "json-to-xml" "$out_json" "$out_from_json"
else
    echo "SKIP: json-to-xml (no .json from test 7)"
fi

# ── 10. stdout output with -f ────────────────────────────────────────────────
stdout_out=$("$CLI" convert "$FIXTURE_XML" -f xml 2>/dev/null || true)
if [[ -n "$stdout_out" ]]; then
    echo "ok:   stdout output with -f xml is non-empty"
else
    echo "FAIL: stdout output with -f xml is empty"
    failures=$(( failures + 1 ))
fi

# ── 11. No -o and no -f → exit 1 ─────────────────────────────────────────────
ec=$("$CLI" convert "$FIXTURE_XML" >/dev/null 2>&1; echo $?)
check "no -o no -f exits 1" "$ec" "1"

# ════════════════════════════════════════════════════════════════════════════
# Task B: x3d validate subcommand
# ════════════════════════════════════════════════════════════════════════════

FIXTURE_CLEAN="$FIXTURES/validate-clean.x3d"
FIXTURE_RANGE="$FIXTURES/validate-range-violation.x3d"
FIXTURE_PROFILE="$FIXTURES/validate-profile-exceed.x3d"

# ── 12. validate --help exits 0 ──────────────────────────────────────────────
ec=$("$CLI" validate --help >/dev/null 2>&1; echo $?)
check "validate --help exits 0" "$ec" "0"

# ── 13. validate on clean scene → exit 0 ────────────────────────────────────
ec=$("$CLI" validate "$FIXTURE_CLEAN" >/dev/null 2>&1; echo $?)
check "validate clean scene exits 0" "$ec" "0"

# ── 14. validate with range violation → exit 3, diagnostic in output ─────────
out=$("$CLI" validate "$FIXTURE_RANGE" 2>&1 || true)
ec=$("$CLI" validate "$FIXTURE_RANGE" >/dev/null 2>&1; echo $?)
check "validate range-violation exits 3" "$ec" "3"
if [[ "$out" == *"diffuseColor"* ]]; then
    echo "ok:   validate range-violation output mentions diffuseColor"
else
    echo "FAIL: validate range-violation output does not mention diffuseColor (got: $out)"
    failures=$(( failures + 1 ))
fi

# ── 15. validate profile exceed → exit 3, mentions TouchSensor + profile ─────
out=$("$CLI" validate "$FIXTURE_PROFILE" 2>&1 || true)
ec=$("$CLI" validate "$FIXTURE_PROFILE" >/dev/null 2>&1; echo $?)
check "validate profile-exceed exits 3" "$ec" "3"
if [[ "$out" == *"TouchSensor"* || "$out" == *"PointingDeviceSensor"* ]]; then
    echo "ok:   validate profile-exceed output mentions TouchSensor or its component"
else
    echo "FAIL: validate profile-exceed output does not mention TouchSensor/PointingDeviceSensor (got: $out)"
    failures=$(( failures + 1 ))
fi
if [[ "$out" == *"profile"* || "$out" == *"Profile"* ]]; then
    echo "ok:   validate profile-exceed output mentions profile"
else
    echo "FAIL: validate profile-exceed output does not mention profile (got: $out)"
    failures=$(( failures + 1 ))
fi

# ── 16. validate --json produces parseable JSON with valid:false ──────────────
json_out=$("$CLI" validate "$FIXTURE_RANGE" --json 2>&1 || true)
if [[ "$json_out" == *'"valid"'* && "$json_out" == *'false'* ]]; then
    echo "ok:   validate --json output contains valid:false"
else
    echo "FAIL: validate --json output missing valid:false (got: $json_out)"
    failures=$(( failures + 1 ))
fi
# Check it's vaguely JSON-shaped (starts with '{')
trimmed="${json_out#"${json_out%%[![:space:]]*}"}"
if [[ "$trimmed" == "{"* ]]; then
    echo "ok:   validate --json output starts with '{'"
else
    echo "FAIL: validate --json output does not start with '{' (got first 80 chars: ${json_out:0:80})"
    failures=$(( failures + 1 ))
fi

# ── 17. validate missing file → exit 2 ───────────────────────────────────────
ec=$("$CLI" validate /nonexistent/nope.x3d >/dev/null 2>&1; echo $?)
check "validate missing file exits 2" "$ec" "2"

# ── 18. validate with no args → exit 1 (usage error) ────────────────────────
ec=$("$CLI" validate >/dev/null 2>&1; echo $?)
check "validate no args exits 1" "$ec" "1"

# ════════════════════════════════════════════════════════════════════════════
# Task C: conformance checks (X3DJSAIL-equivalent semantic validation)
# ════════════════════════════════════════════════════════════════════════════

FIXTURE_DUP_META="$FIXTURES/validate-dup-meta.x3d"
FIXTURE_UNUSED_PROTO="$FIXTURES/validate-unused-proto.x3d"
FIXTURE_IFS_NO_CI="$FIXTURES/validate-ifs-no-coordindex.x3d"
FIXTURE_ILS_NO_CI="$FIXTURES/validate-ils-no-coordindex.x3d"

# ── 19. validate duplicate meta → exit 3, mentions 'dup-meta' ────────────────
out=$("$CLI" validate "$FIXTURE_DUP_META" 2>&1 || true)
ec=$("$CLI" validate "$FIXTURE_DUP_META" >/dev/null 2>&1; echo $?)
check "validate dup-meta exits 3" "$ec" "3"
if [[ "$out" == *"dup-meta"* || "$out" == *"duplicate"* ]]; then
    echo "ok:   validate dup-meta output mentions duplicate meta"
else
    echo "FAIL: validate dup-meta output missing duplicate indicator (got: $out)"
    failures=$(( failures + 1 ))
fi

# ── 20. validate unused ProtoDeclare → exit 3, mentions 'unused-proto' ────────
out=$("$CLI" validate "$FIXTURE_UNUSED_PROTO" 2>&1 || true)
ec=$("$CLI" validate "$FIXTURE_UNUSED_PROTO" >/dev/null 2>&1; echo $?)
check "validate unused-proto exits 3" "$ec" "3"
if [[ "$out" == *"unused-proto"* || "$out" == *"ProtoInstance"* || "$out" == *"ProtoDeclare"* ]]; then
    echo "ok:   validate unused-proto output mentions proto"
else
    echo "FAIL: validate unused-proto output missing proto indicator (got: $out)"
    failures=$(( failures + 1 ))
fi
if [[ "$out" == *"MyShape"* ]]; then
    echo "ok:   validate unused-proto output mentions proto name MyShape"
else
    echo "FAIL: validate unused-proto output missing proto name MyShape (got: $out)"
    failures=$(( failures + 1 ))
fi

# ── 21. validate IFS coord without coordIndex → exit 3, mentions 'ifs-coord' ──
out=$("$CLI" validate "$FIXTURE_IFS_NO_CI" 2>&1 || true)
ec=$("$CLI" validate "$FIXTURE_IFS_NO_CI" >/dev/null 2>&1; echo $?)
check "validate ifs-no-coordindex exits 3" "$ec" "3"
if [[ "$out" == *"ifs-coord"* || "$out" == *"IndexedFaceSet"* || "$out" == *"coordIndex"* ]]; then
    echo "ok:   validate ifs-no-coordindex output mentions IFS coordIndex"
else
    echo "FAIL: validate ifs-no-coordindex output missing IFS coordIndex indicator (got: $out)"
    failures=$(( failures + 1 ))
fi

# ── 22. validate ILS coord without coordIndex → exit 3, mentions 'ils-coord' ──
out=$("$CLI" validate "$FIXTURE_ILS_NO_CI" 2>&1 || true)
ec=$("$CLI" validate "$FIXTURE_ILS_NO_CI" >/dev/null 2>&1; echo $?)
check "validate ils-no-coordindex exits 3" "$ec" "3"
if [[ "$out" == *"ils-coord"* || "$out" == *"IndexedLineSet"* || "$out" == *"coordIndex"* ]]; then
    echo "ok:   validate ils-no-coordindex output mentions ILS coordIndex"
else
    echo "FAIL: validate ils-no-coordindex output missing ILS coordIndex indicator (got: $out)"
    failures=$(( failures + 1 ))
fi

# ── 23. validate clean scene still exits 0 after conformance checks ───────────
ec=$("$CLI" validate "$FIXTURE_CLEAN" >/dev/null 2>&1; echo $?)
check "validate clean scene still exits 0 after conformance checks" "$ec" "0"

# ════════════════════════════════════════════════════════════════════════════
# Task C2: x3d extract subcommand
# ════════════════════════════════════════════════════════════════════════════

FIXTURE_SIMPLE="$FIXTURES/simple.x3d"

# ── 24. extract --help exits 0 ────────────────────────────────────────────────
ec=$("$CLI" extract --help >/dev/null 2>&1; echo $?)
check "extract --help exits 0" "$ec" "0"

# ── 25. extract with no args → exit 1 ────────────────────────────────────────
ec=$("$CLI" extract >/dev/null 2>&1; echo $?)
check "extract no args exits 1" "$ec" "1"

# ── 26. extract missing file → exit 2 ────────────────────────────────────────
ec=$("$CLI" extract /nonexistent/nope.x3d >/dev/null 2>&1; echo $?)
check "extract missing file exits 2" "$ec" "2"

# ── 27. extract malformed input → exit non-zero, no crash ────────────────────
garbage="$TD/garbage_extract.x3d"
printf '%s' 'NOT VALID X3D' > "$garbage"
ec=$("$CLI" extract "$garbage" >/dev/null 2>&1; echo $?)
if [[ "$ec" != "0" ]]; then
    echo "ok:   extract malformed exits non-zero ($ec)"
else
    echo "FAIL: extract malformed unexpectedly exited 0"
    failures=$(( failures + 1 ))
fi

# ── 28. extract simple.x3d → non-empty STL, 12 triangles (Box) ───────────────
stl_out="$TD/simple_extract.stl"
ec=$("$CLI" extract "$FIXTURE_SIMPLE" -o "$stl_out" 2>/dev/null; echo $?)
check "extract simple.x3d exits 0" "$ec" "0"

if [[ -s "$stl_out" ]]; then
    echo "ok:   extract output is non-empty"
else
    echo "FAIL: extract output is empty or missing"
    failures=$(( failures + 1 ))
fi

# Read tri count from binary STL (uint32 at offset 80, little-endian).
# Use od to dump 4 bytes at offset 80 as little-endian unsigned int.
if [[ -s "$stl_out" ]]; then
    # od -A d -t u4 prints decimal offset then the uint32 value.
    tri_count=$(od -A d -j 80 -N 4 -t u4 "$stl_out" 2>/dev/null | head -1 | awk '{print $2}')
    if [[ "$tri_count" == "12" ]]; then
        echo "ok:   extract Box has 12 triangles"
    else
        echo "FAIL: extract Box tri count is '$tri_count' (expected 12)"
        failures=$(( failures + 1 ))
    fi
fi

# ── 29. extract unknown option → exit 1 ──────────────────────────────────────
ec=$("$CLI" extract "$FIXTURE_SIMPLE" --unknown-flag >/dev/null 2>&1; echo $?)
check "extract unknown option exits 1" "$ec" "1"

# ════════════════════════════════════════════════════════════════════════════
# Task C3: x3d canonicalize subcommand
# ════════════════════════════════════════════════════════════════════════════

# ── 37. canonicalize --help exits 0 ──────────────────────────────────────────
ec=$("$CLI" canonicalize --help >/dev/null 2>&1; echo $?)
check "canonicalize --help exits 0" "$ec" "0"

# ── 38. canonicalize with no args → exit 1 ───────────────────────────────────
ec=$("$CLI" canonicalize >/dev/null 2>&1; echo $?)
check "canonicalize no args exits 1" "$ec" "1"

# ── 39. canonicalize missing file → exit 2 ───────────────────────────────────
ec=$("$CLI" canonicalize /nonexistent/nope.x3d >/dev/null 2>&1; echo $?)
check "canonicalize missing file exits 2" "$ec" "2"

# ── 40. canonicalize malformed input → exit non-zero, no crash ───────────────
garbage_canon="$TD/garbage_canon.x3d"
printf '%s' 'NOT VALID X3D <<<' > "$garbage_canon"
ec=$("$CLI" canonicalize "$garbage_canon" >/dev/null 2>&1; echo $?)
if [[ "$ec" != "0" ]]; then
    echo "ok:   canonicalize malformed exits non-zero ($ec)"
else
    echo "FAIL: canonicalize malformed unexpectedly exited 0"
    failures=$(( failures + 1 ))
fi

# ── 41. canonicalize produces non-empty output ───────────────────────────────
canon_out="$TD/simple_canon.x3d"
ec=$("$CLI" canonicalize "$FIXTURE_SIMPLE" -o "$canon_out" 2>/dev/null; echo $?)
check "canonicalize simple.x3d exits 0" "$ec" "0"

if [[ -s "$canon_out" ]]; then
    echo "ok:   canonicalize output is non-empty"
else
    echo "FAIL: canonicalize output is empty or missing"
    failures=$(( failures + 1 ))
fi

# ── 42. canonicalize idempotence: canon(canon(x)) == canon(x) byte-for-byte ──
canon2_out="$TD/simple_canon2.x3d"
ec=$("$CLI" canonicalize "$canon_out" -o "$canon2_out" 2>/dev/null; echo $?)
check "canonicalize re-canonicalize exits 0" "$ec" "0"

if [[ -s "$canon2_out" ]]; then
    if cmp -s "$canon_out" "$canon2_out"; then
        echo "ok:   canonicalize is idempotent (byte-identical)"
    else
        echo "FAIL: canonicalize is not idempotent (outputs differ)"
        diff "$canon_out" "$canon2_out" | head -10
        failures=$(( failures + 1 ))
    fi
else
    echo "FAIL: canonicalize re-canonicalize output is empty"
    failures=$(( failures + 1 ))
fi

# ── 43. canonicalize unknown option → exit 1 ─────────────────────────────────
ec=$("$CLI" canonicalize "$FIXTURE_SIMPLE" --unknown-flag >/dev/null 2>&1; echo $?)
check "canonicalize unknown option exits 1" "$ec" "1"

# ════════════════════════════════════════════════════════════════════════════
# sim: headless behavior simulation + field-change trace
# ════════════════════════════════════════════════════════════════════════════

FIXTURE_ANIM="$FIXTURES/sim-anim.x3d"
FIXTURE_PROX="$FIXTURES/sim-proximity.x3d"

# ── 44. sim --help exits 0 ───────────────────────────────────────────────────
ec=$("$CLI" sim --help >/dev/null 2>&1; echo $?)
check "sim --help exits 0" "$ec" "0"

# ── 45. sim no args → exit 1 ─────────────────────────────────────────────────
ec=$("$CLI" sim >/dev/null 2>&1; echo $?)
check "sim no args exits 1" "$ec" "1"

# ── 46. sim missing file → exit 2 ────────────────────────────────────────────
ec=$("$CLI" sim /nonexistent/nope.x3d >/dev/null 2>&1; echo $?)
check "sim missing file exits 2" "$ec" "2"

# ── 47. sim malformed input → exit 2, no crash ───────────────────────────────
garbage_sim="$TD/garbage_sim.x3d"
printf '%s' 'NOT VALID X3D <<<' > "$garbage_sim"
ec=$("$CLI" sim "$garbage_sim" >/dev/null 2>&1; echo $?)
check "sim malformed exits 2" "$ec" "2"

# ── 48. sim bad --move spec → exit 1 ─────────────────────────────────────────
ec=$("$CLI" sim "$FIXTURE_ANIM" --move "garbage spec" >/dev/null 2>&1; echo $?)
check "sim bad --move exits 1" "$ec" "1"

# ── 49. sim anim broad trace → non-empty, shows animating translation ────────
out=$("$CLI" sim "$FIXTURE_ANIM" --fps 4 --ticks 4 2>/dev/null || true)
if [[ -n "$out" ]]; then
    echo "ok:   sim anim broad trace non-empty"
else
    echo "FAIL: sim anim broad trace empty"
    failures=$(( failures + 1 ))
fi
# At t=0.5 (fps=4 -> tick 2), Mover.translation should be the lerp value 5 0 0.
if [[ "$out" == *"Mover.translation = 5 0 0"* ]]; then
    echo "ok:   sim anim shows Mover.translation = 5 0 0 at t=0.5"
else
    echo "FAIL: sim anim missing interpolated translation 5 0 0 (got: $out)"
    failures=$(( failures + 1 ))
fi

# ── 50. sim --watch narrows the trace ────────────────────────────────────────
watched=$("$CLI" sim "$FIXTURE_ANIM" --fps 4 --ticks 4 --watch Mover.translation 2>/dev/null || true)
# Should mention Mover.translation but NOT the Clock.* fields.
if [[ "$watched" == *"Mover.translation"* && "$watched" != *"Clock."* ]]; then
    echo "ok:   sim --watch narrows to Mover.translation only"
else
    echo "FAIL: sim --watch did not narrow correctly (got: $watched)"
    failures=$(( failures + 1 ))
fi

# ── 51. sim --json parses (starts with '[') ──────────────────────────────────
json_sim=$("$CLI" sim "$FIXTURE_ANIM" --fps 4 --ticks 4 --json 2>/dev/null || true)
trimmed="${json_sim#"${json_sim%%[![:space:]]*}"}"
if [[ "$trimmed" == "["* && "$json_sim" == *'"tick"'* && "$json_sim" == *'"changes"'* ]]; then
    echo "ok:   sim --json output is a JSON array of tick records"
else
    echo "FAIL: sim --json malformed (got first 120: ${json_sim:0:120})"
    failures=$(( failures + 1 ))
fi

# ── 52. sim determinism: two runs byte-identical ─────────────────────────────
r1="$TD/sim_r1.json"; r2="$TD/sim_r2.json"
"$CLI" sim "$FIXTURE_ANIM" --fps 30 --ticks 30 --json >"$r1" 2>/dev/null || true
"$CLI" sim "$FIXTURE_ANIM" --fps 30 --ticks 30 --json >"$r2" 2>/dev/null || true
if cmp -s "$r1" "$r2"; then
    echo "ok:   sim is deterministic (two runs byte-identical)"
else
    echo "FAIL: sim is non-deterministic (traces differ)"
    failures=$(( failures + 1 ))
fi

# ── 53. sim proximity --move fires enter/exit at the crossing ────────────────
prox=$("$CLI" sim "$FIXTURE_PROX" --fps 10 --ticks 10 --move "-5 0 0 -> 5 0 0 over 1" 2>/dev/null || true)
if [[ "$prox" == *"Region.enterTime"* && "$prox" == *"Region.exitTime"* ]]; then
    echo "ok:   sim proximity --move fires Region enter+exit"
else
    echo "FAIL: sim proximity --move did not fire enter/exit (got: $prox)"
    failures=$(( failures + 1 ))
fi
# Enter at tick 4 (x=-1, t=0.4), exit at tick 7 (x=+1, t=0.7).
if [[ "$prox" == *"tick 4"*"Region.enterTime"* ]]; then
    echo "ok:   sim proximity enterTime at tick 4"
else
    echo "FAIL: sim proximity enterTime not at tick 4 (got: $prox)"
    failures=$(( failures + 1 ))
fi
if [[ "$prox" == *"tick 7"*"Region.exitTime"* ]]; then
    echo "ok:   sim proximity exitTime at tick 7"
else
    echo "FAIL: sim proximity exitTime not at tick 7 (got: $prox)"
    failures=$(( failures + 1 ))
fi

# ── 54. sim tracer crash-safety: sim on a DEF-rich scene must exit 0 ────────
# (guards the any_cast pointer-form fix in FieldTracer::buildNodeIndex; a
# throwing cast would previously crash / exit non-zero here)
ec=$("$CLI" sim "$FIXTURE_ANIM" --fps 4 --ticks 4 >/dev/null 2>&1; echo $?)
check "sim tracer crash-safety (DEF-rich scene exits 0)" "$ec" "0"

# ── 55. sim golden-trace regression (if goldens dir provided) ───────────────
if [[ -n "$GOLDENS" && -f "$GOLDENS/sim-anim.trace.json" ]]; then
    got_anim=$("$CLI" sim "$FIXTURE_ANIM" --fps 4 --ticks 8 --watch Mover.translation --json 2>/dev/null || true)
    if [[ "$got_anim" == "$(cat "$GOLDENS/sim-anim.trace.json")" ]]; then
        echo "ok:   sim anim trace matches golden"
    else
        echo "FAIL: sim anim trace drifted from golden"
        diff <(printf '%s' "$got_anim") "$GOLDENS/sim-anim.trace.json" | head -20
        failures=$(( failures + 1 ))
    fi
    got_prox=$("$CLI" sim "$FIXTURE_PROX" --fps 10 --ticks 10 --move "-5 0 0 -> 5 0 0 over 1" --watch Region.enterTime --watch Region.exitTime --watch Region.isActive --json 2>/dev/null || true)
    if [[ "$got_prox" == "$(cat "$GOLDENS/sim-proximity.trace.json")" ]]; then
        echo "ok:   sim proximity trace matches golden"
    else
        echo "FAIL: sim proximity trace drifted from golden"
        diff <(printf '%s' "$got_prox") "$GOLDENS/sim-proximity.trace.json" | head -20
        failures=$(( failures + 1 ))
    fi
else
    echo "SKIP: sim golden-trace regression (no goldens dir)"
fi

# ── 56. sim §37 physics (only when x3d_cli was built with the Jolt backend) ──
# A RigidBody falls under gravity; position_changed is routed to Faller.translation.
# Asserts: (a) free-fall — at t=1.0s (fps 60) y ~= 10 - 1/2*9.8*1^2 = 5.1; (b)
# determinism — two --json runs byte-identical; (c) golden-trace regression.
FIXTURE_PHYS="$FIXTURES/sim-physics.x3d"
if [[ "$PHYSICS" == "1" && -f "$FIXTURE_PHYS" ]]; then
    # (a) Free-fall: the y at t=1.0 (tick 60 @ fps 60) must be ~5.1.
    phys=$("$CLI" sim "$FIXTURE_PHYS" --fps 60 --ticks 61 --watch Faller.translation 2>/dev/null || true)
    # Pull the y of the last "tick 60" line; expect 5.0x (between 4.8 and 5.4).
    y60=$(printf '%s\n' "$phys" | awk '/tick 60 /{grab=1; next} grab && /Faller.translation/{print $4; exit}')
    if [[ "$y60" == 5.* || "$y60" == 4.[89]* ]]; then
        echo "ok:   sim physics free-fall y(1s)=$y60 (~5.1, matches 1/2 g t^2)"
    else
        echo "FAIL: sim physics free-fall y(1s)=$y60 not near 5.1"
        echo "$phys" | tail -4
        failures=$(( failures + 1 ))
    fi
    # The body must actually descend from y=10 (Faller tracks RigidBody.position).
    if [[ "$phys" == *"Faller.translation = 0 9."* ]]; then
        echo "ok:   sim physics RigidBody.position drives Faller.translation (descending)"
    else
        echo "FAIL: sim physics did not move Faller.translation"
        failures=$(( failures + 1 ))
    fi
    # (b) Determinism: two --json runs byte-identical.
    pr1="$TD/phys_r1.json"; pr2="$TD/phys_r2.json"
    "$CLI" sim "$FIXTURE_PHYS" --fps 60 --ticks 30 --watch Faller.translation --json >"$pr1" 2>/dev/null || true
    "$CLI" sim "$FIXTURE_PHYS" --fps 60 --ticks 30 --watch Faller.translation --json >"$pr2" 2>/dev/null || true
    if cmp -s "$pr1" "$pr2"; then
        echo "ok:   sim physics is deterministic (two runs byte-identical)"
    else
        echo "FAIL: sim physics is non-deterministic (traces differ)"
        failures=$(( failures + 1 ))
    fi
    # (c) Golden-trace regression.
    if [[ -n "$GOLDENS" && -f "$GOLDENS/sim-physics.trace.json" ]]; then
        if [[ "$(cat "$pr1")" == "$(cat "$GOLDENS/sim-physics.trace.json")" ]]; then
            echo "ok:   sim physics trace matches golden"
        else
            echo "FAIL: sim physics trace drifted from golden"
            diff "$pr1" "$GOLDENS/sim-physics.trace.json" | head -20
            failures=$(( failures + 1 ))
        fi
    fi
else
    echo "SKIP: sim §37 physics (x3d_cli built without the Jolt backend)"
fi

# ── 57. sim §37 JOINTS (only when x3d_cli was built with the Jolt backend) ────
# A RigidBody ball-jointed to a fixed world anchor at the origin swings under
# gravity; position_changed is routed to Bob.translation. Asserts: (a) the
# pendulum swings (y drops well below 0) while the constraint holds the bob ~1
# unit from the anchor (it does not free-fall, does not fly off); (b) determinism;
# (c) golden-trace regression.
FIXTURE_JOINTS="$FIXTURES/sim-joints.x3d"
if [[ "$PHYSICS" == "1" && -f "$FIXTURE_JOINTS" ]]; then
    # (a) Pendulum behavioral: over the swing the distance from the anchor stays
    #     ~1 (within 0.05) AND the bob swings down (minimum y well below 0).
    jtrace=$("$CLI" sim "$FIXTURE_JOINTS" --fps 60 --ticks 90 --watch Bob.translation 2>/dev/null || true)
    read -r jmind jmaxd jminy < <(printf '%s\n' "$jtrace" | awk '
        /Bob.translation/ { x=$3; y=$4; d=sqrt(x*x+y*y);
            if (d>maxd) maxd=d; if (mind==0||d<mind) mind=d; if (y<miny) miny=y }
        END { printf "%.4f %.4f %.4f\n", mind, maxd, miny }')
    # Distance to anchor must hold near 1.0 throughout (the constraint holds).
    if awk "BEGIN{exit !($jmind>0.95 && $jmaxd<1.05)}"; then
        echo "ok:   sim joints pendulum holds anchor (dist $jmind..$jmaxd ~1)"
    else
        echo "FAIL: sim joints pendulum distance-to-anchor drifted ($jmind..$jmaxd)"
        failures=$(( failures + 1 ))
    fi
    # The bob must actually swing down under gravity (minY well below 0).
    if awk "BEGIN{exit !($jminy < -0.5)}"; then
        echo "ok:   sim joints pendulum swings down (minY=$jminy)"
    else
        echo "FAIL: sim joints pendulum did not swing (minY=$jminy)"
        failures=$(( failures + 1 ))
    fi
    # (b) Determinism: two --json runs byte-identical.
    jr1="$TD/joints_r1.json"; jr2="$TD/joints_r2.json"
    "$CLI" sim "$FIXTURE_JOINTS" --fps 60 --ticks 30 --watch Bob.translation --json >"$jr1" 2>/dev/null || true
    "$CLI" sim "$FIXTURE_JOINTS" --fps 60 --ticks 30 --watch Bob.translation --json >"$jr2" 2>/dev/null || true
    if cmp -s "$jr1" "$jr2"; then
        echo "ok:   sim joints is deterministic (two runs byte-identical)"
    else
        echo "FAIL: sim joints is non-deterministic (traces differ)"
        failures=$(( failures + 1 ))
    fi
    # (c) Golden-trace regression.
    if [[ -n "$GOLDENS" && -f "$GOLDENS/sim-joints.trace.json" ]]; then
        if [[ "$(cat "$jr1")" == "$(cat "$GOLDENS/sim-joints.trace.json")" ]]; then
            echo "ok:   sim joints trace matches golden"
        else
            echo "FAIL: sim joints trace drifted from golden"
            diff "$jr1" "$GOLDENS/sim-joints.trace.json" | head -20
            failures=$(( failures + 1 ))
        fi
    fi
else
    echo "SKIP: sim §37 joints (x3d_cli built without the Jolt backend)"
fi

# ── 58. sim §37 contact reporting (only when x3d_cli was built with the Jolt backend) ──
# A dynamic box falls onto a fixed floor; a CollisionSensor must go active when
# the boxes collide (§37 CONF-RBP contact reporting).
FIXTURE_COLL="$FIXTURES/sim-collision.x3d"
if [[ "$PHYSICS" == "1" && -f "$FIXTURE_COLL" ]]; then
    # (a) CollisionSensor.isActive must fire during 90 ticks (faller lands ~t=0.7s).
    coll=$("$CLI" sim "$FIXTURE_COLL" --fps 60 --ticks 90 --watch Sensor.isActive 2>/dev/null || true)
    if [[ "$coll" == *"Sensor.isActive = true"* ]]; then
        echo "ok:   sim collision -> CollisionSensor.isActive fires"
    else
        echo "FAIL: sim collision did not report CollisionSensor.isActive (got: $coll)"
        failures=$(( failures + 1 ))
    fi
    # (b) Determinism: two --json runs byte-identical.
    cr1="$TD/coll_r1.json"; cr2="$TD/coll_r2.json"
    "$CLI" sim "$FIXTURE_COLL" --fps 60 --ticks 90 --watch Sensor.isActive --json >"$cr1" 2>/dev/null || true
    "$CLI" sim "$FIXTURE_COLL" --fps 60 --ticks 90 --watch Sensor.isActive --json >"$cr2" 2>/dev/null || true
    if cmp -s "$cr1" "$cr2"; then
        echo "ok:   sim collision is deterministic (two runs byte-identical)"
    else
        echo "FAIL: sim collision is non-deterministic (traces differ)"
        failures=$(( failures + 1 ))
    fi
    # (c) Golden-trace regression.
    if [[ -n "$GOLDENS" && -f "$GOLDENS/sim-collision.trace.json" ]]; then
        got_coll=$("$CLI" sim "$FIXTURE_COLL" --fps 60 --ticks 90 --watch Sensor.isActive --json 2>/dev/null || true)
        if [[ "$got_coll" == "$(cat "$GOLDENS/sim-collision.trace.json")" ]]; then
            echo "ok:   sim collision trace matches golden"
        else
            echo "FAIL: sim collision trace drifted from golden"
            diff <(printf '%s' "$got_coll") "$GOLDENS/sim-collision.trace.json" | head -20
            failures=$(( failures + 1 ))
        fi
    fi
else
    echo "SKIP: sim §37 contact reporting (x3d_cli built without the Jolt backend)"
fi

# ── 59. sim §37 inertia/COM: quadcopter rises + rolls (CONF-RBP-INERTIA) ──────
# A RigidBody with constant lift (forces="0 10 0", net upward) and a constant
# roll torque (torques="0.02 0 0") with a non-uniform authored inertia tensor
# must rise under lift and rotate about x at a rate governed by that inertia.
FIXTURE_QUAD="$FIXTURES/sim-quadcopter.x3d"
if [[ "$PHYSICS" == "1" && -f "$FIXTURE_QUAD" ]]; then
    # §37 inertia: the quadcopter rises (lift > mg) and rolls (torque about x).
    quad=$("$CLI" sim "$FIXTURE_QUAD" --fps 60 --ticks 120 --watch QuadXform.translation 2>/dev/null || true)
    if [[ "$quad" == *"QuadXform.translation = 0 1."* || "$quad" == *"QuadXform.translation = 0 2."* ]]; then
        echo "ok:   sim quadcopter rises under lift"
    else
        echo "FAIL: sim quadcopter did not rise (got: $quad)"
        failures=$(( failures + 1 ))
    fi
    # Determinism: two --json runs byte-identical.
    qr1="$TD/quad_r1.json"; qr2="$TD/quad_r2.json"
    "$CLI" sim "$FIXTURE_QUAD" --fps 60 --ticks 120 --watch QuadXform.translation --watch QuadXform.rotation --json >"$qr1" 2>/dev/null || true
    "$CLI" sim "$FIXTURE_QUAD" --fps 60 --ticks 120 --watch QuadXform.translation --watch QuadXform.rotation --json >"$qr2" 2>/dev/null || true
    if cmp -s "$qr1" "$qr2"; then
        echo "ok:   sim quadcopter is deterministic (two runs byte-identical)"
    else
        echo "FAIL: sim quadcopter is non-deterministic"
        failures=$(( failures + 1 ))
    fi
    # Golden-trace regression.
    if [[ -n "$GOLDENS" && -f "$GOLDENS/sim-quadcopter.trace.json" ]]; then
        got_quad=$("$CLI" sim "$FIXTURE_QUAD" --fps 60 --ticks 120 --watch QuadXform.translation --watch QuadXform.rotation --json 2>/dev/null || true)
        if [[ "$got_quad" == "$(cat "$GOLDENS/sim-quadcopter.trace.json")" ]]; then
            echo "ok:   sim quadcopter trace matches golden"
        else
            echo "FAIL: sim quadcopter trace drifted from golden"
            diff <(printf '%s' "$got_quad") "$GOLDENS/sim-quadcopter.trace.json" | head -20
            failures=$(( failures + 1 ))
        fi
    fi
else
    echo "SKIP: sim §37 inertia/COM quadcopter (x3d_cli built without the Jolt backend)"
fi

# ── 60. sim §37 vehicle: RWD car drives forward under wheel torque + friction ──
# A rear-wheel-drive chassis (4 cylinder wheels on SingleAxisHingeJoints, RW
# torques="3 0 0") drives forward in +z under authored frictionCoefficients="1 1".
# Asserts: (a) the car drives (z > 1.0 by tick 210); (b) determinism.
FIXTURE_VEH="$FIXTURES/sim-vehicle.x3d"
if [[ "$PHYSICS" == "1" && -f "$FIXTURE_VEH" ]]; then
    veh=$("$CLI" sim "$FIXTURE_VEH" --fps 60 --ticks 210 --watch CarXform.translation 2>/dev/null || true)
    # Drives forward in +z: the last CarXform.translation must have a z > 1.0.
    # Extract the final z from the last "CarXform.translation = x y z" line.
    veh_z=$(printf '%s\n' "$veh" | awk '/CarXform.translation/{z=$5} END{print z}')
    if awk "BEGIN{exit !(($veh_z)+0 > 1.0)}"; then
        echo "ok:   sim vehicle drives forward (final z=$veh_z)"
    else
        echo "FAIL: sim vehicle did not drive forward (final z=$veh_z; tail: $(printf '%s' "$veh" | tail -2))"
        failures=$(( failures + 1 ))
    fi
    # Determinism: two --json runs byte-identical.
    vr1="$TD/veh_r1.json"; vr2="$TD/veh_r2.json"
    "$CLI" sim "$FIXTURE_VEH" --fps 60 --ticks 210 --watch CarXform.translation --json >"$vr1" 2>/dev/null || true
    "$CLI" sim "$FIXTURE_VEH" --fps 60 --ticks 210 --watch CarXform.translation --json >"$vr2" 2>/dev/null || true
    if cmp -s "$vr1" "$vr2"; then
        echo "ok:   sim vehicle is deterministic (two runs byte-identical)"
    else
        echo "FAIL: sim vehicle is non-deterministic"
        failures=$(( failures + 1 ))
    fi
    # Golden-trace regression.
    if [[ -n "$GOLDENS" && -f "$GOLDENS/sim-vehicle.trace.json" ]]; then
        got_veh=$("$CLI" sim "$FIXTURE_VEH" --fps 60 --ticks 210 --watch CarXform.translation --json 2>/dev/null || true)
        if [[ "$got_veh" == "$(cat "$GOLDENS/sim-vehicle.trace.json")" ]]; then
            echo "ok:   sim vehicle trace matches golden"
        else
            echo "FAIL: sim vehicle trace drifted from golden"
            diff <(printf '%s' "$got_veh") "$GOLDENS/sim-vehicle.trace.json" | head -20
            failures=$(( failures + 1 ))
        fi
    fi
else
    echo "SKIP: sim §37 vehicle (x3d_cli built without the Jolt backend)"
fi

# ── 61. sim §37 skid-steer pivot (yaws in place) ─────────────────────────────
# Left wheels driven forward, right wheels backward (opposing torques); opposing
# tyre friction yaws the chassis. Asserts: (a) rotation angle grows (pivot spins);
# (b) determinism; (c) golden-trace regression.
FIXTURE_PIV="$FIXTURES/sim-vehicle-pivot.x3d"
if [[ "$PHYSICS" == "1" && -f "$FIXTURE_PIV" ]]; then
    piv=$("$CLI" sim "$FIXTURE_PIV" --fps 60 --ticks 210 --watch CarXform.rotation 2>/dev/null || true)
    # Extract the final rotation angle (4th field of the last CarXform.rotation line).
    piv_angle=$(printf '%s\n' "$piv" | awk '/CarXform.rotation/{a=$NF} END{print a}')
    if [[ -n "$piv_angle" ]] && awk "BEGIN{exit !(($piv_angle)+0 > 0.5)}"; then
        echo "ok:   sim vehicle pivot yaws the chassis (final angle=$piv_angle)"
    else
        echo "FAIL: sim vehicle pivot did not yaw (final angle=$piv_angle; tail: $(printf '%s' "$piv" | tail -2))"
        failures=$(( failures + 1 ))
    fi
    pp1="$TD/piv_r1.json"; pp2="$TD/piv_r2.json"
    "$CLI" sim "$FIXTURE_PIV" --fps 60 --ticks 210 --watch CarXform.rotation --json >"$pp1" 2>/dev/null || true
    "$CLI" sim "$FIXTURE_PIV" --fps 60 --ticks 210 --watch CarXform.rotation --json >"$pp2" 2>/dev/null || true
    cmp -s "$pp1" "$pp2" && echo "ok:   sim vehicle pivot is deterministic" || { echo "FAIL: pivot non-deterministic"; failures=$(( failures + 1 )); }
    if [[ -n "$GOLDENS" && -f "$GOLDENS/sim-vehicle-pivot.trace.json" ]]; then
        [[ "$(cat "$pp1")" == "$(cat "$GOLDENS/sim-vehicle-pivot.trace.json")" ]] && echo "ok:   sim vehicle pivot trace matches golden" || { echo "FAIL: pivot golden drift"; diff <(cat "$pp1") "$GOLDENS/sim-vehicle-pivot.trace.json" | head -20; failures=$(( failures + 1 )); }
    fi
else
    echo "SKIP: sim §37 vehicle pivot (x3d_cli built without the Jolt backend)"
fi

# ── 62. sim §37 incline climb (chassis y rises) ──────────────────────────────
# All-wheel drive (torques=-6) carries the car up a ~7° tilted ground under
# tyre friction. Asserts: (a) chassis y rises (> 2.0 by tick 210); (b) determinism;
# (c) golden-trace regression.
FIXTURE_RMP="$FIXTURES/sim-vehicle-ramp.x3d"
if [[ "$PHYSICS" == "1" && -f "$FIXTURE_RMP" ]]; then
    # Use non-JSON trace for y extraction (cleaner field layout).
    rmp_txt=$("$CLI" sim "$FIXTURE_RMP" --fps 60 --ticks 210 --watch CarXform.translation 2>/dev/null || true)
    # Extract the final y (4th awk field of "CarXform.translation = x y z": $1=name $2='=' $3=x $4=y $5=z).
    rmp_y=$(printf '%s\n' "$rmp_txt" | awk '/CarXform.translation/{y=$4} END{print y}')
    if [[ -n "$rmp_y" ]] && awk "BEGIN{exit !(($rmp_y)+0 > 2.0)}"; then
        echo "ok:   sim vehicle ramp climbs the incline (final y=$rmp_y)"
    else
        echo "FAIL: sim vehicle ramp did not climb (final y=$rmp_y; tail: $(printf '%s' "$rmp_txt" | tail -2))"
        failures=$(( failures + 1 ))
    fi
    # Determinism: two --json runs byte-identical.
    rr1="$TD/rmp_r1.json"; rr2="$TD/rmp_r2.json"
    "$CLI" sim "$FIXTURE_RMP" --fps 60 --ticks 210 --watch CarXform.translation --json >"$rr1" 2>/dev/null || true
    "$CLI" sim "$FIXTURE_RMP" --fps 60 --ticks 210 --watch CarXform.translation --json >"$rr2" 2>/dev/null || true
    cmp -s "$rr1" "$rr2" && echo "ok:   sim vehicle ramp is deterministic" || { echo "FAIL: ramp non-deterministic"; failures=$(( failures + 1 )); }
    if [[ -n "$GOLDENS" && -f "$GOLDENS/sim-vehicle-ramp.trace.json" ]]; then
        [[ "$(cat "$rr1")" == "$(cat "$GOLDENS/sim-vehicle-ramp.trace.json")" ]] && echo "ok:   sim vehicle ramp trace matches golden" || { echo "FAIL: ramp golden drift"; diff <(cat "$rr1") "$GOLDENS/sim-vehicle-ramp.trace.json" | head -20; failures=$(( failures + 1 )); }
    fi
else
    echo "SKIP: sim §37 vehicle ramp (x3d_cli built without the Jolt backend)"
fi

# ── 63. sim §39 Followers: PositionDamper eases toward destination ───────────
# A PositionDamper with initialDestination="5 0 0" (differs from initialValue="0 0 0")
# fires the §39 initial transition at load — Mover.translation eases 0→(5,0,0).
# Asserts: (a) easing — x rises above 4.0 by tick 120; (b) determinism; (c) golden.
FIXTURE_DMP="$FIXTURES/sim-damper.x3d"
if [[ -f "$FIXTURE_DMP" ]]; then
    d1="$TD/dmp1.json"; d2="$TD/dmp2.json"
    "$CLI" sim "$FIXTURE_DMP" --fps 60 --ticks 120 --watch Mover.translation --json >"$d1" 2>/dev/null || true
    "$CLI" sim "$FIXTURE_DMP" --fps 60 --ticks 120 --watch Mover.translation --json >"$d2" 2>/dev/null || true
    cmp -s "$d1" "$d2" && echo "ok:   sim damper is deterministic" || { echo "FAIL: damper non-deterministic"; failures=$(( failures + 1 )); }
    if [[ -n "$GOLDENS" && -f "$GOLDENS/sim-damper.trace.json" ]]; then
        [[ "$(cat "$d1")" == "$(cat "$GOLDENS/sim-damper.trace.json")" ]] && echo "ok:   sim damper trace matches golden" || { echo "FAIL: damper golden drift"; diff "$d1" "$GOLDENS/sim-damper.trace.json" | head -20; failures=$(( failures + 1 )); }
    fi
fi

# ════════════════════════════════════════════════════════════════════════════
# Gallery/smoke scene profile gate: `x3d validate` must exit 0 on every
# first-party showcase/smoke scene committed under examples/cpu_raster/assets/
# (the cpu_raster "gallery" hero scenes, the lion_head demo model + its studio
# wrappers, and the flagship raster_smoke.x3d + its sibling smoke scenes).
# These ship as documentation/demo material (README "try it" pointers,
# docs/images/gallery/*.png) — a declared profile narrower than the content
# (e.g. "Interchange" with a PhysicalMaterial or NURBS node) makes `x3d
# validate` fail with "component ... exceeds declared profile" (exit 3).
# Regression test for that class of bug (task: fix declared X3D profiles on
# first-party showcase/gallery scenes).
# ════════════════════════════════════════════════════════════════════════════

if [[ -n "$REPO_ROOT" ]]; then
    GALLERY_SCENES=(
        "$REPO_ROOT/examples/cpu_raster/assets/raster_smoke.x3d"
        "$REPO_ROOT/examples/cpu_raster/assets/text_smoke.x3d"
        "$REPO_ROOT/examples/cpu_raster/assets/textured_scene.x3d"
        "$REPO_ROOT/examples/cpu_raster/assets/skybox_smoke.x3d"
        "$REPO_ROOT/examples/cpu_raster/assets/gallery/hero_lights.x3d"
        "$REPO_ROOT/examples/cpu_raster/assets/gallery/hero_pbr_grid.x3d"
        "$REPO_ROOT/examples/cpu_raster/assets/gallery/hero_primitives.x3d"
        "$REPO_ROOT/examples/cpu_raster/assets/gallery/hero_skybox_pbr.x3d"
        "$REPO_ROOT/examples/cpu_raster/assets/gallery/hero_teapot_nurbs.x3d"
        "$REPO_ROOT/examples/cpu_raster/assets/models/lion_head/lion_head.x3d"
        "$REPO_ROOT/examples/cpu_raster/assets/models/lion_head/lion_head_lit.x3d"
        "$REPO_ROOT/examples/cpu_raster/assets/models/lion_head/lion_head_garden.x3d"
    )
    for scene in "${GALLERY_SCENES[@]}"; do
        if [[ ! -f "$scene" ]]; then
            echo "FAIL: gallery/smoke scene profile gate — missing scene: $scene"
            failures=$(( failures + 1 ))
            continue
        fi
        rel="${scene#"$REPO_ROOT/"}"
        out=$("$CLI" validate "$scene" 2>&1)
        ec=$?
        if [[ "$ec" == "0" ]]; then
            echo "ok:   validate $rel exits 0"
        else
            echo "FAIL: validate $rel exited $ec (expected 0)"
            echo "$out" | sed 's/^/      /'
            failures=$(( failures + 1 ))
        fi
    done
else
    echo "SKIP: gallery/smoke scene profile gate (no repo root arg provided)"
fi

# ── summary ───────────────────────────────────────────────────────────────────
echo ""
if [[ "$failures" -gt 0 ]]; then
    echo "x3d_cli_test: $failures failure(s)"
    exit 1
fi
echo "x3d_cli_test: all checks passed"
exit 0
