# X3DJSAIL Validate Check Categories

Triage worklist for the `validate` differential against X3DJSAIL, organized by
**disagreement category** — the taxonomy the live gate emits, so improvements are
tracked per category rather than by the headline agreement percentage alone.

- **Source of truth:** the live gate (`x3d_cli_gate`) run over the XML-only
  (`*.x3d`) golden subset (200 files of the 204 in `subset.txt`, stride-80 sample).
  Regenerate with `mise run cli-gate` (informative) or refresh the enforced floor
  with `mise run cli-gate-baseline`.
- **Baseline floor:** `cli-gate-baseline.tsv` — currently **40/200 agree** (20%),
  i.e. 160 disagreements. Every PASS row is locked by `mise run cli-gate-regression`.
- **Adjusted agreement:** `divergence-report.md` also reports agreement over the
  files where both validators rendered a comparable verdict, excluding the two
  oracle-behavior categories (`jsail-error`, `jsail-crash`) — currently **40/48**
  (83%). The floor and the regression gate still use the raw number.
- **Per-file findings:** `validate-verdicts.tsv` (X3DJSAIL's verdicts) and
  `divergence-report.md` (full per-file disagreement list).

Category names below are the exact strings from `classifyDisagreement()` in
`tools/x3d-cli/cli_gate.cpp`.

## Disagreement Categories (live gate, 160 total)

| Count | Category | Direction | Dominant pattern | Disposition |
|---|---|---|---|---|
| 114 | `jsail-error` | JSAIL `INVALID` / us `VALID` | `ERROR_NODE_NOT_FOUND`: X3D has no `<head>`/`<meta>` | **SKIPPED** — JSAIL best-practice, not required by the X3D spec |
| 38 | `jsail-crash` | JSAIL `INVALID` / us `VALID` | `Exception in thread "main"`: `containerField='rootNode'` aborts the JSAIL CLI | **SKIPPED** — no verdict was produced; pre-4.0 pattern, valid in 3.x |
| 5 | `invalid-only-jsail` | JSAIL `INVALID` / us `VALID` | `InvalidFieldException`: coord without `coordIndex` on pre-4.0 files | **SKIPPED** — our `coord-index` check is scoped to X3D 4.0+ |
| 2 | `inline-warn-us` | us `INVALID` / JSAIL `VALID` | `inline:` unresolvable `Inline`/`ImageTexture` `url` | **KNOWN** — our stricter reference check; JSAIL does not chase inlines |
| 1 | `proto-warn-us` | us `INVALID` / JSAIL `VALID` | `proto:` EXTERNPROTO shadows a built-in node type | **OPEN** — not yet triaged |

These five sum to the 160 disagreements behind the `40/200` floor. Track progress by
moving a file **out** of a category (into agreement), and record the count change here.

### `jsail-error` (114 files) — SKIPPED

X3DJSAIL raises `ERROR_NODE_NOT_FOUND` when a scene has no `<head>` or no `<meta>`
elements ("… is undescribed"). The X3D specification does not require `<head>` or
`<meta>` — they are recommended best-practice metadata. Implementing this would flag
a large number of conforming (but sparsely documented) scenes as invalid, producing
unacceptable noise. Skipped. (This is the former `jsail-no-head-meta` bucket; the
count moved from 118 to 114 because the live gate scores the 200-file subset, not all
204 verdict rows.)

### `jsail-crash` (38 files) — SKIPPED

The X3DJSAIL CLI dies with an uncaught `InvalidFieldValueException` on
`containerField='rootNode'` on child nodes of `GeoLOD`, which was valid in X3D 3.x
(where `rootNode` was a declared field slot in the XML encoding). The golden capture
recorded the abort as `INVALID`, but JSAIL produced no verdict. Our parser tolerates
the pattern correctly; this is a backward-compatibility regression in the validator,
not a spec violation by the files. The classifier keys on `Exception in thread` in the
golden note. Skipped. (These files were counted under `invalid-only-jsail` until the
classifier split them out.)

### `invalid-only-jsail` (5 files) — SKIPPED

Field-level rejections we intentionally do not mirror — mostly
`IndexedFaceSet`/`IndexedLineSet` coord-without-`coordIndex` on **pre-4.0** files,
which our `coord-index` check scopes to X3D 4.0+ (empty `coordIndex` means "no faces"
in 3.x). Skipped.

### `inline-warn-us` (2 files) — KNOWN, ours stricter

We report `INVALID` when an `Inline`/`ImageTexture` `url` cannot be resolved; X3DJSAIL
does not follow inline references during `-validate`, so it reports `VALID`.

- `TopEdge.x3d` — `inline: ../../X3dForWebAuthors/Chapter03Grouping/CoordinateAxes.x3d`
- `TinkercadX3dExtrudedTextCreationGLTF.x3d` — `inline: TinkercadX3dExtrudedTextCreation.glb`

### `proto-warn-us` (1 file) — OPEN

`Geometry2dComponentExternProtos.x3d` — we report `proto: Arc2D: EXTERNPROTO 'Arc2D'
shadows a built-in node type; declaration ignored`; X3DJSAIL reports `VALID`. Not yet
triaged.

### `range-only-us` — retired

`ConformanceNist/.../DirectionalLight/test_intensitysim.x3d` was falsely flagged when a
float32 boundary value (`1.570796`, the UOM truncation of π/2) landed epsilon above a
double-precision bound literal. ADR-0003's 2026-07-18 refinement compares bounds in the
field's own float32 precision, and the live gate no longer emits this category. See
`docs/wiki/decisions/0003-throw-on-range.md`.

## Improvements Landed (former disagreements now AGREE)

These X3DJSAIL findings used to appear as `invalid-only-jsail` disagreements. We now
implement the matching check, so those files **agree** with X3DJSAIL and no longer
count against the floor. This is what "track improvements by category" looks like: the
work moved rows out of `invalid-only-jsail`, not the headline percentage.

| Check | Category ID | JSAIL message pattern | Fixture |
|---|---|---|---|
| Duplicate `<meta>` | `dup-meta` | `WARNING_MESSAGE: duplicate statement found` | `tools/x3d-cli/fixtures/validate-dup-meta.x3d` |
| Unused (Extern)ProtoDeclare | `unused-proto` | `WARNING_PROTOINSTANCE_NOT_FOUND … has no corresponding ProtoInstance` | `tools/x3d-cli/fixtures/validate-unused-proto.x3d` |
| IFS coord without coordIndex | `ifs-coord-index` | `InvalidFieldException: IndexedFaceSet containing Coordinate node …` | `tools/x3d-cli/fixtures/validate-ifs-no-coordindex.x3d` |
| ILS coord without coordIndex | `ils-coord-index` | `InvalidFieldException: IndexedLineSet containing Coordinate node …` | `tools/x3d-cli/fixtures/validate-ils-no-coordindex.x3d` |

## Implemented Checks (in `cmdValidate` / `cli_detail::collectDiagnostics`, `validate.hpp`)

### Check 1: Duplicate `<meta>` statements (`dup-meta`)
- **Trigger**: Two `<meta>` elements with identical `name` AND `content` in `<head>`.
- **Severity**: warning
- **Category**: `dup-meta`
- **Scope**: All X3D versions.
- **Fixture**: `tools/x3d-cli/fixtures/validate-dup-meta.x3d`

### Check 2: Unused ProtoDeclare / ExternProtoDeclare (`unused-proto`)
- **Trigger**: A `ProtoDeclare` or `ExternProtoDeclare` has no corresponding
  `ProtoInstance` in the scene graph.
- **Severity**: warning
- **Category**: `unused-proto`
- **Scope**: All X3D versions.
- **Fixture**: `tools/x3d-cli/fixtures/validate-unused-proto.x3d`
- **Note**: Covers both `protoDeclarations` and `externProtoDeclarations`.

### Check 3: IndexedFaceSet coord without coordIndex (`ifs-coord-index`)
- **Trigger**: An `IndexedFaceSet` has a `Coordinate` child node with non-empty
  `point` data, but `coordIndex` is empty.
- **Severity**: error
- **Category**: `ifs-coord-index`
- **Scope**: X3D 4.0+ only (pre-4.0 files: empty coordIndex means "no faces" = valid).
- **Fixture**: `tools/x3d-cli/fixtures/validate-ifs-no-coordindex.x3d`

### Check 4: IndexedLineSet coord without coordIndex (`ils-coord-index`)
- **Trigger**: An `IndexedLineSet` has a `Coordinate` child node with non-empty
  `point` data, but `coordIndex` is empty.
- **Severity**: error
- **Category**: `ils-coord-index`
- **Scope**: X3D 4.0+ only (same rationale as Check 3).
- **Fixture**: `tools/x3d-cli/fixtures/validate-ils-no-coordindex.x3d`

## Refreshing this doc

1. `mise run cli-gate <corpus>` — regenerates `divergence-report.md` and prints the
   `Disagreement categories:` breakdown to stdout.
2. Copy those category counts into the table above; update the `40/200` floor if a
   `mise run cli-gate-baseline` refresh changed it.
3. Note any category that reached `0` under "Improvements Landed".
