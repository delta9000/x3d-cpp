# ClassicVRML (.x3dv) Reader — Spec

**Date:** 2026-06-03
**Status:** Proposed
**Branch:** `modernize-x3d-spec`

## Goal

Add a hand-written, node-agnostic **ClassicVRML reader** (`VrmlReader`) that
parses the X3D ClassicVRML encoding (ISO/IEC 19776-2, `.x3dv`) into the runtime
`X3DDocument` model. It is the inverse of the existing `VrmlWriter` and the
ClassicVRML sibling of `XmlReader`, closing the "reading is XML-only" gap. Like
every other codec it contains **zero per-node code**: node creation goes through
`X3DNodeFactory`, all field population through the reflection `FieldTable`, and
all wire-value parsing through `FieldValueIO`.

This makes the 90 `.x3dv` corpus fixtures (and hand-authored ClassicVRML)
ingestable, and gives `VrmlWriter → VrmlReader` round-trip coverage.

## Scope

In: header, scene tree, nodes/fields, `DEF`/`USE`, `ROUTE`, `PROTO`/
`EXTERNPROTO`/proto instances, `IS`, `PROFILE`/`COMPONENT`/`UNIT`/`META`,
`IMPORT`/`EXPORT`, `#` comments. Out: PROTO body *expansion* (carried as data,
expansion stays the documented `ProtoInstance::expand()` stub), Script execution,
gzip `.wrl`, VRML97 `#VRML V2.0` header (X3D ClassicVRML only).

## Files (header-only, namespace `x3d::codec`)

- **`runtime/codecs/VrmlReader.hpp`** — the reader. Public surface mirrors
  `XmlReader`:
  ```cpp
  runtime::X3DDocument readDocument(const std::string &x3dvText);
  ```
  Throws `std::runtime_error` on a syntax error it cannot recover from
  (unexpected token, unbalanced braces/brackets, unterminated string). Unknown
  node *types* are skipped gracefully (factory returns null → skip the node and
  its `{...}` body), matching `XmlReader`.
- **`runtime/codecs/VrmlLexer.hpp`** — the tokenizer (see below). Split out the
  way `XmlLite.hpp` is split from `XmlReader.hpp`, so it is unit-testable alone.
- Add both to the `X3DCodecs.hpp` umbrella include.

### Optional shared interface (`X3DReader`)

Introduce a thin `runtime/codecs/X3DReader.hpp` abstract base
(`virtual X3DDocument readDocument(const std::string&) = 0;`) and have
`XmlReader` and `VrmlReader` derive from it, so callers can select a codec by
format. This is additive and behavior-preserving for `XmlReader`. If it risks
golden/test churn, ship `VrmlReader` standalone first and add the interface in a
follow-up — the method signature already matches.

## Tokenizer (`VrmlLexer`)

ClassicVRML is whitespace-delimited with a handful of punctuation tokens. The
lexer turns the input into a `Token` stream the parser consumes with one-token
lookahead (`peek()` / `next()`); it tracks line/column for error messages.

Token kinds:

- **Punctuation:** `{` `}` `[` `]` — the only structural single-char tokens.
- **String:** `"..."` with backslash escapes `\"` and `\\`. Internal whitespace
  preserved. Reuse the escape logic already in
  `FieldValueIO::parseMFString`/`tokenize` rather than reimplementing.
- **Identifier/keyword:** a maximal run of non-whitespace, non-`{}[]"#`
  characters. Covers node type names, field names, DEF names, keywords, and bare
  scalar value tokens (`true`, `-1`, `3.14`, `0x1A`, enum tokens) — the parser
  decides meaning from grammar position.
- **Delimiters:** spaces, tabs, CR, LF, **and commas** are token separators and
  are otherwise insignificant (X3D allows comma or whitespace between MF
  elements). Newlines are not significant except to terminate comments.
- **Comments:** `#` to end-of-line is skipped — *except* the very first line,
  which is the `#X3D V…` header (handled by the parser, not stripped as a
  comment). No block comments exist in the grammar.

The lexer does **not** parse numbers/vectors; it only segments tokens. Numeric
and MF value parsing is delegated to `FieldValueIO` after the parser has
re-joined the value tokens for a field into a wire string.

## Parser (recursive descent)

### Header line

First line: `#X3D V<major>.<minor> <encoding>` (e.g. `#X3D V4.0 utf8`). Extract
`version` ("4.0"). Tolerate a leading UTF-8 BOM and `#VRML V2.0` (accept but flag
as best-effort). Set `doc.version`; `encoding` is informational.

### Header statements (populate `runtime::Head` / doc)

Zero or more, in any order, before the first node:

- `PROFILE <name>` → `doc.profile = profileFromString(name)`.
- `COMPONENT <name>:<level>` → `Head::components` (`{name, level}`).
- `UNIT <category> <name> <factor>` → `Head::units`.
- `META "<name>" "<content>"` → `Head::meta` (name/content only; the optional
  dir/httpEquiv/lang/scheme variants are not expressible in ClassicVRML).

These map 1:1 to what `VrmlWriter` emits — the writer is the reference grammar.

### Scene body

A sequence of top-level statements until EOF, each dispatched by leading keyword:

| Leading token | Production |
|---|---|
| `DEF` / `USE` / `<TypeName>` | node → `scene.addRootNode(node)` |
| `ROUTE` | route statement |
| `PROTO` | proto declaration |
| `EXTERNPROTO` | extern-proto declaration |
| `IMPORT` / `EXPORT` | import/export statement |

### Node production

```
node      := "USE" Name
           | ["DEF" Name] TypeName "{" body "}"
           | TypeName "{" body "}"          // anonymous
body      := ( fieldStmt | nodeStmt | routeStmt | protoStmt | "IS"-clause )*
```

- `USE Name` → `scene.resolve(Name)` (shared `shared_ptr`, identity sharing);
  ignore any following `{...}` per USE semantics (a conformant file has none).
- `DEF Name TypeName` → `X3DNodeFactory::create(TypeName)`; set the node's DEF
  (`node->setDEF(Name)` if available, else the DEF reflection field) and call
  `scene.define(Name, node)` **before** recursing the body, so a `USE` inside the
  subtree resolves (identical ordering to `XmlReader::readNode`).
- Unknown `TypeName` (factory null): consume the balanced `{...}` block and
  return null so the parent skips the slot.

Inside the body, the leading token of each statement disambiguates field vs.
nested node vs. route:

- A token that matches a `FieldInfo.x3dName` on the current node's `FieldTable`
  → **field statement**.
- `ROUTE` / `PROTO` / `EXTERNPROTO` → those productions (proto-local scope).
- Otherwise treat as an error (or, for resilience, skip to the next balanced
  unit) — but field-name lookup against reflection is authoritative.

### Field statement (the reflection core)

```
fieldStmt := fieldName fieldValue
fieldValue := scalarTokens          // SF*/MF* non-node value
            | node                  // SFNode field
            | "[" node* "]"         // MFNode field
            | "IS" protoFieldName   // proto interface mapping
```

Resolve `fieldName` to a `const FieldInfo*` via `findField(table, name)` (by
`x3dName`, case-sensitive — same helper as `XmlReader`). Then by
`FieldInfo.type`:

1. **SFNode field** (`f.isNode()` && `SFNode`): parse one `node`, `f.set(parent,
   any(child))`.
2. **MFNode field** (`f.isNode()` && `MFNode`): expect `[`; parse `node*` until
   `]`; collect into `vector<shared_ptr<X3DNode>>`; `f.set`. (A single bare node
   without brackets is also accepted by some authors — accept it as a 1-element
   list for resilience.)
3. **Enum field** (`f.isEnum()`): read one token; `f.setEnumString(parent,
   token)`. Do **not** route enum tokens through `parseValue`.
4. **Value field** (everything else): collect the value tokens for this field
   into a wire string, then `parseValue(f.type, wire)` → `std::any` →
   `f.set(parent, any)` (guard `f.isWritable()`; skip read-only like `XmlReader`).
   - *SF scalar:* exactly one token (`SFString` is a single quoted token —
     re-quote into the wire form `parseValue`/`parseMFString` expects, or pass the
     unquoted text per `FieldValueIO`'s SFString convention).
   - *MF non-node:* a bracketed `[ ... ]` list **or** a bare run; join the
     element tokens back with spaces and hand the whole thing to `parseValue`
     (its `tokenize` re-splits and consumes commas). For `MFString`, preserve the
     per-element quotes so `parseMFString` sees them.
   - This is the one subtlety vs. XML: in XML the whole value is one attribute
     string; here the lexer has already split it, so the parser must **re-join**
     a field's value tokens (respecting `[]` boundaries and quoted strings)
     before calling `parseValue`. Reuse `FieldValueIO` for all interpretation.

Reuse `XmlReader::applyAttribute`'s exact branch logic (enum → setEnumString;
writable → parseValue+set; else skip) so the two readers stay consistent.

### `IS` clause

`fieldName IS protoFieldName` inside a node that lives in a `PROTO` body maps the
node's field to a proto-interface field. The runtime models `IS` as a reserved
`SFNode` field carrying mapping metadata (current model is a stub). Capture the
`(fieldName, protoFieldName)` pair into the proto-body / instance mapping
structure available in `runtime::X3DProto`; do not attempt to wire events
(runtime IS-routing is roadmap sub-project #8). Outside a proto body, `IS` is an
error.

### ROUTE statement

```
ROUTE Name1.field1 TO Name2.field2
```

Split each endpoint on the `.`; emit `scene.routes.emplace_back(fromNode,
fromField, toNode, toField)` exactly as `XmlReader` does. Endpoint resolution is
deferred to `scene.resolveRoutes()` at the end (string names are authoritative;
unresolved/forward refs are tolerated, not fatal).

### PROTO / EXTERNPROTO

Populate the structural model in `runtime/X3DProto.hpp`; **no expansion**.

```
PROTO Name "[" interface "]" "{" protoBody "}"
EXTERNPROTO Name "[" interface "]" urlList
interface := ( accessType FieldType fieldName [defaultValue] )*
accessType := inputOnly | outputOnly | initializeOnly | inputOutput
            | eventIn | eventOut | field | exposedField     // VRML97 aliases
```

- Map the four access-type tokens to `AccessType`; accept the VRML97 aliases
  (`eventIn→inputOnly`, `eventOut→outputOnly`, `field→initializeOnly`,
  `exposedField→inputOutput`) — pure token translation at parse time.
- Map the `FieldType` token (e.g. `SFVec3f`, `MFNode`) to `X3DFieldType` (add a
  `fieldTypeFromString` helper alongside the existing type tags if one does not
  exist).
- `defaultValue` present only for `field`/`exposedField` (initializeOnly/
  inputOutput); parse it with `parseValue`/as a node, store on the `ProtoField`.
- `protoBody` is a nested scene-body (nodes + routes) parsed with the same
  recursion into a child DEF scope; the **first** node of the body is the proto's
  primary node. Store body nodes/routes on `ProtoDeclaration`.
- `EXTERNPROTO`: parse interface (no defaults), then a `urlList` (one quoted
  string or `[ "..." ... ]`) into `ExternProtoDeclaration::url`.
- **Proto instance** (`TypeName` matching a declared/extern proto, not a factory
  node): parse `{ (fieldName fieldValue)* }` as `ProtoFieldValue` overrides into a
  `ProtoInstance`; carry it, do not expand. Detect "is a proto" by checking the
  scene's proto table before falling back to `X3DNodeFactory::create`.

### IMPORT / EXPORT

```
IMPORT inlineDEF.importedDEF AS localName
EXPORT localDEF AS exportedName
```

→ `scene.imports` / `scene.exports`, mirroring `XmlReader`'s structs (`AS` is
optional).

## DEF scope

Use the existing `Scene` DEF symbol table for the top level. PROTO bodies define
a nested DEF scope (DEFs inside a proto are local to it). Minimal approach: parse
the proto body against a fresh `Scene` whose `defs` are proto-local, then attach
its `rootNodes`/`routes` to the `ProtoDeclaration`. (Cross-scope leakage is
already out of scope per the XmlReader notes; match that behavior.)

## Error handling (match repo tolerance)

- Unknown node type / unknown field name → skip (consume balanced block /
  consume value), never throw. Same silent-skip contract as `XmlReader`.
- Unbalanced `{}`/`[]`, unterminated string, malformed header, `IS` outside a
  proto → `std::runtime_error` with line:col.
- Routes/IS to unresolved names → tolerated; `resolveRoutes()` leaves them empty.
- Enum/`parseValue` failures degrade exactly as `FieldValueIO` already does (bad
  token ignored / 0-value), no new error path.

## Testing (TDD, ctest) — `runtime/codecs/tests/`

Add `vrml_reader_test.cpp` (and `vrml_lexer_test.cpp` for the lexer), then extend
`roundtrip_test.cpp` with `VrmlWriter → VrmlReader → VrmlWriter` value-asserting
round-trips. Increments:

1. **Lexer:** tokens, comma==whitespace, `#` comment skip, header-line exemption,
   quoted strings with escapes, `{}[]` punctuation, line/col tracking.
2. **Minimal scene:** header + `PROFILE` + one `Shape{ geometry Box{ size … } }`;
   assert node types and a parsed field value via reflection `get`.
3. **DEF/USE:** `DEF A Transform{…} USE A` → both `rootNodes`/children share one
   `shared_ptr` (identity, `.get()` equality).
4. **ROUTE:** `ROUTE Clock.fraction_changed TO Root.set_translation` survives and
   resolves to the DEF'd nodes.
5. **Header statements:** COMPONENT/UNIT/META populate `Head` correctly.
6. **PROTO/EXTERNPROTO/instance + IS:** declaration, interface (incl. VRML97
   access aliases), instance overrides, and an `IS` mapping are captured in the
   `X3DProto` model (no expansion asserted).
7. **MF parsing:** `MFInt32`/`MFVec3f` with mixed comma+whitespace and bracketed
   lists; `MFString` per-element quotes preserved.
8. **Round-trip:** writer→reader→writer byte-stable on fixtures 1–4 below; and a
   value round-trip on a hand-built document (mirrors the existing
   value-asserting generated test).

### Corpus fixtures (from recon; under `<x3d-render-workspace>`)

Verified present; copy minimal/representative ones into the repo's test data (or
point the test at the corpus path) for read-only parser coverage:

1. `…/x3d/content/examples/Basic/Geospatial/MBARI/ship.x3dv` — 6-line minimal
   `Shape{ Box }`; smallest sanity fixture.
2. `…/x3d/content/examples/HelloWorld.x3dv` — 86 lines; Viewpoint/Transform/
   Sphere/Text/Shape, header statements; the canonical happy path.
3. `…/x3d/tools/Vrml97ToX3dNist/JavaLBExamples/AddDynamicRoutes.x3dv` — 9 lines;
   `Script` node + ROUTE patterns (Script carried as data, not executed).
4. `…/x3d/content/examples/Basic/NURBS/originals/animated_patch.x3dv` — 103
   lines; NurbsPatchSurface, larger MF arrays, exercises MF re-join logic.

(`#` comment diversity, compact/minimal whitespace, and field-declaration
context-sensitivity from the survey are covered across fixtures 2–4.)

## Risks / notes

- **Token re-join** is the main divergence from `XmlReader`: a field's value is a
  span of lexer tokens, not one attribute string. Get the `[]`/quote boundary
  handling right and delegate everything else to `FieldValueIO` — do not
  re-implement number/MF parsing.
- **Field-vs-node disambiguation** relies on reflection (`findField` by
  `x3dName`); a body token is a field iff it names a field on *this* node, else
  it is an error/skip. This keeps the parser node-agnostic.
- **Proto-vs-node** lookup must check the scene proto table *before* the factory,
  or proto instances misparse as unknown nodes.
- **Enum fields** must use `setEnumString`, never `parseValue` (same trap noted
  in the reflection audit).
- No golden/codegen changes required — this is runtime-only, additive. If the
  shared `X3DReader` interface is added, confirm `XmlReader`'s tests still pass
  (signature already matches).
