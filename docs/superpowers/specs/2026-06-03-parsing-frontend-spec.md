# X3D Parsing Front-End — Unified Reader Layer

**Date:** 2026-06-03
**Status:** Spec for build
**Branch:** `modernize-x3d-spec`

## Goal

A first-party, unified **parsing front-end** that ingests any of the four X3D
encodings and produces the existing `x3d::runtime::X3DDocument` (Head + Scene
with DEF/USE identity, ROUTEs, IMPORT/EXPORT, head metadata). Today only
X3D-XML can be read (`XmlReader`). This layer adds ClassicVRML (`.x3dv`),
VRML97 (`.wrl`), and X3D-JSON (`.json`) readers behind one interface, plus
**encoding sniffing** so a caller can hand over bytes (or a path) and get a
document without naming the format.

Every reader stays **node-agnostic**: it instantiates nodes via
`X3DNodeFactory::create(typeName)`, sets fields by `x3dName` through the
reflection `FieldInfo` thunks (`set` / `setEnumString`), and parses wire values
through `FieldValueIO`. No per-node parsing code, mirroring the codecs.

## Layout

New header-only files under `runtime/parse/`, namespace `x3d::codec` (same as
the existing codecs), pulled together by an umbrella header:

```
runtime/parse/
  X3DReader.hpp        // the interface every encoding reader implements
  Encoding.hpp         // Encoding enum + sniffByExtension + sniffByContent
  NodeBuilder.hpp      // shared node-agnostic build steps (the reusable core)
  VrmlTokenizer.hpp    // shared lexer for ClassicVRML + VRML97 text grammars
  ClassicVrmlReader.hpp// .x3dv  -> X3DDocument
  Vrml97Reader.hpp     // .wrl   -> X3DDocument (VRML97 grammar + accessType map)
  JsonReader.hpp       // .json  -> X3DDocument (over bundled JsonLite)
  JsonLite.hpp         // tiny recursive-descent JSON parser (no deps)
  X3DParse.hpp         // umbrella: includes all of the above + parseDocument()
```

`XmlReader` (in `runtime/codecs/`) is **wrapped**, not moved, by an
`XmlReaderAdapter` that satisfies `X3DReader`. The codecs umbrella is untouched;
`X3DParse.hpp` is the new front door.

## The interface

```cpp
namespace x3d::codec {

/// Identifies an encoding for sniffing and dispatch.
enum class Encoding { Unknown, XML, ClassicVRML, VRML97, JSON };

/// Common front-end for every encoding reader. Header-only, stateless per call.
class X3DReader {
public:
  virtual ~X3DReader() = default;

  /// The encoding this reader handles.
  virtual Encoding encoding() const = 0;

  /// Parse a complete document from in-memory text. Throws std::runtime_error
  /// only on unrecoverable malformation; unknown node/field names are skipped
  /// gracefully (consistent with XmlReader). The returned document already has
  /// scene.resolveRoutes() applied.
  virtual runtime::X3DDocument readDocument(const std::string &text) = 0;
};

} // namespace x3d::codec
```

Each concrete reader (`XmlReaderAdapter`, `ClassicVrmlReader`, `Vrml97Reader`,
`JsonReader`) implements this and nothing else. Every reader's `readDocument`
ends by calling `doc.scene.resolveRoutes()` so the contract is uniform.

## Encoding sniffing (extension AND content)

`Encoding.hpp` provides two independent sniffers and a combiner:

```cpp
Encoding sniffByExtension(std::string_view path); // ".x3d"->XML, ".x3dv"->ClassicVRML,
                                                  // ".wrl"->VRML97, ".json"->JSON
Encoding sniffByContent(std::string_view bytes);   // see rules below
Encoding sniff(std::string_view path, std::string_view bytes); // content wins
                                                               // when confident
```

Content rules (first match after skipping leading BOM/whitespace):

| Probe | Result |
|---|---|
| starts with `#VRML V2.0` | VRML97 |
| starts with `#VRML V1.0` | VRML97 (best-effort; warn) |
| starts with `#X3D V3` / `#X3D V4` | ClassicVRML |
| starts with `<?xml`, `<X3D`, or `<!DOCTYPE X3D` | XML |
| first non-ws char is `{` and `"X3D"` appears | JSON |
| starts with gzip magic `0x1f 0x8b` | gzip — caller must inflate first (see Risks) |

`sniff()` prefers **content** when it yields a confident answer (handles
mislabeled / extensionless files and `.wrl` files that are actually X3D
ClassicVRML); otherwise falls back to the extension. A top-level convenience
binds it all together:

```cpp
runtime::X3DDocument parseDocument(const std::string &text,
                                   Encoding hint = Encoding::Unknown);
runtime::X3DDocument parseFile(const std::string &path); // reads bytes, sniffs,
                                                         // dispatches
```

`parseDocument` resolves `Unknown` via `sniffByContent`, constructs the matching
reader, and delegates. Unknown-after-sniffing throws `std::runtime_error`.

## Shared node-agnostic core (`NodeBuilder.hpp`)

The reusable middle layer every reader funnels through, so the four encodings
differ only in their **syntax front-end**, not their model-building. These are
the same operations `XmlReader` performs, lifted into free functions keyed on
parsed name/value strings (encoding-independent):

- `std::shared_ptr<X3DNode> beginNode(std::string_view typeName)` —
  `X3DNodeFactory::create`; returns null for unknown types (caller skips).
- `void applyField(X3DNode&, std::string_view x3dName, std::string_view wire)` —
  find `FieldInfo` by `x3dName`; if `isEnum()` call `setEnumString`, else
  `parseValue(type, wire)` + `set` (skips read-only fields, matching XmlReader).
- `void attachChild(X3DNode& parent, std::string_view containerField,
  const std::shared_ptr<X3DNode>& child)` — the containerField resolution chain
  from `XmlReader::attachChild` (explicit containerField → field-name match →
  `children` → first writable node field; SFNode set / MFNode append).
- `void defineDef(runtime::Scene&, std::string_view def, node)` and
  `resolveUse(...)` — DEF registered **before** child recursion (so a USE inside
  the subtree resolves), USE returns the shared node and ignores its body.

This is the lift that lets `XmlReader`'s `applyAttribute` / `attachChild` /
`findField` logic be **shared verbatim** by the text and JSON readers rather
than re-implemented three times. (XmlReader may be refactored to call
`NodeBuilder` in a follow-up; not required for this layer to land.)

## Shared SF/MF value parsing

All scalar/array value parsing **reuses `FieldValueIO`** unchanged:
`parseValue(X3DFieldType, string)`, `tokenize`, `parseMFString`, `parseFloat/
Double/Int`. The text readers additionally need a small utility that the XML
reader did not (because XML delimits values by attribute quotes, but the text
grammars embed values inline in the token stream):

`NodeBuilder::collectFieldValue(VrmlTokenizer&, X3DFieldType) -> std::string`
gathers the correct run of tokens for a field and hands the joined wire string
to `FieldValueIO::parseValue`:

- **SF scalar** (Bool/Int32/Float/Double/Time): one token.
- **SF struct** (Color/Vec*/Rotation): N component tokens (N from the type).
- **SFString**: one quoted token (tokenizer returns the unescaped contents).
- **MF***: a bracketed `[ ... ]` run, or a single bare element (VRML permits
  `field 0 0 0` as a one-element MF). `MFString` keeps per-element quotes.
- **Enum**: one token, routed to `setEnumString` (never `parseValue`).

This keeps a single source of truth for value semantics; the tokenizer only
decides *how many tokens* a field consumes, and `FieldValueIO` decides what they
*mean*. JSON values bypass `collectFieldValue`: `JsonReader` converts JSON
scalars/arrays straight to the wire string `parseValue` expects (numbers
joined space-separated, booleans → `true/false`, MFString arrays → quoted list).

## ClassicVRML reader (`.x3dv`)

`VrmlTokenizer` lexes the ISO/IEC 19776-2 grammar: `#`-to-EOL comments,
whitespace/comma delimiters, `{ } [ ]` punctuation, quoted strings with `\"`/`\\`
escapes, and bare identifiers/numbers. `ClassicVrmlReader` then:

1. Header: `#X3D Vx.y utf8` → `doc.version`; `PROFILE name`; `COMPONENT name:lvl`;
   `UNIT category name factor`; `META "name" "content"` → `Head` (mirrors
   `VrmlWriter`'s output 1:1).
2. Scene body — a sequence of statements:
   - `DEF Name Type { … }` / `Type { … }` / `USE Name` → `NodeBuilder`.
   - Inside a node body, alternating `fieldName value` pairs: a value beginning
     with `DEF`/`USE`/an identifier-followed-by-`{` is a child node
     (`attachChild` using the field name as the containerField); otherwise it is
     a value field (`collectFieldValue` + `applyField`).
   - `ROUTE A.f TO B.g` → `scene.routes.emplace_back(A,f,B,g)`.
   - `PROTO` / `EXTERNPROTO` / `IS` / `Script {…}` are **recognized and
     skipped structurally** (balanced-brace skip) for this layer — same
     limitation XmlReader has with ProtoInstance/Script; the document still
     parses. (Full PROTO capture is a follow-up; see Non-goals.)

## VRML97 reader (`.wrl`)

`Vrml97Reader` reuses `VrmlTokenizer` and the same statement loop, with three
deltas:

1. Header `#VRML V2.0 utf8` accepted (and `#VRML V1.0` best-effort); document
   version set to `4.0` (X3D superset — all 87 VRML97 nodes exist in the
   factory).
2. No `PROFILE`/`COMPONENT`/`UNIT` statements; VRML97 has only `#`-comment
   header lines + the scene. A leading `WorldInfo` is an ordinary node.
3. **AccessType alias mapping** for inline `field`/`eventIn`/`eventOut`/
   `exposedField` keywords (only meaningful inside `PROTO`/`Script` interface
   declarations, which this layer skips). A small `accessTypeFromVrml97()` map
   (`eventIn→inputOnly`, `eventOut→outputOnly`, `field→initializeOnly`,
   `exposedField→inputOutput`) is provided in `Vrml97Reader.hpp` for when PROTO
   capture lands; node-level field assignments use the same `x3dName` lookup as
   ClassicVRML and need no remapping.

## JSON reader (`.json`)

`JsonLite` parses the bytes into a tiny variant tree (object/array/string/
number/bool/null) with no external deps (sibling to `XmlLite`). `JsonReader`
walks the X3D-JSON shape produced by `JsonWriter` (the inverse):

- `{"X3D":{"@profile","@version","head":{…},"Scene":{"-children":[…]}}}`.
- Object key `TypeName` → `beginNode`; `@field` keys → value fields
  (`@DEF`/`@USE` handled specially); `-childField` keys → arrays of child nodes
  routed via `attachChild` using the de-prefixed key as the containerField.
- Head: `head.meta` / `head.component` / `head.unit` arrays → `Head`.
- `Scene.-children` → root nodes; a `ROUTE` object (`@fromNode` …) → `routes`.
- JSON scalar → wire string conversion as described under *shared value
  parsing*; MF numeric arrays are flattened to space-separated text so a single
  `FieldValueIO::parseValue` call covers them.

## Error handling (uniform across readers)

- Unknown node type → skip (factory returns null), consistent with `XmlReader`.
- Unknown field `x3dName` → ignore.
- Read-only field assignment (outputOnly/initializeOnly) → skip.
- Malformed value tokens → `FieldValueIO`'s tolerant parse (0 / empty), no throw.
- Unresolved ROUTE/USE endpoints → tolerated; `resolveRoutes()` leaves
  weak_ptrs empty (matches event-system + serialization tolerance).
- Only structural impossibilities throw `std::runtime_error` (unbalanced braces,
  truncated input, JSON syntax error, unknown encoding after sniffing).

## Testing (TDD, ctest)

`runtime/parse/tests/reader_test.cpp`, incrementally:

1. **Sniffing**: extension and content sniffers classify the four encodings;
   content overrides a wrong extension; gzip magic detected.
2. **Round-trip parity**: write a known document with `VrmlWriter` /
   `JsonWriter` / `XmlWriter`, read it back with the matching reader, assert the
   resulting documents are structurally equal (node types, field values,
   DEF/USE identity, routes) — proves reader is the writer's inverse.
3. **DEF/USE identity**: a USE shares the same `shared_ptr` as its DEF in every
   text/JSON reader (pointer-equality check, as in `roundtrip_test.cpp`).
4. **Corpus smoke**: parse a handful of real fixtures per encoding from the
   x3d-render corpus (`HelloWorld.x3dv`, a small `.wrl`, `HelloWorld.json`) and
   assert non-empty scene + resolvable routes, no throw.
5. **Unified entry**: `parseDocument(text)` with no hint produces an equal
   document to the explicitly-dispatched reader for each encoding.

## Non-goals (this layer)

- PROTO/ExternPROTO/Script **expansion** and IS-event wiring (recognized and
  skipped; full capture/expansion is the existing proto/event roadmap).
- gzip inflation (caller inflates `.wrl.gz`; sniffer only *detects* it).
- Constraint/range validation at parse time (set thunks already throw on range;
  no extra schema validation here).
- Streaming/partial parse of giant MF arrays (whole-document, in-memory; the
  605K-line corpus files parse but are not optimized).
- XML namespace/DTD conformance beyond what `XmlLite` already does.
