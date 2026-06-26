# UOM ↔ Spec-Prose Anchors — Design

**Date:** 2026-06-15
**Status:** Approved (brainstorm), pending implementation plan
**Scope:** Tooling. Pure-Python + committed data. **Zero C++/golden impact.**

## Problem

x3d-cpp-gen has two spec encodings on disk, with nothing linking them:

- **UOM / manifests** (`src/x3d_cpp_gen/conformance/manifests/x3d-*.json`) — the *structural* spec
  per node/field: type, accessType, default, min/maxInclusive, acceptableNodeTypes,
  containerField, component, baseType. Drives the generator and the L2 conformance validator.
- **Spec prose RAG** (`scripts/spec_rag.py`) — the *normative semantic* spec: 62 ISO/IEC
  19775-1:2023 markdown pages mirrored to `x3d-render/data/x3d_spec_prose/`, chunked into
  ~6,900 Qdrant vectors. Fuzzy semantic search only.

Today, going from a UOM node/field to the prose clause that governs it is manual (query the
RAG, eyeball, implement, re-verify) — exactly how the recent TC1–TC4 default-texcoord work
was done. There is no deterministic, citable map from `Cone` → its governing prose section.

This also matters to the **moat**: the M3 conformance design named the "ISO-prose hand-check"
as one of the few *genuinely independent* conformance anchors (the UOM and the XSD share Web3D
pipeline lineage — they cross-check for *fidelity*, not independence; the ISO prose is the
independent human spec). Mechanizing UOM↔prose linkage turns that hand-check into a
repeatable, citable artifact.

## Goal

Build the deterministic **node→prose anchor map** as a shared foundation, then expose it two ways:

1. **Dev/implementation aid** — a node-keyed lookup in `spec_rag.py` (`node Cone`,
   `field Cone.bottomRadius`) that returns the manifest entry + the linked prose section +
   RAG neighbors. Optimizes the implement→verify loop.
2. **Conformance/fidelity cross-check** — a coverage report of which UOM nodes the prose
   covers vs not (gaps, version skew, naming mismatches).

**Granularity:** node-level deterministic spine + field best-effort at query time.

This is "Approach A" of the brainstorm. Promoting coverage into a formal M3-report oracle with
a CI gate ("Approach C") is a deliberate fast-follow, out of scope here.

## Non-goals

- No vendoring of the prose mirror into this repo (decided: keep it external in `x3d-render`;
  the rebuild-gate test skips when the mirror is absent). Revisit when promoting to the M3 report.
- No field-level data baked into the committed artifact (field best-effort is query-time RAG;
  nothing embedding-derived enters the committed JSON, preserving determinism).
- No new MCP tool, no standalone CLI (the lookup extends the existing `spec_rag.py`).
- No C++ changes, no codegen changes, no golden change.

## Architecture & data flow

Two units, mirroring the existing build-vs-consume split (`manifest.py` builds → `validate.py`
consumes):

- **`src/x3d_cpp_gen/conformance/prose_anchors.py`** — *builds and owns* the artifact.
  Pure-Python, deterministic, no network. Entry point:
  `python -m x3d_cpp_gen.conformance.prose_anchors build|coverage`.
- **`scripts/spec_rag.py`** — *consumes* the committed artifact for day-to-day lookup
  (`node`/`field` subcommands), plus the existing RAG for field best-effort.

Committed artifact: **`src/x3d_cpp_gen/conformance/prose_anchors.json`** (sibling to
`manifests/`). The prose mirror stays in `x3d-render` (shared `PROSE` path, as `spec_rag.py`
already assumes). The committed map is the small *derived* index; full section-text display
reads the live mirror and degrades to citation-only if it is absent.

```
manifests/x3d-*.json ─┐
                      ├─► prose_anchors.py build ─► prose_anchors.json ─┬─► spec_rag.py node/field  (dev lookup)
x3d_spec_prose/*.md ──┘                                                 └─► prose_anchors.py coverage (cross-check)
```

## Component 1 — Anchor-build algorithm (deterministic core)

In `prose_anchors.py`.

1. **Target node set:** load the union of node names from all committed manifests
   (`conformance/manifests/x3d-*.json`). Each node carries its `abstract` flag and `component`
   from the manifest.
2. **Walk the prose** (`PROSE.glob("*.md")`, skipping the index pages `INDEX.md`,
   `nodeIndex.md`, `componentIndex.md`):
   - Read line 1 `<!-- source: URL -->` → `sourceUrl`.
   - For every heading line `^(#{1,4})\s+(.*)`:
     - Extract the **first** `<span id="([A-Za-z0-9_]+)">` → `anchorId`.
     - Clean the title: strip all `<span…>`/`<img…>` HTML, strip the leading section number
       `^[\d.]+\s*`, strip surrounding `*` emphasis, trim.
     - Parse the leading `[\d.]+` → `sectionNumber`; `depth` = number of dots.
     - Record candidate `(anchorId, file, sourceUrl, sectionNumber, depth, headingLevel, title)`.
3. **Match** each target node `N` against candidates where `anchorId == N` (exact):
   - **1 match** → anchor it.
   - **>1** → choose by deepest section `depth`, then heading level, then file order; stash the
     losers under `ambiguous`.
   - **0** → case-insensitive retry; if found, record under `fuzzy` (needs-review, **not** a
     clean anchor); still 0 → `unanchored`.
4. **Emit** `prose_anchors.json` (canonical, sorted, like the manifests) with a
   `proseCorpusHash` = sha256 over the sorted list of `(filename, sha256(content))`, so mirror
   drift is detectable.

Determinism holds because matching is exact string identity over committed inputs (the
manifests) and the on-disk prose. Nothing embedding-derived enters the committed artifact.

### Why the anchor-id is the reliable key

Prose node headings look like `## <span id="Cone"></span> 13.3.2 Cone`. The `<span id="">`
anchor equals the node name for node-reference sections, including abstract types
(`<span id="X3DInterpolatorNode"></span> 19.3.1 *X3DInterpolatorNode*`). Section numbering is
**not** uniform across components (some put concrete nodes under `N.3`, some under `N.4`), so we
rely on the anchor-id, not the number. Concept-section anchors (`id="Overview"`, `id="Name"`,
etc.) simply never match a UOM node name and fall away.

## Component 2 — `prose_anchors.json` schema

```json
{
  "provenance": {
    "proseCorpusHash": "sha256…",
    "builtFromManifests": ["x3d-3.0", "x3d-3.1", "x3d-3.2", "x3d-3.3", "x3d-4.0", "x3d-4.1"],
    "nodeCount": 351,
    "anchoredCount": 333,
    "fuzzyCount": 2,
    "unanchoredCount": 16
  },
  "anchors": {
    "Cone": {
      "file": "geometry3D.md",
      "sourceUrl": "https://www.web3d.org/.../components/geometry3D.html",
      "fragment": "Cone",
      "section": "13.3.2",
      "title": "Cone",
      "abstract": false,
      "ambiguous": []
    }
  },
  "fuzzy":      { "SomeNode": { "matchedAnchor": "somenode", "file": "…", "section": "…" } },
  "unanchored": ["ExtensionNodeX", "…"]
}
```

The citable deep-link for any anchor is `sourceUrl + "#" + fragment`.

## Component 3 — Dev lookup (`spec_rag.py` extensions)

New subcommands; existing `ingest` and free-text query untouched. `spec_rag.py` gains a loader
that reads the committed `prose_anchors.json` and the manifests.

- **`spec_rag.py node Cone`**
  - Manifest summary: component, containerField, baseType, field count (and abstract flag).
  - Citation: `geometry3D.md §13.3.2 Cone` + `sourceUrl#Cone`.
  - Prose section text: live slice of `geometry3D.md` from the `Cone` heading to the next
    heading of the same or higher level. `[prose mirror not present]` fallback if absent.
  - `--rag k`: append `k` semantically-related chunks from the existing RAG.
- **`spec_rag.py field Cone.bottomRadius`**
  - The field's manifest record: type, accessType, default, min/maxInclusive,
    acceptableNodeTypes, simpleType.
  - **Best-effort** prose (clearly labeled): slice the node's section, substring-find the field
    name → surrounding sentence(s); on miss, fall back to a scoped RAG query
    (`"<Node> <field>"`). Field best-effort is never persisted, only displayed.

Which version's manifest backs `node`/`field` lookups defaults to the newest (4.1 superset);
a `--version` flag can select another.

## Component 4 — Coverage cross-check

`prose_anchors.py coverage [--version 4.1] [--json|--md]` → for the chosen version's manifest:

- `anchored` — nodes with a clean anchor.
- `unanchored` — nodes with no prose anchor, each tagged abstract/concrete + component
  (extension nodes, mirror gaps).
- `fuzzy` — case-only matches needing review.
- `prose-only` — prose node-anchors matching **no** node in **any** manifest (version-newer
  nodes, or our naming mismatches).

Output styled like `report.py` (JSON + markdown). This is the standalone fidelity artifact;
wiring it into the M3 report with a stable code + CI gate is the deferred Approach C.

## Testing & impact

`tests/test_prose_anchors.py`:

- **Synthetic build test** — a tiny in-test prose fixture (2–3 fake component `.md` files with
  known anchors) + a tiny manifest, exercising exact / ambiguous / fuzzy / unanchored paths.
  Deterministic, no network, no real mirror.
- **Schema round-trip** on the committed `prose_anchors.json`: valid shape, every anchor's
  `fragment == node name`, every `section` parses.
- **Coverage-floor test** — `anchoredCount / nodeCount ≥ floor` for 4.1, and
  `unanchored ⊆ calibrated allow-list` so a node silently losing its anchor fails CI.
- **Rebuild-gate** (committed == fresh build) — **skips** when the real prose mirror is absent
  (same stance as the network-dependent RAG tests).

Impact: pure Python + committed JSON. **Golden byte-unchanged; no codegen; no C++.** pytest
count rises by the new tests.

## Open follow-ups (out of scope, named)

- **Approach C** — promote coverage into `report.py` as a first-class oracle with a stable
  issue code (e.g. `PROSE_ANCHOR_MISSING`) and a meta-validator CI gate. Do once the anchor map
  is proven stable.
- **Prose vendoring** — vendor the 62 prose `.md` files into this repo to make the prose a
  self-contained, CI-gateable third oracle. Tie to Approach C.
- **3.x prose** — only the 4.0/4.1 prose is mirrored; algorithms are version-stable, so the
  single corpus anchors all versions. Mirror 3.x prose only if a version-specific clause is ever
  needed.
- **Field-level baked anchors** — if query-time field best-effort proves valuable, consider a
  curated (human-reviewed) field→clause table, kept separate from the deterministic node map.
