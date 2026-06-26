---
title: Canonical XML Writer
summary: X3D canonical-XML writer (X3DC14N) for byte-stable, deterministic output used by the canonicalize gate.
tags: [subsystem, canonical, xml, x3dc14n, canonicalize]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/codecs-writers.md
  - ../decisions/0008-canonicalize-source-provenance.md
---

# Canonical XML Writer

The Canonical XML Writer serializes the runtime document model to X3D Canonical Form (X3DC14N): a deterministic, byte-stable XML dialect whose rules are derived from X3DJSAIL's `-canonical` mode. Its purpose is to give the toolchain a fixed-point output that can be compared byte-for-byte across runs, tools, and versions — the foundation of the `x3d canonicalize` command and the canon-gate regression suite.

The writer is deliberately a **separate class** from the default `XmlWriter`. The default codec path has zero coupling to this writer; adding or changing canonicalization rules carries no risk of touching general serialization.

## Purpose

X3D files produced by different authoring tools vary arbitrarily in attribute order, number precision, whitespace, and quote style. Downstream diff-based gates need a deterministic normal form. `CanonicalXmlWriter` enforces eight X3DC14N rules:

1. XML prolog: `<?xml version="1.0" encoding="UTF-8"?>`
2. Version-appropriate X3D DOCTYPE line (covers 3.x and 4.x).
3. Attributes sorted alphabetically by name within each element.
4. Single-quote attribute delimiters.
5. X3DC14N escaping in attribute values: `&amp;`, `&lt;`, `&gt;`, `&apos;` (not `&quot;` — the delimiter is a single quote).
6. 2-space-per-level indentation; one element per line.
7. Minimal number formatting: shortest round-trip `float`/`double` via `std::to_chars`; integers emit without a decimal point (`0`, not `0.0`); doubles always include a decimal point (`0.0`, not `0`) to avoid type-mismatch on re-parse.
8. Structural fidelity: DEF/USE sharing, ProtoDeclare/ProtoBody/IS connections, ROUTE, IMPORT/EXPORT, `<head>` meta, `xmlns:xsd`, and `xsd:noNamespaceSchemaLocation` are all preserved.

The output is idempotent: `canonicalize(canonicalize(x)) == canonicalize(x)` byte-for-byte. This property is the hard (100%-required) gate in the canon-gate test suite.

## Key files

| File | Role |
|---|---|
| `runtime/codecs/CanonicalXmlWriter.hpp` | Header-only implementation; namespace `x3d::codec`; the entire subsystem lives here |
| `runtime/codecs/X3DCodecs.hpp` | Umbrella codec header — `#include`s `CanonicalXmlWriter.hpp` alongside the other writers |
| `include/x3d/sdk.hpp` | Re-exports `x3d::codec::CanonicalXmlWriter` as `x3d::sdk::CanonicalXmlWriter` |
| `tools/x3d_cli.cpp` | `cmdCanonicalize` — CLI entry point; instantiates `sdk::CanonicalXmlWriter` and writes to stdout or `-o` |
| `tools/x3d-cli/canonicalize_unit_test.cpp` | 13 focused unit tests (T1–T13); the ctest target `x3d_canonicalize_unit_test` |
| `tools/x3d-cli/canon_gate.cpp` | Three-tier differential gate vs X3DJSAIL golden fixtures; run via `mise run canon-gate` |

## Interfaces and seams

### Exposed interface

```cpp
// namespace x3d::codec (also re-exported as x3d::sdk::CanonicalXmlWriter)
class CanonicalXmlWriter {
public:
    /// Serialize a full document to its X3D Canonical Form string.
    std::string writeDocument(const runtime::X3DDocument &doc);
};
```

`writeDocument` is the single public entry point. It accepts the parsed runtime document model and returns the complete canonical XML string including prolog and DOCTYPE. The writer holds no shared state with `XmlWriter`; each call to `writeDocument` resets internal DEF/USE tracking (`seen_.clear()`); the default-value cache (`defaults_`) persists across calls.

### Internal helpers (free functions, same header)

| Function | Role |
|---|---|
| `canonEscape(s)` | Single-quote-context XML escaping (`&amp;`, `&lt;`, `&gt;`, `&apos;`) |
| `canonFmtFloat(v)` | Shortest round-trip `float` string; integer fast-path emits `"0"` not `"0.0"` |
| `canonFmtDouble(v)` | Shortest round-trip `double` string; always includes a decimal point |
| `canonFormatValue(type, v)` | Dispatches over all `X3DFieldType` values; uses `canonFmtFloat`/`canonFmtDouble` for numeric types |
| `x3dDoctype(version)` | Returns the DOCTYPE line for a given version string |
| `x3dSchemaLocation(version)` | Returns the XSD schema URL for the `xsd:noNamespaceSchemaLocation` attribute |

### Seam points

- **Runtime document model** — `writeDocument` takes `const runtime::X3DDocument &`. The document is produced by any of the parse readers (XML, VRML, JSON); the writer is agnostic to which reader was used.
- **Reflection / `FieldTable`** — field enumeration, default comparison, and enum-string formatting are driven by the generated reflection `FieldTable` (same path as `XmlWriter`). The canonical writer calls `node->fields()`, `f.get()`, `f.getEnumString`, and `X3DNodeFactory::create` for default-value comparison; it does not contain any per-node code.
- **`DynamicField` / `effectiveFields()`** — Script author fields (declared via `<field>` elements) are serialized through `runtime::effectiveFields(script)`, the same seam used by the event cascade and routes layer. Author fields beyond the static field count are emitted as child `<field>` elements with `accessType`/`name`/`type` and optional `value`.
- **`ProtoInstance::expanded` flag** — un-expanded proto instances are round-tripped via `scene_->expandedSources` and `scene_->protoInstances`; expanded instances are written as their expanded node with a back-pointer to the proto instance element.
- **Inline round-trip** — expanded `Inline` nodes are looked up in `scene_->expandedInlines` and serialized as their original Inline element, not the expanded subtree.
- **Default-writer isolation** — `CanonicalXmlWriter` builds its own `xml::Element` tree using the same `XmlLite` types as `XmlWriter`, but renders it with `renderCanonical` (sorted attrs, single-quote). `XmlWriter`'s `renderElement` is never called. The unit test T11 verifies byte-identity of `XmlWriter` output before and after a `CanonicalXmlWriter` invocation.

### What it depends on

| Dependency | Why |
|---|---|
| `DynamicField.hpp` | `effectiveFields()` for Script author fields |
| `FieldValueIO.hpp` | `fmtBool`, `fmtMFString`, `fmtMatrixF/D`, `fmtImage` for non-numeric field types |
| `ProtoNameMaps.hpp` | `accessTypeName`, `fieldTypeName` for field/IS serialization |
| `Script.hpp` | `Script` dynamic-cast for CDATA source code + author field serialization |
| `X3DNodeFactory.hpp` | `X3DNodeFactory::create` — instantiates a default node per type for default-suppression |
| `X3DRuntime.hpp` | `runtime::X3DDocument`, `runtime::Scene`, `runtime::ProtoInstance`, etc. |
| `XmlLite.hpp` | `xml::Element` tree used internally for two-phase build-then-render |

## How it is tested

**Unit tests (fast, in-process):**

`ctest --preset dev -R x3d_canonicalize_unit_test` runs 13 property checks implemented in `tools/x3d-cli/canonicalize_unit_test.cpp`:

| Test | Property verified |
|---|---|
| T1 | XML prolog line is present |
| T2 | DOCTYPE line is version-specific (3.2 and 4.0 checked) |
| T3 | Attribute delimiters are single-quote; no double-quote on attrs |
| T4 | Attributes are alphabetically sorted within each element |
| T5 | Minimal float formatting: `0.8 0 0` not `0.800000 0.000000 0.000000` |
| T6 | Integer-valued floats have no `.0` suffix: `size='3 4 5'` not `3.0 4.0 5.0` |
| T7 | `xmlns:xsd` and `xsd:noNamespaceSchemaLocation` present on `<X3D>` |
| T8 | DEF/USE identity preserved across round-trip |
| T9 | Idempotence: `canonicalize(canonicalize(x)) == canonicalize(x)` |
| T10 | 2-space indentation; no tab characters |
| T11 | `XmlWriter` output byte-identical before and after `CanonicalXmlWriter` use (isolation proof) |
| T12 | `<meta>` attrs in canonical order (`content` before `name`) |
| T13 | Apostrophe in attribute value escapes to `&apos;` (single-quote context) |

**Canon gate (corpus-scale differential, external corpus required):**

`tools/x3d-cli/canon_gate.cpp` implements a three-tier gate driven via `x3d canonicalize` subprocess calls over the XML-only subset of the conformance archive:

- **Tier 1 — Idempotence** (hard gate, 100% required, Java-free): `c1 == c2` byte-for-byte over the full XML subset. Any failure exits 1. This is the CI-wired gate (`--gate` flag).
- **Tier 2 — Tolerant diff vs X3DJSAIL goldens** (baseline-locked regression): structural and numeric agreement with committed X3DJSAIL canonical fixtures (`tools/x3d-cli/goldens/canonical-goldens/`). Numbers compared within relative tolerance 1e-5; `containerField` and schema-location attrs stripped from both sides before comparison. PASS→FAIL is a regression.
- **Tier 3 — Byte-exact vs X3DJSAIL** (informative stretch metric): fraction of corpus files where our output is byte-identical to the JSAIL reference. Not a gate.

`x3d_canon_gate` is NOT registered as a ctest target (requires the external corpus). Run manually via `mise run canon-gate`. Baselines for both cli-gate and canon-gate are written by `mise run cli-gate-baseline`.

## Related

In-wiki:

- [Architecture](../architecture.md) — layered system overview; the codec writers sit in the codec layer above the runtime core
- [Codec Writers](../subsystems/codecs-writers.md) — the sibling writers (`XmlWriter`, `VrmlWriter`, `JsonWriter`) that share the same codec umbrella but have zero coupling to this writer
- [ADR-0008: Canonicalize Source-Provenance Handling](../decisions/0008-canonicalize-source-provenance.md) — the decision record for how `x3d canonicalize` handles source-file provenance and base-URL in the canonical output header

Cited paths (out-of-wiki — not hyperlinked per CONVENTIONS):

- `docs/superpowers/specs/2026-06-20-x3d-canonicalize-design.md` — the dated design spec that drove the X3DC14N rule set and the canon-gate tiers
- `docs/superpowers/BACKLOG.md` — conformance backlog; canon-gate wired as a pre-merge gate per the ADR-0010 decision
