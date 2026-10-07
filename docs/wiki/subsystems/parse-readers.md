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

**Input-hardening (DoS guards).** The recursive-descent readers descend
attacker-controlled structure, so each caps nesting depth against a shared
ceiling (`runtime/RecursionLimits.hpp`, `x3d::kMaxNestingDepth` = 1000): the XML
tokenizer (`parseElement`↔`parseContent`), JSON (`parseValue`↔`parseArray`/
`parseObject`), and ClassicVRML/VRML97 (`parseNode`↔`parseNodeBody`) throw
`std::runtime_error` past the cap instead of overrunning the native stack
(SEC-1). Independently, `SFImage`/`PixelTexture` pixel parsing clamps
`numComponents` to the spec range `[0,4]` before unpacking, so a hostile token
cannot trigger a signed-shift overflow or a multi-GB allocation (SEC-2).
Legitimate documents nest far below the cap and are unaffected.

The file front door is hardened against the same adversary. Gzip input is
inflated with a decompressed-size ceiling (`kMaxDecompressedBytes`, default
256 MiB) clamping both the initial reservation and the grow loop, so a
decompression bomb or a forged ISIZE footer cannot drive an OOM (SEC-5,
`Inflate.hpp`). The default `Inline`/`EXTERNPROTO` resolvers canonicalize a
file-like `url` and confine it to a configurable root — `parseFile`/
`parseDocument` default it to the source file's own directory (a secure default
for untrusted input: absolute urls and `../` cross-directory reads like
`url='/etc/passwd'` are rejected, SEC-3), while a tool parsing a **trusted** tree
passes a wider `confineRoot` so legitimate `../` references within it resolve
(the conformance CLI gate does this with the corpus root). Either way the
canonical path keys the cross-file cycle guard, so spelling aliases (`./a.x3d`
vs `a.x3d`) cannot defeat it (SEC-4). See `runtime/parse/PathConfine.hpp` and
[ADR-0038](../decisions/0038-local-resolver-path-confinement.md). Embedders
needing remote/non-filesystem access supply their own resolver.

The subsystem boundary is everything under `runtime/parse/`. The entry point for consumers is `parseFile()` / `parseDocument()` in `runtime/parse/X3DParse.hpp`. The concrete reader implementations are all header-only in `namespace x3d::codec`.

Readers record successfully assigned scalar field names in
`Scene::authoredScalarFields`, keyed by weak node identity. This keeps an
explicit value distinct from an absent field even when the value equals the
generated default; DEF/USE shares one identity. PROTO readers transfer the
declaration-local scene's interface and body marks to `ProtoDeclaration::authoredScalarFields`, so
an externally resolved declaration keeps its marks after its source document
is destroyed. Unknown and read-only fields are not recorded.

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
| `runtime/X3DAuthoredScalarFields.hpp` | Weak node-identity field presence shared by scenes and PROTO declarations |
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

- **Inline resolver** — `runtime::InlineResolver` (`runtime/InlineExpand.hpp`): a `std::function<shared_ptr<Scene>(urls, baseUrl)>` injected into `parseDocument`. The default `localFileInlineResolver` follows the same lenient file-local pattern. Embedders override for custom asset resolution and pass that resolver plus the base URL to the runtime when deferred `load=TRUE` events should load content.

### Recursive asset loader policy

`assetResolversFrom(AssetResolver, Encoding)` in
`runtime/parse/AssetDocumentResolvers.hpp` returns `.proto` and `.inlineScene`.
Pass both to `parseDocument` to retain one synchronous asset policy across every
Inline/EXTERNPROTO document boundary. Nested parsing never enables the default
local-file loader. Backends receive fragment-free URLs resolved against the
source document's directory, including nested relative references. The helper
normalizes dot segments for cycle identity while preserving the authority,
query/fragment bytes, opaque identifiers and significant empty path segments.
Terminal `/.` and `/..` retain directory semantics (including authority and
relative roots), so documents fetched at directory URLs resolve their nested
references against that same directory. It performs no
filesystem or network I/O itself.

```cpp
const auto loaders = x3d::codec::assetResolversFrom(myAssets);
auto doc = x3d::codec::parseDocument(text, encoding, sourceDirectory,
                                    loaders.proto, loaders.inlineScene);
```

`protoResolverFrom(asset, hint)` remains available in `AssetProtoResolver.hpp`;
its nested Inline loads now use the same asset backend rather than an implicit
local-file default. `inlineResolverFrom(asset, hint)` in
`AssetInlineResolver.hpp` supplies the symmetric loader. To retain an independent
opposite policy, use `protoResolverFrom(asset, hint, inlinePolicy)` or
`inlineResolverFrom(asset, hint, protoPolicy)`. An explicitly empty opposite
callback denies those loads. Independently supplied callbacks are opaque: pass
the intended override to the helper as well as the top-level parser, or use the
paired factory. No global or thread-local loader policy is installed.

Failed/unparseable candidates remain lenient and try the next URL. A Pending
asset result aborts the entire active recursive asset load, including parent
candidate lists; the parser records its ordinary unresolved Inline/EXTERNPROTO
warning. Backend exceptions are treated as failed candidates. Cross-kind
cycles share an explicit per-call active-URL stack; a new call starts clean.

The front door also captures source-directory provenance separately from
serialized values. `ProtoDeclaration::sourceBaseUrl` and
`ExternProtoDeclaration::sourceBaseUrl` survive cached/fetched declarations;
body/default clones retain that directory in `Scene::nodeBaseUrls`. URL values
forwarded by IS retain the supplying fieldValue's source directory. Consequently
an imported prototype's body URLs resolve against its library while an authored
URL override resolves against the caller. Authored URL strings are unchanged.
The local-file Inline resolver uses the same confinement/canonicalization helper
as EXTERNPROTO; rejected absolute/escaping includes never open the target.

The front door snapshots each document's authored `head.units` onto its `Scene::sourceUnits` and locally authored `ProtoDeclaration::sourceUnits` before expansion. Child Scenes returned by Inline and declarations selected by file or asset EXTERNPROTO resolution therefore carry their own source UNIT declarations; an empty snapshot means that source declared none. These runtime snapshots do not change `Head` serialization or numeric field values.

- **Dialect hooks on `ClassicVrmlReader`** — three protected virtual methods that `Vrml97Reader` overrides:
  - `mapNodeName(token)` — renames a node type token before the factory lookup (identity in Classic VRML; delegates to `vrml97::mapNodeName` in VRML97).
  - `mapFieldName(nodeType, token)` — renames a field token (identity in Classic VRML; applies the LOD/Switch field renames in VRML97).
  - `onHeaderLine(src, doc)` — reacts to the raw first line to set `doc.version`, `doc.profile`, or reject an unsupported encoding (e.g. VRML 1.0 throws in `Vrml97Reader`).
  - `warn(message)` — diagnostic sink; discarded by the base, collected into a `std::vector<std::string>` by `Vrml97Reader` (optional strict mode via `setStrict(true)`).

- **Node factory** — all readers instantiate nodes via `X3DNodeFactory::create(typeName)`, which is the same factory used everywhere else in the runtime. Unknown type names return null; the element is skipped, and the reader records a `ReaderWarning{Kind::UnknownNode}` in `X3DDocument.readerWarnings` so `x3d validate` reports it instead of silently deleting author content (DIAG-UNKNOWN-NODE). In XML, a `<ROUTE>` nested among a node's children is not a node: it is added to the enclosing scene's (or PROTO body's) routes, and a nested `<IS>` is skipped there because the IS pass reads it (before 2026-09-26 nested ROUTEs were silently dropped; `nested_route_xml_is_kept`).

- **Reader-recovery diagnostics** — `X3DDocument.readerWarnings` (`runtime/X3DProto.hpp`) is the structured channel for lenient-read recoveries that are not legal X3D: an unknown/misspelled node element (`UnknownNode`) and an unknown `profile=` token coerced to `Interchange` (`ProfileCoerced`, DIAG-PROFILE-COERCE). Every reader (`XmlReader`, `ClassicVrmlReader`/`Vrml97Reader`, `JsonReader`) fills it; `cmdValidate` and the cli-gate validate path surface each as a `node`/`profile` diagnostic. It parallels `rangeWarnings`/`protoWarnings`/`inlineWarnings`.

- **Profile token preservation** — `X3DDocument::setProfileToken` records the authored `profile=` spelling verbatim in `profileRaw` (exposed via `profileToken()`), resolves it to a `Profile` for profile-fit, and diagnoses a non-canonical token. The XML/JSON/VRML writers emit `profileToken()`, so a round-trip never rewrites the declared conformance class (DIAG-PROFILE-COERCE).

- **UNIT header validation** — `parseDocument` checks UNIT declarations before scene expansion for all supported X3D encodings. Declarations require X3D 3.3 or later, one of `angle`/`force`/`length`/`mass`, no duplicate category, a nonempty name without whitespace, and a finite positive conversion factor. An invalid declaration throws; valid authored factors and field values remain unchanged for round-tripping. Runtime conversion is tracked separately as REQ-UNIT. Runtime entry converts explicitly authored `TextureTransform.rotation` through the angle UNIT factor, including values forwarded via IS; UV center, translation and scale remain dimensionless. The focused `UNIT runtime: TextureTransform*` cases in `runtime_session_test.cpp` cover defaults, repeated initialization and subsequent radian writes.

- **PROTO built-in-shadow quarantine** — after `readDocument`, `parseDocument` runs `quarantineBuiltinShadowingProtos` (`runtime/parse/X3DParse.cpp`): any `ProtoDeclare`/`ExternProtoDeclare` whose name is a built-in (`X3DNodeFactory::registry()`) is dropped and recorded as `ProtoWarning{Kind::BuiltinShadow}`, so the built-in keeps precedence (ADR-0033, `PROTO-SHADOW`). Applies uniformly across all four encodings from the single front door; lenient by default.

- **Reflection / field population** — all readers set fields through the `FieldInfo` thunks exposed by `node.fields()` (the reflection `FieldTable`). `build::applyField` routes enum fields through `setEnumString` and everything else through `FieldValueIO::parseValue + set`. The `outputOnly`/`inputOnly` access guards in `applyField` skip read-only fields during parse.

- **DynamicFieldStore (S1 seam)** — `ClassicVrmlReader`, `JsonReader`, and `XmlReader` capture author `<field>` declarations into `runtime::dynamicFieldStore()` as `AuthorFieldDecl` entries (see `runtime/events/DynamicField.hpp`). This covers every `X3DProgrammableShaderObject`: `Script` and — since the Phase-3 ComposedShader plumbing — `ComposedShader` (its `<field>` uniforms). This seam is the parse-reader touchpoint for the Script/SAI and author-shader runtimes; all other nodes are fully handled by the reflection layer. Inline `<![CDATA[...]]>` source is mirrored into the node's `sourceCode` slot — `Script.sourceCode` for scripts, and `ShaderPart.sourceCode` for a ComposedShader's GLSL stages — so the runtime has a uniform source path.

- **PROTO body order** — direct body declarations, nodes and instances are recorded in `ProtoBody::statements`. The statement records retain unused local/extern declarations and the point at which a nested declaration shadows an inherited name. Node and instance values remain in the existing body collections. Ordinary body-node children and declarations are also recorded in `ProtoBody::nodeStatements`, keyed by weak parent identity. Reflected fields and `nestedInstances` remain the value authority. Ordinary interface-default node graphs share this context. Direct ProtoInstance defaults occupy `nodeDefault` as authored `ProtoInstanceTemplate` nodes. The local DEF table resolves USE references to the same template across fields and body graphs; these defaults do not become direct body instances. XML, Classic and JSON also capture direct and contained ProtoInstances in SFNode/MFNode fieldValues as bound templates in the owning value graph. All scene structural instances, named or unnamed, occupy an authored template slot in `rootNodes` or their ordinary parent's node field. The structural record's weak `placementTemplate` link preserves the exact identity; expansion replaces the slot without appending it after ordinary nodes. USE references reuse the same expanded primary. Raw writers read source values from the linked structural record while retaining slot order.

- **PROTO DEF scoping** — XML, JSON, and Classic readers parse each declaration's interface defaults and body through one local `Scene`. DEF/USE aliases therefore resolve across fields and into the body, while outer scene DEFs and nested PROTO DEFs remain separate (§4.4.4.4). Previously declared PROTO/EXTERNPROTO declarations remain available for nested instances; a same-name nested declaration shadows the inherited name across both declaration kinds without changing the enclosing scene. The declaration carries authored scalar-field marks from both interface and body after the local scene is consumed. IS-connection links are threaded through as `runtime::IsConnection` entries on `runtime::ProtoBody`.

## How it is tested

- `ctest --preset dev -R x3d_parse_reader` — multi-encoding integration smoke (XML via `XmlReaderAdapter`, Classic VRML, VRML97, JSON); round-trip invariants over small representative fixtures in `runtime/parse/tests/data/`.
- `ctest --preset dev -R x3d_parse_tests` (doctest case: `encoding_lex_audit_test`) — `VrmlTokenizer` unit tests covering punctuation, quoted strings, comment skipping, BOM handling, two-token lookahead (`encoding_lex_audit_test.cpp`).
- `ctest --preset dev -R x3d_parse_tests` (doctest case: `reader_audit_test`) — differential reader audit over the full conformance corpus (`reader_audit_test.cpp`).
- `ctest --preset dev -R x3d_parse_tests` (doctest case: `version_floor_test`) — version-inference ladder: VRML97 header floored to 3.0, sub-3.0 legacy headers, `#X3D V4` round-trips (`version_floor_test.cpp`).
- `ctest --preset dev -R x3d_parse_tests` (doctest case: `lenient_read_test`) — unknown node/field skip; outputOnly/inputOnly field guards; graceful recovery from malformed brace/bracket structure (`lenient_read_test.cpp`).
- `ctest --preset dev -R x3d_parse_tests` (doctest cases: `unit_header_validation_all_encodings`, `proto_shadow_*`, `unknown_node_*`, `profile_*`, `import_export_wire_*`) — UNIT header rejection, PROTO built-in-shadow quarantine, unknown-node warnings, profile-token preservation, and IMPORT→Inline-exported-DEF route wiring (`core_diagnostics_test.cpp`).
- `ctest --preset dev -R x3d_parse_tests` (doctest case: `range_warnings_test`) — out-of-range field values collected into `doc.rangeWarnings` without throwing (`range_warnings_test.cpp`).
- `ctest --preset dev -R x3d_parse_tests` (doctest case: `proto_expand_test`) — PROTO expansion integration via `parseDocument` (`proto_expand_test.cpp`).
- `ctest --preset dev -R x3d_parse_tests` (doctest case: `proto_clone_test`) — ProtoDeclaration deep-clone correctness (`proto_clone_test.cpp`).
- `ctest --preset dev -R x3d_proto_front_door` — EXTERNPROTO cross-file resolution through the default `localFileProtoResolver` (`proto_front_door_test.cpp`).
- `ctest --preset dev -R x3d_parse_tests` (doctest case: `proto_nested_body_test`) — ProtoBody DEF scoping; IS-connection capture; nested PROTO instances (`proto_nested_body_test.cpp`).
- `ctest --preset dev -R x3d_parse_tests` (doctest case: `json_proto_test`) — JSON PROTO/ExternProtoDeclare/ProtoInstance capture and round-trip (`json_proto_test.cpp`).
- `ctest --preset dev -R x3d_parse_tests` (doctest case: `vrml97_proto_test`) — VRML97-specific PROTO parsing (`vrml97_proto_test.cpp`).
- `ctest --preset dev -R x3d_proto_expand_audit` — corpus-wide PROTO expansion invariants (`proto_expand_audit_test.cpp`).
- `ctest --preset dev -R x3d_codecs_tests` (doctest case: `proto_nested_instance_placement_roundtrip_test`) — parent/containerField linkage of nested ProtoInstances survives a parse → expand → re-emit round-trip.
- `ctest --preset dev -R x3d_vrml_script_field` — VRML/Classic VRML Script author-field capture into DynamicFieldStore (`vrml_script_field_test.cpp`).
- `ctest --preset dev -R x3d_json_script_field` — JSON Script author-field capture + `#sourceText` / inline-URL source extraction (`json_script_field_test.cpp`).
- `ctest --preset dev -R x3d_parse` (doctest cases: `asset policy:*`) — recursive paired and explicitly rejecting policies, both Inline/EXTERNPROTO directions, relative source bases, cached fragment selection, caller IS URL overrides, cycles, Pending/Failed/throwing backends, and local confinement (`asset_loader_policy_test.cpp`). Fixtures are generated under the test working directory; on Linux, in-process file-event watches verify that the rejecting paths never open/read the local decoys, with a permitted local read as a positive control. No live network backend is used.
- `ctest --preset dev -R x3d_parse_tests` (doctest case: `inline_expand_test`) — Inline node expansion via `localFileInlineResolver` (parse-time seam) (`inline_expand_test.cpp`).
- `ctest --preset dev -R x3d_inline_roundtrip` — Inline-expanded scenes survive a write → re-parse round-trip (`inline_roundtrip_test.cpp`).
- `ctest --preset dev -R x3d_parse_tests` (doctest case: `inline_carriers_test`) — Inline load=TRUE/FALSE carrier semantics (`inline_carriers_test.cpp`).
- `ctest --preset dev -R x3d_inline_routes` — Routes from within an Inline's sub-scene resolve correctly after expansion (`inline_routes_test.cpp`).
- `ctest --preset dev -R x3d_inline_cycle` — Inline self-reference / mutual cycle terminates (thread-local active-file guard) (`inline_cycle_test.cpp`).
- `ctest --preset dev -R x3d_inline_containment_cycle` — Containment-cycle defense-in-depth post inline expansion (`inline_containment_cycle_test.cpp`).
- `ctest --preset dev -R x3d_codecs_tests` (doctest cases: `xml_*_nesting`, `sfimage_*`) — XML deep-nesting cap (SEC-1) and `SFImage` `numComponents` clamp (SEC-2) (`xml_depth_guard_test.cpp`, `sfimage_overflow_test.cpp`).
- `ctest --preset dev -R x3d_parse_tests` (doctest cases: `json_*_nesting`, `vrml_*_nesting`) — JSON and ClassicVRML deep-nesting caps reject pathological input and keep legitimate nesting (SEC-1) (`parser_depth_guard_test.cpp`).
- `ctest --preset dev -R x3d_inflate_bomb` — gzip decompressed-size cap: output past `maxOut` throws while a legitimate stream still inflates (SEC-5) (`inflate_bomb_test.cpp`).
- `ctest --preset dev -R x3d_path_confine` — `confineLocalIncludePath` blocks absolute/`../`-escape Inline/EXTERNPROTO urls and de-aliases the cycle-guard key (SEC-3/SEC-4) (`path_confine_test.cpp`).
- `cmake --preset fuzz && ./build-fuzz/x3d_parse_fuzz` — libFuzzer harness (`parse_fuzz.cpp`) drives `sdk::parseDocument` with mutated bytes across all four encodings under ASan + UBSan; asserts the "never panic" contract (no crash/leak/UB on any input). Built Clang-only; the `cpp-fuzz` CI job runs a bounded smoke. See [Build and mise tasks → Sanitizer and fuzz gates](../guides/build-and-mise.md#sanitizer-and-fuzz-gates-bld-2).

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
