# X3DJSAIL Validate Check Categories

Generated from the XML-only (`*.x3d`) golden subset (204 files, stride-80 sample).
See `validate-verdicts.tsv` for per-file findings and `divergence-report.md` for
current agreement rates.

## Check Categories Found (ranked by frequency)

| Rank | Count | Category ID | X3DJSAIL Message Pattern | Status |
|---|---|---|---|---|
| 1 | 118 | `jsail-no-head-meta` | `ERROR_NODE_NOT_FOUND: X3D has no head element / head has no meta elements` | SKIPPED — JSAIL-specific best-practice, not required by X3D spec |
| 2 | 38 | `containerfield-invalid` | `InvalidFieldValueException: Found invalid value, containerField='rootNode'` | SKIPPED — JSAIL rejects old-style `containerField` for GeoLOD children; pre-4.0 pattern, valid in 3.x |
| 3 | 3 | `unused-proto` | `WARNING_PROTOINSTANCE_NOT_FOUND, (Extern)ProtoDeclare X has no corresponding ProtoInstance` | **IMPLEMENTED** — `unused-proto` diagnostic in `cmdValidate` |
| 4 | 2 | `ifs-coord-no-index` | `InvalidFieldException: IndexedFaceSet containing Coordinate node with N values must also include coordIndex field` | **IMPLEMENTED** (X3D 4.0+ only) — `ifs-coord-index` diagnostic |
| 5 | 1 | `dup-meta` | `WARNING_MESSAGE: duplicate statement found: <meta name='X' content='Y'/>` | **IMPLEMENTED** — `dup-meta` diagnostic |
| 6 | 1 | `ils-coord-no-index` | `InvalidFieldException: IndexedLineSet containing Coordinate node with N values must also include coordIndex field` | **IMPLEMENTED** (X3D 4.0+ only) — `ils-coord-index` diagnostic |

## Implemented Checks (in `cmdValidate` / `conformance_checks::runAll`)

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

## Skipped Checks (and why)

### `jsail-no-head-meta` (118 files)
X3DJSAIL requires a `<head>` with `<meta>` elements and reports an error when
absent. The X3D specification does not require `<head>` or `<meta>` — they are
recommended best-practice metadata. Implementing this would flag a large number
of conforming (but sparsely documented) scenes as invalid, producing unacceptable
noise. Skipped.

### `containerfield-invalid` (38 files)
X3DJSAIL rejects the `containerField='rootNode'` attribute on child nodes of
`GeoLOD`, which was valid in X3D 3.x (where `rootNode` was a declared field slot
in the XML encoding). Our parser tolerates this correctly. The JSAIL error is a
backward-compatibility regression in the validator, not a spec violation by the
files. Skipped.
