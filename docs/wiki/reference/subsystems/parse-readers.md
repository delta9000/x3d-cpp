---
title: Parse Readers
summary: Parse frontend — XML, VRML97, Classic VRML, and JSON readers; node builder; encoding sniffing; version-inference ladder.
tags: [subsystem, parse, readers, xml, vrml, json, version-inference]
updated: 2026-11-11
related:
  - ../../explanation/architecture.md
  - proto-expand.md
  - inline-expand.md
  - codecs-writers.md
  - scene-graph.md
  - ../../explanation/decisions/0007-version-inference-ladder.md
  - ../../explanation/decisions/0014-dynamic-field-foundation.md
---

# Parse Readers

## Purpose

- The parse frontend turns raw X3D bytes — in any of four encodings (XML,
  Classic VRML, VRML97, JSON) and optionally gzip-compressed — into the
  runtime document model (`x3d::runtime::X3DDocument`).
- It owns encoding detection, version inference, node and field population via
  the reflection layer, DEF/USE identity, ROUTE capture, PROTO/EXTERNPROTO
  structural capture, IMPORT/EXPORT, and Script author-field capture (` S1`).
- After the document is built it triggers PROTO/Inline expansion through
  injectable resolver seams.
- Unknown node types and unknown field names are skipped gracefully
  (lenient-read policy); only unrecoverable malformation throws.
- Input-hardening (DoS guards).
- The recursive-descent readers descend attacker-controlled structure, so each
  caps nesting depth against a shared ceiling (`runtime/RecursionLimits.hpp`,
  `x3d::kMaxNestingDepth` = 1000): the XML tokenizer
  (`parseElement`↔`parseContent`), JSON (`parseValue`↔`parseArray`/
  `parseObject`), and ClassicVRML/VRML97 (`parseNode`↔`parseNodeBody`) throw
  `std::runtime_error` past the cap instead of overrunning the native stack
  (SEC-1).
- Independently, `SFImage`/`PixelTexture` pixel parsing clamps `numComponents`
  to the spec range `[0,4]` before unpacking, so a hostile token cannot
  trigger a signed-shift overflow or a multi-GB allocation (SEC-2).
- Legitimate documents nest far below the cap and are unaffected.
- The file front door is hardened against the same adversary.
- Gzip input is inflated with a decompressed-size ceiling
  (`kMaxDecompressedBytes`, default 256 MiB) clamping both the initial
  reservation and the grow loop, so a decompression bomb or a forged ISIZE
  footer cannot drive an OOM (SEC-5, `Inflate.hpp`).
- The default `Inline`/`EXTERNPROTO` resolvers canonicalize a file-like `url`
  and confine it to a configurable root — `parseFile()`/ `parseDocument()`
  default it to the source file's own directory (a secure default for
  untrusted input: absolute urls and `../` cross-directory reads like
  `url='/etc/passwd'` are rejected, SEC-3), while a tool parsing a trusted
  tree passes a wider `confineRoot` so legitimate `../` references within it
  resolve (the conformance CLI gate does this with the corpus root).
- Either way the canonical path keys the cross-file cycle guard, so spelling
  aliases (`./a.x3d` vs `a.x3d`) cannot defeat it (SEC-4).
- See `runtime/parse/PathConfine.hpp` and
  [ADR-0038](../../explanation/decisions/0038-local-resolver-path-confinement.md).
- Embedders needing remote/non-filesystem access supply their own resolver.
- The front doors also share a synchronous per-thread parse session across
  nested resolver calls.
- `ParseLimits` defaults to 64 documents of include depth (including the
  top-level document) and 256 MiB of cumulative input.
- File reads are checked before growing the buffer; gzip consumes both
  compressed and decoded bytes.
- Failed URL candidates still consume their input allowance.
- Exceeding a budget throws `x3d::ParseResourceLimitError` through
  Inline/EXTERNPROTO fallback and PROTO expansion; ordinary missing or
  malformed URL candidates remain lenient.
- Independent top-level calls start fresh, including after exceptions.
- These limits bound parser input, not expanded scene heap size or bytes
  fetched internally by custom asset backends.
- Nested calls inherit the outer limits rather than widening them;
  asynchronous custom resolvers must enforce their own budgets.

The subsystem boundary is everything under `runtime/parse/`. The entry point for consumers is `parseFile()` / `parseDocument()` in `runtime/parse/X3DParse.hpp`. Reader declarations are in headers; their implementations are compiled from `runtime/parse/*.cpp` in `namespace x3d::codec`.

- Readers record successfully assigned scalar field names in
  `Scene::authoredScalarFields`, keyed by weak node identity.
- This keeps an explicit value distinct from an absent field even when the
  value equals the generated default; DEF/USE shares one identity.
- PROTO readers transfer the declaration-local scene's interface and body
  marks to `ProtoDeclaration::authoredScalarFields`, so an externally resolved
  declaration keeps its marks after its source document is destroyed.
- Unknown and read-only fields are not recorded.

## Scene text created at run time

`codec::makeStringSceneFactory()` returns the parser behind the script services
`Browser.createX3DFromString` and `createVrmlFromString`. It accepts a full
document or a fragment in any encoding, expands PROTOs declared in the text,
returns the root nodes, and never opens a file: its PROTO and Inline resolvers
decline every url. See [Script execution](system-script-sai.md).

## Fetched-source diagnostics proposal

- The [fetched-document diagnostics
  proposal](../../explanation/decisions/0072-fetched-document-diagnostics.md)
  preserves warnings from selected asset-loaded child documents as
  source-qualified `SourceWarning` details in the enclosing document's
  Inline/PROTO warning channel.
- Top-level reader and range diagnostics keep their existing structured
  channels.
- For an EXTERNPROTO only the selected declaration carries the fetched
  document's warnings, and expansion reports each one once per warning
  collection however many instances resolve it.
- The registered `inline_diagnostics_test.cpp` cases run in
  `x3d_extract_tests`.

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

How Inline and EXTERNPROTO loads are confined, bounded and reported is in
[Asset loader policy](asset-loader-policy.md).

## Related

- [ADR-0007: Version-inference ladder](../../explanation/decisions/0007-version-inference-ladder.md)
- [ADR-0014: Dynamic-field foundation](../../explanation/decisions/0014-dynamic-field-foundation.md)
- [Proto Expand](proto-expand.md)
- [Inline Expand](inline-expand.md)
- [Codecs Writers](codecs-writers.md)
- [Scene Graph](scene-graph.md)
- [ADR-0062](../../explanation/decisions/0062-classic-prototype-metadata-comments.md)
- [Architecture](../../explanation/architecture.md)
