---
title: Conformance Infrastructure
summary: Conformance validator, prose-anchor manifests, fidelity metrics, corpus audit/sweep, and the conformance view generator.
tags: [subsystem, conformance, validator, corpus-audit, conformance-view]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/cli-suite.md
---

# Conformance Infrastructure

## Purpose

The conformance infrastructure joins structural checks, generated-source checks, and behavioral findings. These signals record evidence and gaps; they do not certify ISO 19775-1 conformance.

Axis 1 — Structural (L2 validator). A Python validator derives a per-version criteria oracle directly from the ISO Unified Object Model (UOM) XML. It checks each document against the UOM-extracted manifest for its own declared version: unknown nodes/fields, illegal ROUTE endpoints, and containerField mismatches. The oracle is commit-locked (golden-style) so criteria drift fails loudly. The structural report (`docs/conformance/REPORT.md`) over the full 16,866-file conformance archive returns 99.66% structurally clean against per-file oracles.

Axis 2 — Behavioral (conformance view). A findings register (`docs/conformance/findings.yaml`) records author-judged behavioral gaps referenced to ISO 19775-1 clause numbers. A view generator (`scripts/conformance_view.py`) auto-derives code FACTS (node exists, extracts, has a System wired) from the generated bindings and runtime registry, joins them with the YAML JUDGMENTS, and emits the machine-readable zoom-out (`docs/conformance/INDEX.md`, `model.json`, `components/`). The split prevents judgments from rotting into stale code claims and vice versa.

Axis 3 — Oracle-free (corpus audit + sweep). Two C++ tools drive the full X3D conformance archive through the `x3d::sdk` façade. The sweep (`corpus_sweep`) verifies the full parse-extract-tick pipeline at breadth. The audit (`corpus_audit`) runs oracle-free invariant checks (bounds finiteness, tick determinism, re-extract stability) and round-trip fidelity (write → reparse → semantic fingerprint comparison across XML/JSON/ClassicVRML) to surface codec bugs without needing golden answers.

## Key files

| File / directory | Role |
|---|---|
| `src/x3d_cpp_gen/conformance/` | Python conformance package (L2 validator, manifests, version resolution, errata, fidelity, prose anchors, report) |
| `src/x3d_cpp_gen/conformance/manifests/` | Committed per-version UOM-derived criteria JSON (x3d-3.0.json … x3d-4.1.json) |
| `src/x3d_cpp_gen/conformance/prose_anchors.json` | Committed artifact: node → ISO-19775-1 prose section anchor map |
| `docs/conformance/findings.yaml` | Behavioral findings source of truth — the one file authors edit to raise/close gaps |
| `docs/conformance/INDEX.md` | Generated zoom-out (bug picture + component × dimension matrix) |
| `docs/conformance/model.json` | Generated machine/agent/RAG-queryable merged conformance model |
| `docs/conformance/components/` | Generated per-component drill-down pages |
| `docs/conformance/REPORT.md` | Generated structural conformance report (per-version scorecard + residual findings) |
| `tools/corpus_audit.hpp` | Header-only oracle-free audit engine (invariants + round-trip fidelity) |
| `tools/corpus_audit.cpp` | CLI binary `x3d_corpus_audit` — directory walk + JSONL output |
| `tools/corpus_sweep.cpp` | CLI binary `x3d_corpus_sweep` — breadth smoke over the full archive |
| `scripts/conformance_view.py` | View generator: `generate` (writes INDEX.md, model.json, components/) and `check` (CI gate) |
| `tests/conformance/` | Python pytest suite for the validator, manifests, errata, fidelity, and version resolution |

## Interfaces and seams

### L2 structural validator

`conformance.report.build_report()` is the Python corpus walker. It resolves
each file's version, caches manifests, and reports validation results. Malformed
XML counts as a parse error; internal validator exceptions propagate so a broken
validator cannot appear as malformed corpus input.

The Python validator takes an XML bytestring and a `Manifest` object; returns a list of `Finding` records.

```python
# src/x3d_cpp_gen/conformance/validate.py
def validate_document(xml: str | bytes, manifest: Manifest) -> List[Finding]

# src/x3d_cpp_gen/conformance/codes.py
@dataclass
class Finding:
    code: str       # e.g. "NODE_UNKNOWN_FOR_VERSION", "ROUTE_ACCESS_ILLEGAL"
    severity: int   # 0=Error, 1=Warning, 2=Info, 3=Hint
    pointer: str    # document path/locator (e.g. "/Scene/Shape[0]/Box")
    message: str
    data: Optional[Dict[str, Any]]
```

Supported codes (append-only, never renumbered): `NODE_UNKNOWN_FOR_VERSION`, `FIELD_UNKNOWN_FOR_NODE`, `CONTAINERFIELD_MISMATCH`, `ROUTE_ENDPOINT_UNKNOWN`, `ROUTE_ACCESS_ILLEGAL`, `VERSION_DECLARED`, `VERSION_INFERRED_XSD_CITED`, `VERSION_INFERRED_NODE_FORCED`, `VERSION_INFERRED_PROFILE_FLOOR`, `VERSION_INFERRED_BARE_FLOOR`, `DEFAULT_NOT_MATERIALIZED`.

### Version resolution (VP-2 ladder)

```python
# src/x3d_cpp_gen/conformance/version_resolve.py
def resolve_version(xml: str | bytes) -> VersionResolution
# VersionResolution: version (str), stamp (str), declared (bool)

@functools.lru_cache(maxsize=None)
def load_manifest(version: str) -> Manifest
# raises FileNotFoundError if no committed manifest for that version
```

The ladder runs five rungs in order (Rung 0–4): declared `version` attribute → xsi:noNamespaceSchemaLocation / DOCTYPE SYSTEM citation → node-floor (highest earliest-version among nodes actually used) → profile-floor → bare 3.0 floor.

### Manifest + errata

```python
# src/x3d_cpp_gen/conformance/manifest.py
def extract_manifest(uom_file: str) -> Manifest
# Parses a UOM XML file; derives version, node dict, content hash, source path.

# src/x3d_cpp_gen/conformance/errata.py
def apply_errata(version: str, nodes: Dict[str, Any]) -> List[Dict[str, Any]]
# Applied IN PLACE at load time. Each erratum is guarded by the expected `from`
# value and an optional type guard; a missed guard is a silent no-op (self-disabling).
```

Errata are applied on `load_manifest`; the committed JSON files remain byte-faithful to the UOM. The only current erratum corrects `Viewpoint.orientation.accessType` (`initializeOnly` → `inputOutput`) in the X3D 3.0 manifest (an upstream UOM bug, corrected in 3.1).

`ERRATA` can only rewrite an attribute of a field that exists. Fields the published standard defines but the vendored UOM omits go in `FIELD_ADDITIONS` (same file): each record cites the standard's section and URL and carries the standard's own description text. The overlay is applied in two places and nowhere else — `load_manifest` (`apply_field_additions_manifest`, so the corpus audit stops flagging the field) and `x3d-cpp-gen` after `parse_x3d_model` (`apply_field_additions_model`, so the generated bindings gain the accessors). `parse_x3d_model` itself and `conformance/manifest.py` stay pure, so the vendored XML, the committed manifests and their `uomManifestHash` remain byte-faithful. An addition is a no-op once the UOM carries the field. The only current addition is `WaveShaper.curve` (X3D 4.0, ISO/IEC 19775-1 §16.4.21); other Sound differences are tracked in `UOM-`. Never edit `src/x3d_cpp_gen/data/` to fix a UOM gap.

### Fidelity check (generator moat)

```python
# src/x3d_cpp_gen/conformance/fidelity.py
def check_defaults_materialize(manifest: Manifest) -> List[Finding]
# Returns DEFAULT_NOT_MATERIALIZED findings for any non-enum field whose UOM
# default cannot be translated to a C++ default expression by the generator.
```

This is the non-circular pillar: the L2 validator against UOM-derived criteria could be circular (the generator reads the same UOM). The fidelity check crosses the UOM → generator boundary directly, catching dropped or mangled defaults that a reflection tautology would hide.

### Meta-validator (oracle admission guard)

```python
# src/x3d_cpp_gen/conformance/meta_validator.py
def validate_manifest(m: Manifest) -> None
# Raises MetaValidationError if below node-count floor or missing component/fields shape.
# Floors: 4.0/4.1 >= 240, 3.x >= 180..200.

def assert_field_types_mapped(uom_file: str) -> None
# Raises MetaValidationError if any <field type=...> in the UOM is unmapped in
# FIELD_TYPE_MAPPING / XS_TYPES — the lenient parser silently drops unmapped types.
```

### Prose anchors

```python
# src/x3d_cpp_gen/conformance/prose_anchors.py
def build_anchor_map(...) -> dict
# Derives node → ISO 19775-1 prose section anchor from the committed manifests
# + an on-disk prose mirror (path via $X3D_SPEC_PROSE_DIR or default).
# Output committed as prose_anchors.json; rebuilding requires the prose mirror.

def section_text(anchor: dict, prose_dir=PROSE_DIR) -> Optional[str]
# Live slice of prose for an anchor (heading to next same/higher heading).
# Returns None when the prose mirror is absent (citation-only degrade).
```

### Corpus audit engine (C++)

```cpp
// tools/corpus_audit.hpp — namespace x3d::audit
struct Finding { std::string file, check, encoding, nodeType, detail, signature; };
struct Fingerprint {... };  // order-independent, float-tolerant semantic fingerprint

void auditDocument(sdk::X3DDocument &doc, const std::string &file,
                   const AuditOptions &ao, std::vector<Finding> &out);
// Dispatches auditInvariants (family A) and auditRoundTrip (family B).
// Goes exclusively through x3d::sdk — exercises the exact embedder surface.
```

The fingerprint compares: `itemCount`, a sorted multiset of per-item canonical tuples (topology | position count | index count | baseColor RGB (rounded to 1/256) | world translation (±1e-3) | scale magnitudes (±1e-3) | determinant (±1e-3)), plus structural counts (nodeCount, defCount, useCount, routeCount, protoCount). Rounding is the key mechanism: formatting/default-dropping/float-precision drift across encodings is expected and must not be flagged.

Check families: `bounds-finite`, `bounds-nonempty`, `transform-finite`, `tick-determinism` (only on scenes without time-driver nodes), `reextract-stable`, `roundtrip` (one per encoding: xml/json/classicvrml), `reparse-throw`, `reparse-throw-external` (informational: Inline/EXTERNPROTO url), `write-throw`, `reextract-throw`.

### Behavioral conformance view seam

The view generator reads `docs/conformance/findings.yaml` for JUDGMENTS and queries the generated bindings, `X3DInterfaceRegistry`, and `runtime/` System registration for FACTS. Callers of the view:

- `mise run conformance` — `conformance_view.py generate` — writes `model.json` + `INDEX.md` + `components/`.
- `mise run conformance-gate` — `conformance_view.py check` — validates schema + regenerates to a temp directory and diffs (CI gate, wired into `mise run ci`).

### Seam points

- UOM source — `extract_manifest(uom_file)` reads the ISO UOM XML through the existing `x3d_cpp_gen.parser.parse_x3d_model` pipeline; the conformance manifests share that parser rather than owning a separate one.
- Generator bridge — `fidelity.check_defaults_materialize` calls `x3d_cpp_gen.emit.defaults.default_expr_for` to cross from UOM data to generator output.
- x3d::sdk façade — `corpus_audit.hpp` and `corpus_sweep.cpp` depend only on `x3d/sdk.hpp`; no internal headers are used, ensuring these tools exercise the public embedder surface.
- Prose mirror — `prose_anchors.py` reads an on-disk prose mirror at `$X3D_SPEC_PROSE_DIR` (when unset it falls back to a non-existent `_x3d_spec_prose_unset` placeholder, i.e. effectively disabled). The committed `prose_anchors.json` means the mirror is only needed for rebuilding.

## Related
