---
title: Parse Readers
summary: Parse frontend — XML, VRML97, Classic VRML, and JSON readers; node builder; encoding sniffing; version-inference ladder.
tags: [subsystem, parse, readers, xml, vrml, json, version-inference]
updated: 2026-06-20
related:
  - ../architecture.md
  - proto-expand.md
  - inline-expand.md
  - codecs-writers.md
  - scene-graph.md
  - ../decisions/0007-version-inference-ladder.md
  - ../decisions/0014-dynamic-field-foundation.md
---

# Parse Readers

## Purpose

The parse frontend turns raw X3D bytes — in any of four encodings (XML, Classic VRML, VRML97, JSON) and optionally gzip-compressed — into the runtime document model (`x3d::runtime::X3DDocument`). It owns encoding detection, version inference, node and field population via the reflection layer, DEF/USE identity, ROUTE capture, PROTO/EXTERNPROTO structural capture, IMPORT/EXPORT, and Script author-field capture (`SCR-SAI-DYN S1`). After the document is built it triggers PROTO/Inline expansion through injectable resolver seams. Unknown node types and unknown field names are skipped gracefully (lenient-read policy); only genuinely unrecoverable malformation throws.

The subsystem boundary is everything under `runtime/parse/`. The entry point for consumers is `parseFile()` / `parseDocument()` in `runtime/parse/X3DParse.hpp`. The concrete reader implementations are all header-only in `namespace x3d::codec`.

## Key files

| File / directory | Role |
|---|---|
| `runtime/parse/X3DParse.hpp` | Umbrella front door: `parseFile`, `parseDocument`, `makeReader`, `stripUtf8Bom`, default `localFileProtoResolver`, default `localFileInlineResolver` |
| `runtime/parse/X3DReader.hpp` | Pure-virtual base `X3DReader` — the common interface all concrete readers implement |
| `runtime/parse/Encoding.hpp` | `Encoding` enum + `sniff`, `sniffByContent`, `sniffByExtension`, `isGzip`, `skipBomAndSpace` |
| `runtime/parse/XmlReaderAdapter.hpp` | Thin adapter wrapping `codec::XmlReader` (in `runtime/codecs/`) to satisfy the `X3DReader` interface |
| `runtime/parse/ClassicVrmlReader.hpp` | Full Classic VRML reader (ISO/IEC 19776-2); also the base class for `Vrml97Reader`; defines all dialect hooks |
| `runtime/parse/Vrml97Reader.hpp` | VRML97 reader (ISO/IEC 14772-1) — inherits `ClassicVrmlReader`, overrides the dialect hooks (`mapNodeName`, `mapFieldName`, `onHeaderLine`, `warn`) |
| `runtime/parse/Vrml97Dialect.hpp` | VRML97 → X3D name-remap table (`vrml97::mapNodeName`, `vrml97::mapFieldName`); header-only |
| `runtime/parse/JsonReader.hpp` | X3D-JSON reader — walks the Web3D X3D-JSON shape, converts JSON values to X3D wire strings, applies fields via `build::applyField` |
| `runtime/parse/NodeBuilder.hpp` | Encoding-independent build helpers shared by all text/JSON readers: `beginNode`, `applyField`, `attachChild`, `defineDef`, `resolveUse`, `collectFieldValue`; `namespace x3d::codec::build` |
| `runtime/parse/VrmlTokenizer.hpp` | Streaming VRML lexer with one-token (and two-token) lookahead; shared by `ClassicVrmlReader` and `Vrml97Reader` |
| `runtime/parse/JsonLite.hpp` | Bundled minimal JSON parser used by `JsonReader` (`x3d::json::parse`) |
| `runtime/parse/Inflate.hpp` | In-memory gzip decompression (`inflateGzip`) using `tinfl.h` (bundled); called by `parseFile` when gzip magic is detected |
| `runtime/parse/X3DProtoResolver.hpp` | `ProtoDeclarationResolver` typedef (a `std::function`) + `noopProtoResolver`; the seam for EXTERNPROTO resolution |
| `runtime/parse/tests/` | Per-encoding and per-feature unit tests (see How it is tested) |

## Interfaces and seams

### Exposed interface

The primary consumer-facing API (`namespace x3d::codec`):

```cpp
// runtime/parse/X3DParse.hpp

// Parse from a file path (sniffs encoding, inflates gzip, resolves protos/inlines).
runtime::X3DDocument parseFile(const std::string &path);

// Parse from in-memory text with optional encoding hint and resolver overrides.
runtime::X3DDocument
parseDocument(const std::string &text,
              Encoding hint = Encoding::Unknown,
              const std::string &baseUrl = "",
              const ProtoDeclarationResolver &resolver = localFileProtoResolver,
              const runtime::InlineResolver &inlineResolver = localFileInlineResolver);

// Sniff an encoding from path + content bytes (content wins when confident).
Encoding sniff(std::string_view path, std::string_view bytes); // Encoding.hpp

// Construct the concrete reader for a known encoding; null for Unknown.
std::unique_ptr<X3DReader> makeReader(Encoding enc);
```

The base reader contract:

```cpp
// runtime/parse/X3DReader.hpp, namespace x3d::codec
class X3DReader {
public:
  virtual Encoding encoding() const = 0;
  // Returns a document with scene.resolveRoutes() already applied.
  // Throws std::runtime_error only on unrecoverable malformation.
  virtual runtime::X3DDocument readDocument(const std::string &text) = 0;
};
```

Encoding classification:

```cpp
// runtime/parse/Encoding.hpp
enum class Encoding { Unknown, XML, ClassicVRML, VRML97, JSON };
// Content sniffing: inspects leading bytes after BOM/whitespace.
Encoding sniffByContent(std::string_view raw);
// Extension sniffing: .x3d->XML, .x3dv->ClassicVRML, .wrl->VRML97, .json->JSON;
// .gz/.gzip suffix stripped first.
Encoding sniffByExtension(std::string_view path);
```

### Seam points

- **EXTERNPROTO resolver** — `ProtoDeclarationResolver` (`runtime/parse/X3DProtoResolver.hpp`): a `std::function` injected into `parseDocument`. The default `localFileProtoResolver` resolves relative file-system URLs and guards against cross-file cycles with a `thread_local` active-file stack. Embedders (network fetch, virtual FS) supply their own function.

- **Inline resolver** — `runtime::InlineResolver` (`runtime/InlineExpand.hpp`): a `std::function<shared_ptr<Scene>(urls, baseUrl)>` injected into `parseDocument`. The default `localFileInlineResolver` follows the same lenient file-local pattern. Embedders override for custom asset resolution.

- **Dialect hooks on `ClassicVrmlReader`** — three protected virtual methods that `Vrml97Reader` overrides:
  - `mapNodeName(token)` — renames a node type token before the factory lookup (identity in Classic VRML; delegates to `vrml97::mapNodeName` in VRML97).
  - `mapFieldName(nodeType, token)` — renames a field token (identity in Classic VRML; applies the LOD/Switch field renames in VRML97).
  - `onHeaderLine(src, doc)` — reacts to the raw first line to set `doc.version`, `doc.profile`, or reject an unsupported encoding (e.g. VRML 1.0 throws in `Vrml97Reader`).
  - `warn(message)` — diagnostic sink; discarded by the base, collected into a `std::vector<std::string>` by `Vrml97Reader` (optional strict mode via `setStrict(true)`).

- **Node factory** — all readers instantiate nodes via `X3DNodeFactory::create(typeName)`, which is the same factory used everywhere else in the runtime. Unknown type names return null and are silently skipped.

- **Reflection / field population** — all readers set fields through the `FieldInfo` thunks exposed by `node.fields()` (the reflection `FieldTable`). `build::applyField` routes enum fields through `setEnumString` and everything else through `FieldValueIO::parseValue + set`. The `outputOnly`/`inputOnly` access guards in `applyField` skip read-only fields during parse.

- **DynamicFieldStore (S1 seam)** — both `ClassicVrmlReader` and `JsonReader` capture Script author `<field>` declarations into `runtime::dynamicFieldStore()` as `AuthorFieldDecl` entries (see `runtime/events/DynamicField.hpp`). This seam is the only parse-reader touchpoint for the Script/SAI runtime; all other nodes are fully handled by the reflection layer. Inline CDATA source is mirrored into `Script.sourceCode` by both readers so the runtime has a uniform source path.

- **ProtoBody DEF scoping** — `ClassicVrmlReader::parseProto` and `JsonReader::readJsonProtoBody` parse the proto body into a local `Scene` so body-DEFs do not leak into the enclosing document DEF table (AUD-C fix). IS-connection links are threaded through as `runtime::IsConnection` entries on `runtime::ProtoBody`.

## How it is tested

- `ctest --preset dev -R x3d_parse_reader` — multi-encoding integration smoke (XML via `XmlReaderAdapter`, Classic VRML, VRML97, JSON); round-trip invariants over small representative fixtures in `runtime/parse/tests/data/`.
- `ctest --preset dev -R x3d_parse_lex` — `VrmlTokenizer` unit tests covering punctuation, quoted strings, comment skipping, BOM handling, two-token lookahead (`encoding_lex_audit_test.cpp`).
- `ctest --preset dev -R x3d_reader_audit` — differential reader audit over the full conformance corpus (`reader_audit_test.cpp`).
- `ctest --preset dev -R x3d_version_floor` — version-inference ladder: VRML97 header floored to 3.0, sub-3.0 legacy headers, `#X3D V4` round-trips (`version_floor_test.cpp`).
- `ctest --preset dev -R x3d_lenient_read` — unknown node/field skip; outputOnly/inputOnly field guards; graceful recovery from malformed brace/bracket structure (`lenient_read_test.cpp`).
- `ctest --preset dev -R x3d_range_warnings` — out-of-range field values collected into `doc.rangeWarnings` without throwing (`range_warnings_test.cpp`).
- `ctest --preset dev -R x3d_proto_expand` — PROTO expansion integration via `parseDocument` (`proto_expand_test.cpp`).
- `ctest --preset dev -R x3d_proto_clone` — ProtoDeclaration deep-clone correctness (`proto_clone_test.cpp`).
- `ctest --preset dev -R x3d_proto_front_door` — EXTERNPROTO cross-file resolution through the default `localFileProtoResolver` (`proto_front_door_test.cpp`).
- `ctest --preset dev -R x3d_proto_nested_body` — ProtoBody DEF scoping; IS-connection capture; nested PROTO instances (`proto_nested_body_test.cpp`).
- `ctest --preset dev -R x3d_json_proto` — JSON PROTO/ExternProtoDeclare/ProtoInstance capture and round-trip (`json_proto_test.cpp`).
- `ctest --preset dev -R x3d_vrml97_proto` — VRML97-specific PROTO parsing (`vrml97_proto_test.cpp`).
- `ctest --preset dev -R x3d_proto_expand_audit` — corpus-wide PROTO expansion invariants (`proto_expand_audit_test.cpp`).
- `ctest --preset dev -R x3d_proto_nested_instance_placement_roundtrip` — parent/containerField linkage of nested ProtoInstances survives a parse → expand → re-emit round-trip.
- `ctest --preset dev -R x3d_vrml_script_field` — VRML/Classic VRML Script author-field capture into DynamicFieldStore (`vrml_script_field_test.cpp`).
- `ctest --preset dev -R x3d_json_script_field` — JSON Script author-field capture + `#sourceText` / inline-URL source extraction (`json_script_field_test.cpp`).
- `ctest --preset dev -R x3d_inline_expand` — Inline node expansion via `localFileInlineResolver` (parse-time seam) (`inline_expand_test.cpp`).
- `ctest --preset dev -R x3d_inline_roundtrip` — Inline-expanded scenes survive a write → re-parse round-trip (`inline_roundtrip_test.cpp`).
- `ctest --preset dev -R x3d_inline_carriers` — Inline load=TRUE/FALSE carrier semantics (`inline_carriers_test.cpp`).
- `ctest --preset dev -R x3d_inline_routes` — Routes from within an Inline's sub-scene resolve correctly after expansion (`inline_routes_test.cpp`).
- `ctest --preset dev -R x3d_inline_cycle` — Inline self-reference / mutual cycle terminates (thread-local active-file guard) (`inline_cycle_test.cpp`).
- `ctest --preset dev -R x3d_inline_containment_cycle` — Containment-cycle defense-in-depth post inline expansion (`inline_containment_cycle_test.cpp`).

Test data fixtures (`.x3d`, `.x3dv`, `.wrl`, `.json`, `.gz` samples) live in `runtime/parse/tests/data/`.

## Related specs and ADRs

- [ADR-0007: Version-inference ladder](../decisions/0007-version-inference-ladder.md)
- [ADR-0014: Dynamic-field foundation](../decisions/0014-dynamic-field-foundation.md)
- Spec: `docs/superpowers/specs/2026-06-13-m3-versioning-design.md` — version-inference ladder design (VP-2)
- Spec: `docs/superpowers/specs/2026-06-05-dynamic-field-foundation-spec.md` — Script author-field DynamicFieldStore seam
- Spec: `docs/superpowers/specs/2026-06-17-script-cdata-untabling-design.md` — inline CDATA / `#sourceText` source capture
- Spec: `docs/superpowers/specs/2026-06-19-inline-expansion-design.md` — parse-time Inline expansion seam
- Sibling subsystem: [Proto Expand](proto-expand.md) — PROTO/EXTERNPROTO expansion pass invoked by `parseDocument` after parsing
- Sibling subsystem: [Inline Expand](inline-expand.md) — Inline expansion pass invoked by `parseDocument` after PROTO expansion
- Sibling subsystem: [Codecs Writers](codecs-writers.md) — the write side (the XML reader lives in `runtime/codecs/XmlReader.hpp` and is wrapped here by `XmlReaderAdapter`)
- Sibling subsystem: [Scene Graph](scene-graph.md) — the `X3DDocument` / `Scene` / DEF-table types that parse readers populate
