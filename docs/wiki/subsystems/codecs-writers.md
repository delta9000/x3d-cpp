---
title: Codec Writers
summary: XML, VRML, and JSON serialization writers plus field-value IO — the full set of output codecs.
tags: [subsystem, codecs, writers, xml, vrml, json, serialization]
updated: 2026-11-05
related:
  - ../architecture.md
  - ../subsystems/canonical-xml.md
  - ../subsystems/parse-readers.md
---

# Codec Writers

## UNIT normalization safety

Parsed documents retain authored UNIT declarations and values for round-trip
serialization. Runtime entry converts authored dimensional fields in place.
Until conversion-aware output can also reconcile PROTO interfaces and canonical
built-in defaults, document writers (and XML scene output) throw if nonidentity
UNIT normalization has occurred. Serialize before runtime entry, or reparse the
source into an independent document. A shallow Scene/document copy shares nodes
and is not an independent authoring snapshot. Identity conversions and untouched
built-in defaults do not trigger this guard. Node-only writer APIs lack Scene
provenance and do not provide this document-level protection.

## Purpose

The codec writers subsystem converts an in-memory `x3d::runtime::X3DDocument` (or a `Scene` / single node) into one of the three X3D encoding wire formats: X3D-XML (`.x3d`), ClassicVRML (`.x3dv`), and X3D-JSON (`.x3dj`). A fourth writer, `CanonicalXmlWriter`, produces the deterministic X3D Canonical Form (X3DC14N) used by the differential conformance gate. All four writers are driven purely by the generated reflection `FieldTable` and the `X3DNodeFactory`; none contain per-node code. The shared field-value formatting kernel (`FieldValueIO.hpp`) is the bidirectional text↔`std::any` bridge shared by the writers and the codec readers alike.

The writers sit between the runtime document model and any consumer that needs a serialized representation — the `x3d convert` CLI command, the round-trip conformance auditor, golden-file regression fixtures, and the canonical-form differencer.

PROTO serialization preserves `IS/connect` mappings and node-valued interface defaults across XML, JSON, and ClassicVRML. ClassicVRML output supplies type-default values when initializeOnly/inputOutput PROTO interface or node author-field declarations have no stored value, as required by the encoding grammar; EXTERNPROTO interface declarations carry no values. Declaration and field `appinfo`/`documentation` are stored on `ProtoDeclaration`, `ExternProtoDeclaration`, and `ProtoField`, and preserved in XML, canonical XML, and JSON. ClassicVRML carries them in the versioned x3d-cpp comment convention from [ADR-0062](../decisions/0062-classic-prototype-metadata-comments.md). Standard-only consumers may discard those comments; this is an SDK round-trip convention, not an ISO metadata production.

ClassicVRML can serialize a declaration that splits one node field after an
ordinary node prefix by moving the declaration before that prefix. It preserves
child order and sharing, and refuses this rewrite when the prefix contains a
prototype instance or declaration. See [PROTO expansion](proto-expand.md) for
the remaining cross-encoding ordering limits.

## Key files

| File | Role |
|---|---|
| `runtime/codecs/X3DCodecs.hpp` | Umbrella include that pulls in all codecs (writers + readers + XmlLite) |
| `runtime/codecs/XmlWriter.{hpp,cpp}` | Runtime model → X3D-XML; public entry points and ordinary renderer in `x3d::codec` |
| `runtime/codecs/VrmlWriter.hpp` | Runtime model → ClassicVRML (X3D ClassicVRML / `.x3dv`); definitions in `VrmlWriter.cpp` |
| `runtime/codecs/JsonWriter.hpp` | Runtime model → X3D-JSON; definitions in `JsonWriter.cpp` |
| `runtime/codecs/CanonicalXmlWriter.{hpp,cpp}` | X3DC14N entry point, renderer, and value-formatting helpers |
| `runtime/codecs/XmlGraphBuilder.{hpp,cpp}` | Shared XML graph construction; explicit value-format/default-comparison and child/attribute-order policies |
| `runtime/codecs/FieldValueIO.hpp` | Bidirectional field-value IO: `formatValue` (`std::any` → wire string) and `parseValue` (wire string → `std::any`), all type-switched on `X3DFieldType`; also low-level helpers (`fmtFloat`, `parseFloat`, `tokenize`, `parseMFString`, …). Floats/doubles serialize in shortest round-trip form (`std::to_chars`): authored digits like `0.9` survive a round-trip verbatim instead of expanding to `0.899999976`, and `parse(format(v)) == v` bit-exactly (pinned by `fval_extended_test`). `CanonicalXmlWriter`'s `canonFmtFloat` is deliberately separate — X3DC14N owns its own pinned formatting contract. |
| `runtime/codecs/JsonString.hpp` | Shared JSON string escaping for JsonWriter and the two metadata strings in ClassicVRML comments; control characters never create extra comment lines |
| `runtime/codecs/ProtoNameMaps.hpp` | `fieldTypeName()` and `accessTypeName()` — enum → string helpers shared by the writers; `fieldTypeFromName()` inverts `fieldTypeName()` for the XML, ClassicVRML and JSON readers; `accessTypeFromName()` accepts canonical names and VRML97 aliases |
| `runtime/codecs/XmlLite.hpp` | Lightweight hand-written XML element tree and renderer used by `XmlWriter`, `CanonicalXmlWriter`, and `XmlReader` |
| `runtime/codecs/XmlReader.hpp` | X3D-XML → runtime model (counterpart reader; out of scope for this page — see [Codec Readers](../subsystems/parse-readers.md)) |
| `runtime/codecs/tests/` | Per-writer and cross-writer round-trip tests |

The XML writers, Classic writer, and XML/Classic readers use
`ProtoInstance::interfaceField()` for interface lookup. A local declaration
takes precedence over an external declaration, even when the requested field
is absent locally. JSON retains its existing external-field fallback.
`x3d::codec::isMF` classifies multiple-value wire types for the Classic reader and writer; the writer also
handles MFNode structurally.

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
    std::string writeNode(const std::shared_ptr<X3DNode> &node,
                          const runtime::DynamicFieldStore *authorFields = nullptr);
    // Supply the owner for author fields; omitted owner means generated fields
    // plus Script source only. Full document/Scene writers select it automatically.
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
// before matching against bare token identifiers.
std::string stripEnumQuotes(const std::string &wire);
```

### Serialization strategy common to all writers

Every writer applies the same three-phase strategy, driven entirely by reflection — no per-node code anywhere in the subsystem:

1. Default elision. For each field, a fresh factory instance of the node's type is obtained via `X3DNodeFactory::create(typeName)` and cached. Fields whose current value formats identically to the factory default are omitted, producing clean output.
2. DEF/USE deduplication. Node identity (raw pointer) is tracked in an `unordered_set<const X3DNode *>`. The first emission of a shared node writes a `DEF`; all subsequent references emit a `USE` reference with no fields or children.
3. PROTO / Inline round-trip redirect. Before emitting a node the writer checks `scene.expandedSources` (for expanded `ProtoInstance`s) and `scene.expandedInlines` (for expanded `Inline` nodes). If a match is found, the original captured source structure is re-emitted instead of the expansion, so the written output can be re-parsed to re-expand rather than serializing the expanded tree.

Unknown nodes. `XmlGraphBuilder` re-emits a `runtime::UnknownNode` exactly as the XML reader kept it: authored attributes in order, its own `containerField`, character data and children, plus any entries in `Scene::opaqueChildren` for the parent. XML to XML conversion of a file with X3D 4.1 or extension nodes is therefore lossless and stable. The ClassicVRML and JSON writers have no schema for those attributes; they write only the type name and DEF (READ-UNKNOWN-NODE-PASSTHROUGH).

The three ordinary writers emit the document's `profile` from `X3DDocument::profileToken()` (`runtime/X3DDocument.hpp`): the authored spelling when a reader preserved one, else the canonical name for the resolved `Profile`. An unknown/misspelled `profile=` token therefore round-trips unchanged instead of being silently rewritten to `Interchange` (DIAG-PROFILE-COERCE); the reader still resolves it to `Interchange` for profile-fit.

The two XML writers use `XmlGraphBuilder` for scene traversal, Inline and
ProtoInstance redirection, DEF/USE, node and author fields, declarations, and IS
attachment. `XmlGraphPolicy` selects the formatter used for both retained values
and factory-default comparison, authored versus declaration child-field order,
and insertion versus alphabetical attribute order. Each serialization owns its
builder state and default cache. Document headers and renderers remain separate;
canonical XML retains `profileName()` and the unmodified version token, while
ordinary XML uses `profileToken()` and `headerVersion()`.

### Encoding-specific behaviors

XmlWriter:
- Value fields become XML attributes; `SFNode`/`MFNode` fields become child elements under the child's `containerField` name.
- Enum fields use the `FieldInfo::getEnumString` thunk.
- A `Script` node's inline source is emitted as a `<![CDATA[...]]>` text body rather than an attribute; author `<field>` declarations from the `DynamicFieldStore` are emitted as `<field>` child elements. A literal `]]>` in the source is split across consecutive CDATA sections (`xml::cdataEscape`; the reader concatenates them back — ENC-CDATA-SCRIPT).
- ProtoBody re-emit uses a fresh `XmlGraphBuilder` with the same policy and isolated DEF/USE bookkeeping; `IS` connections are threaded via `bodyIsc_` so `<IS><connect>` blocks appear at any nesting depth inside the body.
- Un-expanded `ProtoInstance`s that were not resolved (no graph node) are re-emitted from `scene.protoInstances` so they survive a round-trip.

VrmlWriter:
- Emits `#X3D V<version> utf8` header; sub-3.0 version tokens are floored to `3.0` (VP-2 conformance) by the shared `codec::headerVersion` (`VersionHeader.hpp`), which the XML and JSON writers also use for their `version` attribute.
- MF values are always bracketed with `[... ]` to satisfy the ClassicVRML grammar (ISO/IEC 19776-2 `mfValue ::= '['... ']'` — bare runs truncate on reparse).
- `SFBool`/`MFBool` tokens are uppercased to `TRUE`/`FALSE` (VRML convention, not XML/JSON).
- `SFString` values are double-quoted with `\"` / `\\` escaping; `MFString` self-quotes via `fmtMFString`.
- Author Script field declarations (from the `DynamicFieldStore`) are re-emitted as `accessType FieldType name [default]` interface lines inside the node body (Task B).
- `IS` connections are emitted as `nodeField IS protoField` lines inside the body node's braces (PRF-1).
- Case-A nested ProtoInstances inside a ProtoBody are injected into the parent body node's braces via `writeVrmlNestedFor` (PRF-3).

JsonWriter:
- Follows Web3D X3D-JSON schema conventions: value attributes are prefixed `@`; node-child slots are prefixed `-` and hold arrays.
- SF structured types (vectors, colors, rotations) are serialized as JSON number arrays; scalars use native JSON types (numbers, booleans, strings).
- MFString is a JSON string array.
- ROUTEs, IMPORT, and EXPORT are emitted as sibling members of `-children` in the `"Scene"` object so the `JsonReader` can recover them.
- `IS` connections are emitted as an `"IS": { "connect": [...] }` member (PRF-1).
- Script inline source is emitted as `"#sourceText": [...]` (one element per line); author fields as a `"field": [...]` array member.

CanonicalXmlWriter (X3DC14N):
- Shares semantic graph construction with `XmlWriter`; keeps a separate renderer and explicit canonical policies, as amended in [ADR-0008](../decisions/0008-canonicalize-source-provenance.md).
- Attributes sorted alphabetically within each element; single-quote attribute delimiters.
- Canonical escaping: `&amp;`, `&lt;`, `&gt;`, `&apos;` (not `&quot;` — single-quoted context).
- Adds DOCTYPE line (`<!DOCTYPE X3D PUBLIC "ISO//Web3D//DTD X3D <ver>//EN"...>`) and `xmlns:xsd` / `xsd:noNamespaceSchemaLocation` on `<X3D>`.
- Number formatting via `canonFmtFloat` / `canonFmtDouble`: shortest round-trip representation (`std::to_chars` with `chars_format::general`). Integers emit without a decimal point for floats; doubles always retain a decimal point (e.g. `0.0` not `0`) per X3DJSAIL behavior.
- See [Canonical XML Writer](../subsystems/canonical-xml.md) for the full X3DC14N page.

### Seam points

- `runtime::X3DDocument` / `runtime::Scene` — the root input type; writers read `rootNodes`, `protoDeclarations`, `externProtoDeclarations`, `protoInstances`, `expandedSources`, `expandedInlines`, `routes`, `imports`, `exports`, `head`.
- `X3DNode::fields()` → `FieldTable` — the reflection table that drives field iteration; writers call `FieldInfo::get`, `FieldInfo::isNode()`, `FieldInfo::isEnum()`, `FieldInfo::getEnumString`, `FieldInfo::isReadable()`.
- `X3DNodeFactory::create(typeName)` — instantiates a fresh default node for default-elision comparison; cached in `defaults_`; for the XML writers, the cache belongs to the per-serialization graph builder.
- `runtime::effectiveFields(script, store)` — used by `XmlWriter` and `CanonicalXmlWriter` to obtain the merged static + dynamic field list for `Script` nodes, so author `<field>` declarations are included in the emit.
- Scene/declaration author owner — `VrmlWriter` and `JsonWriter` retrieve declarations from the current `Scene::authorFields` or `ProtoDeclaration::authorFields` store. All four body writers switch to the declaration owner. Bare-node XML writing accepts an optional explicit store; without one it emits generated fields and Script source only. See [ADR-0057](../decisions/0057-scene-owned-author-fields.md).
- `Script::getSourceCode()` — queried by `XmlWriter`, `JsonWriter`, and `CanonicalXmlWriter` to retrieve the inline source body for re-emit. `VrmlWriter` does not call `getSourceCode()` directly; it emits the `sourceCode` field value via the generic reflection `formatValue` path.
- `FieldValueIO::formatValue` / `parseValue` — the single text↔value bridge; both readers and writers call this; it is the only place where `X3DFieldType` → wire-format logic lives.

## Related

- [Canonical XML Writer](../subsystems/canonical-xml.md)
- [Codec Readers](../subsystems/parse-readers.md)
- [Architecture](../architecture.md)
