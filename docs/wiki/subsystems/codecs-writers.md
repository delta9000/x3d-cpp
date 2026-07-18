---
title: Codec Writers
summary: XML, VRML, and JSON serialization writers plus field-value IO — the full set of output codecs.
tags: [subsystem, codecs, writers, xml, vrml, json, serialization]
updated: 2026-07-17
related:
  - ../architecture.md
  - ../subsystems/canonical-xml.md
  - ../subsystems/parse-readers.md
---

# Codec Writers

## Purpose

The codec writers subsystem converts an in-memory `x3d::runtime::X3DDocument` (or a `Scene` / single node) into one of the three X3D encoding wire formats: **X3D-XML** (`.x3d`), **ClassicVRML** (`.x3dv`), and **X3D-JSON** (`.x3dj`). A fourth writer, `CanonicalXmlWriter`, produces the deterministic X3D Canonical Form (X3DC14N) used by the differential conformance gate. All four writers are driven purely by the generated reflection `FieldTable` and the `X3DNodeFactory`; none contain per-node code. The shared field-value formatting kernel (`FieldValueIO.hpp`) is the bidirectional text↔`std::any` bridge shared by the writers and the codec readers alike.

The writers sit between the runtime document model and any consumer that needs a serialized representation — the `x3d convert` CLI command, the round-trip conformance auditor, golden-file regression fixtures, and the canonical-form differencer.

## Key files

| File | Role |
|---|---|
| `runtime/codecs/X3DCodecs.hpp` | Umbrella include that pulls in all codecs (writers + readers + XmlLite) |
| `runtime/codecs/XmlWriter.hpp` | Runtime model → X3D-XML; header-only, `namespace x3d::codec` |
| `runtime/codecs/VrmlWriter.hpp` | Runtime model → ClassicVRML (X3D ClassicVRML / `.x3dv`); header-only |
| `runtime/codecs/JsonWriter.hpp` | Runtime model → X3D-JSON; header-only (round-trip not required) |
| `runtime/codecs/CanonicalXmlWriter.hpp` | Runtime model → X3D Canonical Form (X3DC14N); header-only, fully separate from `XmlWriter` |
| `runtime/codecs/FieldValueIO.hpp` | Bidirectional field-value IO: `formatValue` (`std::any` → wire string) and `parseValue` (wire string → `std::any`), all type-switched on `X3DFieldType`; also low-level helpers (`fmtFloat`, `parseFloat`, `tokenize`, `parseMFString`, …). Floats/doubles serialize in **shortest round-trip form** (`std::to_chars`): authored digits like `0.9` survive a round-trip verbatim instead of expanding to `0.899999976`, and `parse(format(v)) == v` bit-exactly (pinned by `fval_extended_test`). `CanonicalXmlWriter`'s `canonFmtFloat` is deliberately separate — X3DC14N owns its own pinned formatting contract. |
| `runtime/codecs/ProtoNameMaps.hpp` | `fieldTypeName()` and `accessTypeName()` — enum → string helpers shared by all three writers |
| `runtime/codecs/XmlLite.hpp` | Lightweight hand-written XML element tree and renderer used by `XmlWriter`, `CanonicalXmlWriter`, and `XmlReader` |
| `runtime/codecs/XmlReader.hpp` | X3D-XML → runtime model (counterpart reader; out of scope for this page — see [Codec Readers](../subsystems/parse-readers.md)) |
| `runtime/codecs/tests/` | Per-writer and cross-writer round-trip tests |

## Interfaces and seams

### Exposed interface

All three primary writers expose the same method shape:

```cpp
// namespace x3d::codec

class XmlWriter {
public:
    // Full document serialization (includes <?xml?> prolog, <X3D><head><Scene>).
    std::string writeDocument(const runtime::X3DDocument &doc);

    // Scene-only serialization (no <X3D> wrapper).
    std::string writeScene(const runtime::Scene &scene);

    // Single-node subtree (no Scene/X3D wrapper; DEF/USE tracking reset).
    std::string writeNode(const std::shared_ptr<X3DNode> &node);
};

class VrmlWriter {
public:
    std::string writeDocument(const runtime::X3DDocument &doc);
};

class JsonWriter {
public:
    std::string writeDocument(const runtime::X3DDocument &doc);
};

class CanonicalXmlWriter {
public:
    std::string writeDocument(const runtime::X3DDocument &doc);
};
```

The field-value IO functions are free functions in `namespace x3d::codec`:

```cpp
// X3D wire string -> std::any (for a FieldInfo `set` thunk).
std::any parseValue(X3DFieldType type, const std::string &s);

// std::any -> X3D wire string (for attribute/field text).
std::string formatValue(X3DFieldType type, const std::any &v);

// Enum-quote stripping: removes double-quote chars from an enum wire value
// before matching against bare token identifiers (AUD-D fix).
std::string stripEnumQuotes(const std::string &wire);
```

### Serialization strategy common to all writers

Every writer applies the same three-phase strategy, driven entirely by reflection — no per-node code anywhere in the subsystem:

1. **Default elision.** For each field, a fresh factory instance of the node's type is obtained via `X3DNodeFactory::create(typeName)` and cached. Fields whose current value formats identically to the factory default are omitted, producing clean output.
2. **DEF/USE deduplication.** Node identity (raw pointer) is tracked in an `unordered_set<const X3DNode *>`. The first emission of a shared node writes a `DEF`; all subsequent references emit a `USE` reference with no fields or children.
3. **PROTO / Inline round-trip redirect.** Before emitting a node the writer checks `scene.expandedSources` (for expanded `ProtoInstance`s) and `scene.expandedInlines` (for expanded `Inline` nodes). If a match is found, the original captured source structure is re-emitted instead of the expansion, so the written output can be re-parsed to re-expand rather than serializing the expanded tree.

### Encoding-specific behaviors

**XmlWriter:**
- Value fields become XML attributes; `SFNode`/`MFNode` fields become child elements under the child's `containerField` name.
- Enum fields use the `FieldInfo::getEnumString` thunk.
- A `Script` node's inline source is emitted as a `<![CDATA[...]]>` text body (SCR-SAI-DYN S1) rather than an attribute; author `<field>` declarations from the `DynamicFieldStore` are emitted as `<field>` child elements. A literal `]]>` in the source is split across consecutive CDATA sections (`xml::cdataEscape`; the reader concatenates them back — ENC-CDATA-SCRIPT).
- ProtoBody re-emit uses a fresh `XmlWriter` instance (isolated DEF/USE bookkeeping); `IS` connections are threaded via `bodyIsc_` so `<IS><connect>` blocks appear at any nesting depth inside the body.
- Un-expanded `ProtoInstance`s that were not resolved (no graph node) are re-emitted from `scene.protoInstances` so they survive a round-trip.

**VrmlWriter:**
- Emits `#X3D V<version> utf8` header; sub-3.0 version tokens are floored to `3.0` (VP-2 conformance).
- MF values are always bracketed with `[ ... ]` to satisfy the ClassicVRML grammar (ISO/IEC 19776-2 `mfValue ::= '[' ... ']'` — bare runs truncate on reparse; AUD-A fix).
- `SFBool`/`MFBool` tokens are uppercased to `TRUE`/`FALSE` (VRML convention, not XML/JSON).
- `SFString` values are double-quoted with `\"` / `\\` escaping; `MFString` self-quotes via `fmtMFString`.
- Author Script field declarations (from the `DynamicFieldStore`) are re-emitted as `accessType FieldType name [default]` interface lines inside the node body (Task B).
- `IS` connections are emitted as `nodeField IS protoField` lines inside the body node's braces (PRF-1).
- Case-A nested ProtoInstances inside a ProtoBody are injected into the parent body node's braces via `writeVrmlNestedFor` (PRF-3).

**JsonWriter:**
- Follows Web3D X3D-JSON schema conventions: value attributes are prefixed `@`; node-child slots are prefixed `-` and hold arrays.
- SF structured types (vectors, colors, rotations) are serialized as JSON number arrays; scalars use native JSON types (numbers, booleans, strings).
- MFString is a JSON string array.
- ROUTEs, IMPORT, and EXPORT are emitted as sibling members of `-children` in the `"Scene"` object so the `JsonReader` can recover them.
- `IS` connections are emitted as an `"IS": { "connect": [...] }` member (PRF-1).
- Script inline source is emitted as `"#sourceText": [...]` (one element per line); author fields as a `"field": [...]` array member.

**CanonicalXmlWriter (X3DC14N):**
- Fully separate class with no shared code paths with `XmlWriter`.
- Attributes sorted alphabetically within each element; single-quote attribute delimiters.
- Canonical escaping: `&amp;`, `&lt;`, `&gt;`, `&apos;` (not `&quot;` — single-quoted context).
- Adds DOCTYPE line (`<!DOCTYPE X3D PUBLIC "ISO//Web3D//DTD X3D <ver>//EN" ...>`) and `xmlns:xsd` / `xsd:noNamespaceSchemaLocation` on `<X3D>`.
- Number formatting via `canonFmtFloat` / `canonFmtDouble`: shortest round-trip representation (`std::to_chars` with `chars_format::general`). Integers emit without a decimal point for floats; doubles always retain a decimal point (e.g. `0.0` not `0`) per X3DJSAIL behavior.
- See [Canonical XML Writer](../subsystems/canonical-xml.md) for the full X3DC14N page.

### Seam points

- **`runtime::X3DDocument` / `runtime::Scene`** — the root input type; writers read `rootNodes`, `protoDeclarations`, `externProtoDeclarations`, `protoInstances`, `expandedSources`, `expandedInlines`, `routes`, `imports`, `exports`, `head`.
- **`X3DNode::fields()` → `FieldTable`** — the reflection table that drives field iteration; writers call `FieldInfo::get`, `FieldInfo::isNode()`, `FieldInfo::isEnum()`, `FieldInfo::getEnumString`, `FieldInfo::isReadable()`.
- **`X3DNodeFactory::create(typeName)`** — instantiates a fresh default node for default-elision comparison; called once per type per writer instance and cached in `defaults_`.
- **`runtime::effectiveFields(script)`** — used by `XmlWriter` and `CanonicalXmlWriter` to obtain the merged static + dynamic field list for `Script` nodes, so author `<field>` declarations are included in the emit (SCR-SAI-DYN S1).
- **`runtime::dynamicFieldStore()`** — accessed by `VrmlWriter` and `JsonWriter` to retrieve author field declarations from the `DynamicFieldStore` per-node side-table.
- **`Script::getSourceCode()`** — queried by `XmlWriter`, `JsonWriter`, and `CanonicalXmlWriter` to retrieve the inline source body for re-emit. `VrmlWriter` does not call `getSourceCode()` directly; it emits the `sourceCode` field value via the generic reflection `formatValue` path.
- **`FieldValueIO::formatValue` / `parseValue`** — the single text↔value bridge; both readers and writers call this; it is the only place where `X3DFieldType` → wire-format logic lives.

## How it is tested

Tests live in `runtime/codecs/tests/`. Most are doctest cases compiled into the single `x3d_codecs_tests` ctest executable; `x3d_codec_roundtrip_audit` and `x3d_xml_script_field` are the two standalone ctest targets.

| doctest case | What it covers |
|---|---|
| `roundtrip_test` | Full XML↔parse→write round-trip: reads an X3D-XML string, re-serializes, compares output |
| `x3d_codec_roundtrip_audit` (standalone target) | Differential round-trip audit over a sample corpus; flags structural deviations |
| `vrml_mf_bracket_test` | Verifies MF values are always bracketed in ClassicVRML output (AUD-A regression) |
| `enum_quote_test` | Verifies `stripEnumQuotes` strips MFString-style quotes from enum wire values (AUD-D regression) |
| `proto_roundtrip_test` | PROTO declaration + expansion round-trips through all three writers |
| `proto_decl_body_roundtrip_test` | ProtoBody DEF/USE scope isolation across XML/VRML/JSON |
| `proto_deep_is_roundtrip_test` | `IS` connections at arbitrary nesting depth survive round-trip |
| `proto_is_json_vrml_roundtrip_test` | IS connections in JSON + VRML writers (PRF-1) |
| `proto_writer_parity_test` | All three writers produce structurally equivalent PROTO output |
| `proto_appinfo_json_roundtrip_test` | `appinfo`/`documentation` attributes on ProtoDeclare survive JSON round-trip |
| `proto_extern_url_json_roundtrip_test` | ExternProtoDeclare `url` array survives JSON round-trip |
| `proto_nested_instance_placement_roundtrip_test` | Case-A nested ProtoInstance placement inside ProtoBody (PRF-3) |
| `nested_protoinstance_roundtrip_test` | Nested ProtoInstance round-trip (scene-level, non-expanded) |
| `proto_instance_roundtrip_test` | ProtoInstance `fieldValue` overrides (scalar + node-valued) survive round-trip |
| `proto_body_defscope_test` | ProtoBody DEF scope does not leak into the surrounding scene |
| `initializeonly_read_test` | InitializeOnly fields written and re-read correctly |
| `xml_proto_capture_test` | XML reader captures PROTO body nodes and writes them back via XmlWriter |
| `x3d_xml_script_field` (standalone target) | Script `<field>` and CDATA source round-trip through XmlWriter |
| `script_cdata_audit_test` | Script CDATA inline source survives all three writers |
| `fval_extended_test` | Extended field-value IO: matrices, SFImage, MFImage, edge-case numeric formats |
| `codec_conformance_test` | Conformance spot-checks: field default elision, DEF/USE identity, enum emit |
| `codec_string_hardening_test` | The seven ENC-* string/value regressions: META escaping, unknown-escape backslash retention, SFImage over the `.x3dv` hop, `]]>` CDATA splitting, JSON `unit` array, RFC 8259 control-char escaping, canonical attr whitespace refs |

Run the codec test suite with:

```
ctest --preset dev -R x3d_codecs_tests --output-on-failure
```

Golden-file locks for the codec writers are tracked as per-milestone byte-identical assertions (historically in the deprecated `docs/superpowers/BACKLOG.md`). The conformance audit tool (`tools/corpus_audit.{hpp,cpp}`) exercises the full write→reparse cycle over the 17,719-file conformance archive as `x3d_corpus_audit` / `x3d_corpus_audit_smoke` (ctest targets in `tools/`). The `x3d_codec_roundtrip_audit` target in `runtime/codecs/tests/` is a separate, smaller differential audit over a sample corpus.

## Related specs and ADRs

- [Canonical XML Writer](../subsystems/canonical-xml.md) — the X3DC14N writer is its own page; this page covers the three primary (non-canonical) writers.
- [Codec Readers](../subsystems/parse-readers.md) — the reader subsystem that produces the `X3DDocument` these writers consume.
- [Architecture](../architecture.md) — overall system layering; writers sit at the codec boundary between the runtime model and external consumers.
- Spec: `docs/superpowers/specs/2026-06-20-x3d-canonicalize-design.md` — design for `x3d canonicalize` and the X3DC14N writer.
- Spec: `docs/superpowers/specs/2026-06-20-x3d-cli-convert-validate-design.md` — `x3d convert` CLI command (the primary writer consumer).
- Spec: `docs/superpowers/specs/2026-06-17-script-cdata-untabling-design.md` — SCR-SAI-DYN S1: Script CDATA / author-field round-trip through all writers.
- Spec: `docs/superpowers/specs/2026-06-05-dynamic-field-foundation-spec.md` — DynamicFieldStore foundation that writer seams over for author Script fields.
