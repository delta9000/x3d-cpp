---
title: Parse Readers
summary: Parse frontend — XML, VRML97, Classic VRML, and JSON readers; node builder; encoding sniffing; version-inference ladder.
tags: [subsystem, parse, readers, xml, vrml, json, version-inference]
updated: 2026-11-11
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

## Fetched-source diagnostics proposal

The [fetched-document diagnostics proposal](../decisions/0072-fetched-document-diagnostics.md)
preserves warnings from selected asset-loaded child documents as source-qualified
`SourceWarning` details in the enclosing document's Inline/PROTO warning channel.
Top-level reader and range diagnostics keep their existing structured channels.
For an EXTERNPROTO only the selected declaration carries the fetched document's
warnings, and expansion reports each one once per warning collection however
many instances resolve it. The registered `inline_diagnostics_test.cpp` cases run
in `x3d_extract_tests`.

## Purpose

The parse frontend turns raw X3D bytes — in any of four encodings (XML, Classic VRML, VRML97, JSON) and optionally gzip-compressed — into the runtime document model (`x3d::runtime::X3DDocument`). It owns encoding detection, version inference, node and field population via the reflection layer, DEF/USE identity, ROUTE capture, PROTO/EXTERNPROTO structural capture, IMPORT/EXPORT, and Script author-field capture (` S1`). After the document is built it triggers PROTO/Inline expansion through injectable resolver seams. Unknown node types and unknown field names are skipped gracefully (lenient-read policy); only unrecoverable malformation throws.

Input-hardening (DoS guards). The recursive-descent readers descend
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
file-like `url` and confine it to a configurable root — `parseFile()`/
`parseDocument()` default it to the source file's own directory (a secure default
for untrusted input: absolute urls and `../` cross-directory reads like
`url='/etc/passwd'` are rejected, SEC-3), while a tool parsing a trusted tree
passes a wider `confineRoot` so legitimate `../` references within it resolve
(the conformance CLI gate does this with the corpus root). Either way the
canonical path keys the cross-file cycle guard, so spelling aliases (`./a.x3d`
vs `a.x3d`) cannot defeat it (SEC-4). See `runtime/parse/PathConfine.hpp` and
[ADR-0038](../decisions/0038-local-resolver-path-confinement.md). Embedders
needing remote/non-filesystem access supply their own resolver.

The front doors also share a synchronous per-thread parse session across nested
resolver calls. `ParseLimits` defaults to 64 documents of include depth (including
the top-level document) and 256 MiB of cumulative input. File reads are checked
before growing the buffer; gzip consumes both compressed and decoded bytes.
Failed URL candidates still consume their input allowance. Exceeding a budget
throws `x3d::ParseResourceLimitError` through Inline/EXTERNPROTO fallback and
PROTO expansion; ordinary missing or malformed URL candidates remain lenient.
Independent top-level calls start fresh, including after exceptions. These limits
bound parser input, not expanded scene heap size or bytes fetched internally by
custom asset backends. Nested calls inherit the outer limits rather than widening
them; asynchronous custom resolvers must enforce their own budgets.

The subsystem boundary is everything under `runtime/parse/`. The entry point for consumers is `parseFile()` / `parseDocument()` in `runtime/parse/X3DParse.hpp`. Reader declarations are in headers; their implementations are compiled from `runtime/parse/*.cpp` in `namespace x3d::codec`.

Readers record successfully assigned scalar field names in
`Scene::authoredScalarFields`, keyed by weak node identity. This keeps an
explicit value distinct from an absent field even when the value equals the
generated default; DEF/USE shares one identity. PROTO readers transfer the
declaration-local scene's interface and body marks to `ProtoDeclaration::authoredScalarFields`, so
an externally resolved declaration keeps its marks after its source document
is destroyed. Unknown and read-only fields are not recorded.

## Scene text created at run time

`codec::makeStringSceneFactory()` returns the parser behind the script services
`Browser.createX3DFromString` and `createVrmlFromString`. It accepts a full
document or a fragment in any encoding, expands PROTOs declared in the text,
returns the root nodes, and never opens a file: its PROTO and Inline resolvers
decline every url. See [Script execution](system-script-sai.md).

## Key files

| File / directory | Role |
|---|---|
| `runtime/parse/X3DParse.hpp` | Umbrella front door: `parseFile()`, `parseDocument()`, `makeReader`, `stripUtf8Bom`, default `localFileProtoResolver`, default `localFileInlineResolver` |
| `runtime/parse/X3DReader.hpp` | Pure-virtual base `X3DReader` — the common interface all concrete readers implement |
| `runtime/parse/Encoding.hpp` | `Encoding` enum + `sniff`, `sniffByContent`, `sniffByExtension`, `isGzip`, `skipBomAndSpace` |
| `runtime/parse/XmlReaderAdapter.hpp` | Thin adapter wrapping `codec::XmlReader` (in `runtime/codecs/`) to satisfy the `X3DReader` interface |
| `runtime/parse/ClassicVrmlReader.hpp` | Full Classic VRML reader (ISO/IEC 19776-2); also the base class for `Vrml97Reader`; defines all dialect hooks |
| `runtime/parse/Vrml97Reader.hpp` | VRML97 reader (ISO/IEC 14772-1) — inherits `ClassicVrmlReader`, overrides the dialect hooks (`mapFieldName`, `onHeaderLine`, `warn`) |
| `runtime/parse/Vrml97Dialect.hpp` | VRML97 → X3D field-name aliases (`vrml97::mapFieldName`); header-only |
| `runtime/parse/JsonReader.hpp` | X3D-JSON reader — walks the Web3D X3D-JSON shape, converts JSON values to X3D wire strings, applies fields via `build::applyField` |
| `runtime/parse/NodeBuilder.hpp` | Encoding-independent build helpers shared by all text/JSON readers: `beginNode`, `applyField`, `attachChild`, `defineDef`, `resolveUse`, `collectFieldValue`; `namespace x3d::codec::build` |
| `runtime/X3DAuthoredScalarFields.hpp` | Weak node-identity field presence shared by scenes and PROTO declarations |
| `runtime/parse/VrmlTokenizer.hpp` | Streaming VRML lexer with one-token (and two-token) lookahead; shared by `ClassicVrmlReader` and `Vrml97Reader` |
| `runtime/parse/JsonLite.hpp` | Bundled minimal JSON parser used by `JsonReader` (`x3d::json::parseComplete`) |
| `runtime/parse/Inflate.hpp` | In-memory gzip decompression (`inflateGzip`) using `tinfl.h` (bundled); called by `parseFile()` when gzip magic is detected |
| `runtime/parse/X3DProtoResolver.hpp` | `ProtoDeclarationResolver` typedef (a `std::function`) + `noopProtoResolver`; the seam for EXTERNPROTO resolution |
| `runtime/parse/tests/` | Per-encoding and per-feature unit tests (see How it is tested) |

## Interfaces and seams

### Exposed interface

The primary consumer-facing API (`namespace x3d::codec`):

```cpp
// runtime/parse/X3DParse.hpp

// Parse from a file path (sniffs encoding, inflates gzip, resolves protos/inlines).
runtime::X3DDocument parseFile(const std::string &path,
                               const std::string &confineRoot = "",
                               const ParseLimits &limits = {});

// Parse from in-memory text with optional encoding hint and resolver overrides.
runtime::X3DDocument
parseDocument(const std::string &text,
              Encoding hint = Encoding::Unknown,
              const std::string &baseUrl = "",
              const ProtoDeclarationResolver &resolver = localFileProtoResolver,
              const runtime::InlineResolver &inlineResolver = localFileInlineResolver,
              const std::string &confineRoot = "",
              const ParseLimits &limits = {});

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
// Extension sniffing:.x3d->XML,.x3dv->ClassicVRML,.wrl->VRML97,.json->JSON;
//.gz/.gzip suffix stripped first.
Encoding sniffByExtension(std::string_view path);
```

### Seam points

- EXTERNPROTO resolver — `ProtoDeclarationResolver` (`runtime/parse/X3DProtoResolver.hpp`): a `std::function` injected into `parseDocument()`. The default `localFileProtoResolver` resolves relative file-system URLs and guards against cross-file cycles with a `thread_local` active-file stack. Embedders (network fetch, virtual FS) supply their own function.

- Inline resolver — `runtime::InlineResolver` (`runtime/InlineExpand.hpp`): a `std::function<shared_ptr<Scene>(urls, baseUrl)>` injected into `parseDocument()`. The default `localFileInlineResolver` follows the same lenient file-local pattern. Embedders override for custom asset resolution and pass that resolver plus the base URL to the runtime when deferred `load=TRUE` events should load content.

### Recursive asset loader policy

`assetResolversFrom(AssetResolver, Encoding)` in
`runtime/parse/AssetDocumentResolvers.hpp` returns `.proto` and `.inlineScene`.
Pass both to `parseDocument()` to retain one synchronous asset policy across every
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
cycles share an explicit active-URL stack; independent top-level calls start
clean. Asset-loaded Inline nodes retain a value-owned loader seeded with their
source ancestry. Deferred `load=TRUE` events therefore reject self and mutual
cycles across ticks, including lexical dot-segment aliases. Siblings and reloads
remain independent. This retained policy takes precedence over a session's
fallback Inline resolver for those nodes; choose the asset policy when loading
the world. Custom resolvers and the local-file loader do not acquire source
identity from this helper. See [ADR-0070](../decisions/0070-deferred-inline-loader-ancestry.md).

The additive three-argument overload accepts a `ProtoResolverDecorator`:
`assetResolversFrom(assets, hint, [](auto base) { return decorate(base); })`.
It wraps the asset-backed PROTO resolver at every parse boundary and also wraps
the returned top-level `.proto` callback. The decorator must delegate to its
supplied callback to retain the loader's confinement, Pending propagation, and
cycle checks. Its base callback borrows synchronous invocation state: don't
retain it outside the invocation. The factory retains the decorator by value;
its captures must outlive use of the loaded world.

`x3d_ext_cave_recursive_resolver` exercises ext declarations in nested files.
The `x3d_extract_tests` case
`Session: deferred asset Inline cycles retain source ancestry` checks self and
mutual cycles, lexical aliases, independent siblings, and reload after unload.
Both additions await maintainer execution.

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

- Dialect hooks on `ClassicVrmlReader` — three protected virtual methods that `Vrml97Reader` overrides:
  - `mapFieldName(nodeType, token)` — renames a field token (identity in Classic VRML; applies the LOD/Switch field renames in VRML97).
  - `onHeaderLine(parts, doc)` — reuses the first line's tokens to set `doc.version`, `doc.profile`, or reject an unsupported encoding (e.g. VRML 1.0 throws in `Vrml97Reader`).
  - `warn(message)` — diagnostic sink; discarded by the base, collected into a `std::vector<std::string>` by `Vrml97Reader` (optional strict mode via `setStrict(true)`).

ClassicVRML extracts and tokenizes the header once, then shares the tokens
with the VRML97 dialect hook. Node type names go directly to the factory;
VRML97 remaps only field names.

JSON document reads require exactly one JSON value. Trailing whitespace is
accepted; trailing garbage or another JSON value throws. The regression case
`json_document_rejects_trailing_content` exercises JsonLite, JsonReader, and
`parseDocument()`.

- Node factory — all readers instantiate nodes via `X3DNodeFactory::create(typeName)`, which is the same factory used everywhere else in the runtime. Unknown type names return null, and the reader records a `ReaderWarning{Kind::UnknownNode}` in `X3DDocument.readerWarnings` so `x3d validate` reports it (DIAG-UNKNOWN-NODE). The XML reader then keeps the element as an opaque `runtime::UnknownNode` (`runtime/X3DProto.hpp`): type name, DEF, authored attributes as wire text in source order, character data, and child nodes, which are read normally so their DEFs resolve. It sits in the parent's named slot or `children` field; when the parent has neither (a 4.1 `<Tangent>` under a 4.0 `IndexedFaceSet`) it is parked in `Scene::opaqueChildren`. An `UnknownNode` has an empty field table, so the runtime, the extractor and PROTO cloning ignore it, and a PROTO instance loses it on expansion. `<field>`, `<IS>` and PROTO statements inside an unknown element are discarded with a warning. The ClassicVRML, VRML97 and JSON readers still skip an unknown node (READ-UNKNOWN-NODE-PASSTHROUGH). In XML, a `<ROUTE>` nested among a node's children is not a node: it is added to the enclosing scene's (or PROTO body's) routes, and a nested `<IS>` is skipped there because the IS pass reads it.

- Reader-recovery diagnostics — `X3DDocument.readerWarnings` (`runtime/X3DProto.hpp`) is the structured channel for lenient-read recoveries that are not legal X3D: an unknown/misspelled node element (`UnknownNode`) and an unknown `profile=` token coerced to `Interchange` (`ProfileCoerced`, DIAG-PROFILE-COERCE). Every reader (`XmlReader`, `ClassicVrmlReader`/`Vrml97Reader`, `JsonReader`) fills it; `cmdValidate` and the cli-gate validate path surface each as a `node`/`profile` diagnostic. It parallels `rangeWarnings`/`protoWarnings`/`inlineWarnings`. A third kind, `Value`, records an authored value that `parseValue` coerced: an unparsable number token (read as 0), a short SF tuple (padded with 0), or a trailing partial MF tuple (dropped). Readers call `noteValueSyntax` for node fields and ProtoInstance fieldValues; `parseDocument` collects them through a thread-local `ValueDiagnosticScope` (`runtime/codecs/FieldValueIO.hpp`) and appends them after the read (VALIDATE-VALUE-SYNTAX).

- Profile token preservation — `X3DDocument::setProfileToken` records the authored `profile=` spelling verbatim in `profileRaw` (exposed via `profileToken()`), resolves it to a `Profile` for profile-fit, and diagnoses a non-canonical token. The XML/JSON/VRML writers emit `profileToken()`, so a round-trip never rewrites the declared conformance class (DIAG-PROFILE-COERCE).

- UNIT header validation — `parseDocument()` checks UNIT declarations before scene expansion for all supported X3D encodings. Declarations require X3D 3.3 or later, one of `angle`/`force`/`length`/`mass`, no duplicate category, a nonempty name without whitespace, and a finite positive conversion factor. An invalid declaration throws; valid authored factors and field values remain unchanged for round-tripping. Runtime conversion is described by `runtime/UnitConversion.hpp`. Runtime entry converts explicitly authored `TextureTransform.rotation` through the angle UNIT factor, including values forwarded via IS; UV center, translation and scale remain dimensionless. The focused `UNIT runtime: TextureTransform*` cases in `runtime_session_test.cpp` cover defaults, repeated initialization and subsequent radian writes.

- PROTO built-in-shadow quarantine — after `readDocument`, `parseDocument()` runs `quarantineBuiltinShadowingProtos` (`runtime/parse/X3DParse.cpp`): any `ProtoDeclare`/`ExternProtoDeclare` whose name is a built-in (`X3DNodeFactory::registry()`) is dropped and recorded as `ProtoWarning{Kind::BuiltinShadow}`, so the built-in keeps precedence (ADR-0033, `PROTO-SHADOW`). Applies uniformly across all four encodings from the single front door; lenient by default.

- Reflection / field population — all readers set fields through the `FieldInfo` thunks exposed by `node.fields()` (the reflection `FieldTable`). `build::applyField` routes enum fields through `setEnumString` and everything else through `FieldValueIO::parseValue + set`. The `outputOnly`/`inputOnly` access guards in `applyField` skip read-only fields during parse.

- DynamicFieldStore (S1 seam) — `ClassicVrmlReader`, `JsonReader`, and `XmlReader` capture author `<field>` declarations into the current `Scene::authorFields` owner as `AuthorFieldDecl` entries (see `runtime/events/DynamicField.hpp`). Script fields are captured across the supported readers; XML, Classic, and JSON also capture programmable-shader author declarations such as `ComposedShader` uniforms, including node-valued defaults. Declaration-local Scenes transfer their owner to `ProtoDeclaration::authorFields`; callers do not depend on a process-global table. This seam is the parse-reader touchpoint for the Script/SAI and author-shader runtimes; all other nodes are fully handled by the reflection layer. Inline `<![CDATA[...]]>` source is mirrored into the node's `sourceCode` slot — `Script.sourceCode` for scripts, `ShaderPart.sourceCode` for a ComposedShader's GLSL stages and `ShaderProgram.sourceCode` for a ProgramShader's — so the runtime has a uniform source path.

- PROTO body order — direct body declarations, nodes and instances are recorded in `ProtoBody::statements`. The statement records retain unused local/extern declarations and the point at which a nested declaration shadows an inherited name. Node and instance values remain in the existing body collections. Ordinary body-node children and declarations are also recorded in `ProtoBody::nodeStatements`, keyed by weak parent identity. Reflected fields and `nestedInstances` remain the value authority. Ordinary interface-default node graphs share this context. Direct ProtoInstance defaults occupy `nodeDefault` as authored `ProtoInstanceTemplate` nodes. The local DEF table resolves USE references to the same template across fields and body graphs; these defaults do not become direct body instances. XML, Classic and JSON also capture direct and contained ProtoInstances in SFNode/MFNode fieldValues as bound templates in the owning value graph. All scene structural instances, named or unnamed, occupy an authored template slot in `rootNodes` or their ordinary parent's node field. The structural record's weak `placementTemplate` link preserves the exact identity; expansion replaces the slot without appending it after ordinary nodes. USE references reuse the same expanded primary. Raw writers read source values from the linked structural record while retaining slot order.

- PROTO DEF scoping — XML, JSON, and Classic readers parse each declaration's interface defaults and body through one local `Scene`. DEF/USE aliases therefore resolve across fields and into the body, while outer scene DEFs and nested PROTO DEFs remain separate (§4.4.4.4). Previously declared PROTO/EXTERNPROTO declarations remain available for nested instances; a same-name nested declaration shadows the inherited name across both declaration kinds without changing the enclosing scene. The declaration carries authored scalar-field marks from both interface and body after the local scene is consumed. IS-connection links are threaded through as `runtime::IsConnection` entries on `runtime::ProtoBody`.

## Related

- [ADR-0007: Version-inference ladder](../decisions/0007-version-inference-ladder.md)
- [ADR-0014: Dynamic-field foundation](../decisions/0014-dynamic-field-foundation.md)
- [Proto Expand](proto-expand.md)
- [Inline Expand](inline-expand.md)
- [Codecs Writers](codecs-writers.md)
- [Scene Graph](scene-graph.md)
- [ADR-0062](../decisions/0062-classic-prototype-metadata-comments.md)
