# X3DJSAIL Validate Check Categories

Triage of every disagreement class between `x3d validate` and the frozen
X3DJSAIL oracle capture (`validate-verdicts.tsv`, 204 files, stride-80 sample —
4 of which have since been removed upstream, so live runs see 200). Category
names below match the `classifyDisagreement` labels in `cli_gate.cpp` and the
generated `divergence-report.md` exactly.

**How the oracle was captured** (`gen_golden.sh`): X3DJSAIL exit codes are
unreliable, so a file is VALID iff the output contains `validate results:
success` — **any** warning, best-practice nag, or Java crash was frozen as an
INVALID verdict. That capture rule, not validator quality, drives the raw
agreement number; the report's *Adjusted Agreement* section excludes the two
documented-oracle-behavior categories below.

Refreshed 2026-07-18 from a live gate run (fresh `corpus-fetch`). Raw
agreement 41/200; adjusted agreement **41/48 comparable files (85%)**.
Agreements include 3 both-INVALID matches (dup-meta on
`CadDesignPatternExampleBushing`, unused-proto on `ArtDecoPrototypesExcerpt`,
one X3D-4.0 coordIndex case).

## Disagreement categories (live counts, ranked)

| Count | Category | What it is | Ruling |
|---|---|---|---|
| 114 | `jsail-error` | `ERROR_NODE_NOT_FOUND: X3D head has no meta elements / has no head element` | **Documented oracle behavior — excluded from adjusted agreement.** A JSAIL best-practice documentation rule; the X3D spec makes `<head>`/`<meta>` optional. Implementing it would flag conforming scenes wholesale. (Was 118; 4 `HumanoidAnimation/Skeleton` files vanished upstream.) |
| 38 | `jsail-crash` | `Exception in thread "main" ... InvalidFieldValueException` on `containerField='rootNode'` (GeoLOD tiles, all Savage/Locations) | **Documented oracle behavior — excluded.** The JSAIL CLI *aborted*; no verdict was produced. The pattern is valid X3D 3.x; a JSAIL back-compat regression. |
| 5 | `invalid-only-jsail` | Remaining JSAIL-INVALID/ours-VALID | Three sub-cases, itemized below. |
| 2 | `inline-warn-us` | Ours INVALID: Inline target missing | **Offline-mirror artifacts**, itemized below. |
| 0 | `range-only-us` | Ours INVALID on a range bound | **Was 1 — a real bug on our side, fixed** (float32-boundary comparison, PR #95): `beamWidth='1.570796'` (exactly the UOM's π/2 truncation) was flagged as above-maximum because the generated check compared the float32 value against a wider double literal. |

### `invalid-only-jsail` — the 5 files

- **2 × pre-4.0 coordIndex** (`BonesRightFoot.x3d` v3.3, `proxynochildren.x3d`
  v3.0): JSAIL flags IFS/ILS-with-Coordinate-but-no-coordIndex at every
  version; our `ifs/ils-coord-index` check is **deliberately X3D 4.0+ scoped**
  (pre-4.0, empty coordIndex means "no faces" and is valid; the parser can't
  distinguish absent from empty). Intentional divergence.
- **2 × spurious unused-ExternProtoDeclare** (`SummerExamples.x3d`,
  `NetworkedTexture…`): verified by diffing declare-names against
  instance-names in the document — the sets are **identical**; every declare
  is instantiated. JSAIL's `WARNING_PROTOINSTANCE_NOT_FOUND` there is a false
  positive (plausibly cascading from its containerField attachment handling).
  Our graph-based check agrees with the document.
- **1 × capture noise**: the TSV note is a stray `XNDLoaderDOM`
  java.util.logging line, not a finding.

### `inline-warn-us` — the 2 files

Our validator flags an Inline whose target is missing **locally**; JSAIL never
resolves Inline URLs. Both are artifacts of validating a partial offline
mirror, not document defects:

- **`TinkercadX3dExtrudedTextCreationGLTF.x3d`** references a `.glb` —
  `fetch_corpus.sh`'s dependency closure only follows scene-file extensions
  (`.x3d/.x3dv/.wrl/.json`), so the binary is never mirrored.
- **`TopEdge.x3d`**'s *relative* URL is broken upstream (`../../` resolves
  inside `Savage/`, the target lives three levels up) but its absolute
  web3d.org fallback URLs work — X3D `url` fields are ordered fallback lists,
  and an offline validator cannot try the remote entries.

Open policy question (not a bug): whether unretrievable-inline should affect
document *validity* at all — retrievability is an environment property.

## Implemented checks (in `cmdValidate` / mirrored by `cli_gate`)

Unchanged from the original triage; all four verified live this refresh:

1. **`dup-meta`** — duplicate `<meta name= content=>` pair in `<head>`
   (warning; all versions; fixture `validate-dup-meta.x3d`). Agrees with JSAIL
   on `CadDesignPatternExampleBushing.x3d`.
2. **`unused-proto`** — `ProtoDeclare`/`ExternProtoDeclare` with no
   `ProtoInstance` (warning; all versions; fixture
   `validate-unused-proto.x3d`). Agrees with JSAIL on
   `ArtDecoPrototypesExcerpt.x3d`; see above for the two files where JSAIL
   fires falsely and we correctly do not.
3. **`ifs-coord-index`** — IFS with non-empty `Coordinate` but empty
   `coordIndex` (error; **X3D 4.0+ only**; fixture
   `validate-ifs-no-coordindex.x3d`).
4. **`ils-coord-index`** — same for IndexedLineSet (error; X3D 4.0+ only;
   fixture `validate-ils-no-coordindex.x3d`).

Plus the generated per-field **range diagnostics** (ADR-0003), whose float32
boundary semantics are pinned by ADR-0003's 2026-07-18 note and
`range_warnings_test`.

## Maintenance

- Regenerate the oracle: `mise run cli-golden-gen` (JDK + X3DJSAIL.jar; see
  `gen_golden.sh`). Re-capture is the only way category counts change on the
  JSAIL side; upstream corpus drift changes membership (run a fresh
  `mise run corpus-fetch` first — see the 2026-07-18 baseline-refresh PR #94
  for why).
- This doc must match `classifyDisagreement` in `cli_gate.cpp`; if you add or
  rename a category there, update the table here in the same diff.
