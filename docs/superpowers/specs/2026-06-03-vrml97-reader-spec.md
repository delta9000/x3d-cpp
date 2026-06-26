# VRML97 (.wrl) Reader — Design Spec

**Date:** 2026-06-03
**Status:** Proposed
**Branch:** `modernize-x3d-spec`

## Goal

Add a first-party reader that ingests VRML97 (ISO/IEC 14772-1, `#VRML V2.0 utf8`)
`.wrl` files into the existing runtime document model (`runtime::X3DDocument`),
reusing the codec architecture (`X3DNodeFactory`, reflection `FieldInfo` thunks,
`FieldValueIO`) so that **zero per-node code** is added. The output is a normal
`X3DDocument` that the existing XML/JSON/VRML writers and the event runtime can
consume — i.e. VRML97 in, X3D out, no separate object model.

VRML97 and X3D ClassicVRML share ~90% of their concrete syntax (brace-delimited
`Type { field value }` nodes, `DEF`/`USE`, `ROUTE`, `PROTO`/`EXTERNPROTO`,
`#` comments, identical token/value lexis). The two encodings differ only in the
header, a small set of node/field renames, and the `eventIn/eventOut/field/
exposedField` access-type keywords. So the tokenizer/parser is built **once** and
shared; the VRML97-ness lives entirely in a thin dialect layer (header acceptance
+ a static mapping table) layered on top.

## Scope (compliance)

**In scope (must parse and map):**
- Header `#VRML V2.0 utf8` (and tolerantly `#VRML V1.0`/missing → warn, attempt).
- All 54 VRML97 nodes (the X3D Immersive baseline) via `X3DNodeFactory`.
- `DEF`/`USE`, `ROUTE x.f TO y.g`, nested SFNode/MFNode fields, `NULL`.
- All SF*/MF* value syntax (delegated to `FieldValueIO::tokenize`/`parseValue`).
- `PROTO`/`EXTERNPROTO`/proto instances and `IS` connections — parsed into
  `runtime::Proto*` structures (carried, **not expanded**; same status as XML).
- `Script` nodes with inline `field`/`eventIn`/`eventOut`/`url` declarations —
  parsed into the model as data (see Script handling).
- `#` line comments; whitespace/comma delimiters; quoted/escaped strings.
- gzip-compressed `.wrl` (transparent inflate on a gzip magic-byte sniff).

**Out of scope (this stage):**
- PROTO/Script **execution or expansion** (deferred; identical to current XML
  reader limitations — `ProtoInstance::expand()` remains a stub).
- VRML 1.0 semantics (separators, `WWWInline`, etc.) — detected and rejected
  with a clear error, not silently mis-parsed.
- VRML97 conformance *validation* (profile/level/bounds). Values are set via
  reflection thunks exactly as the XML reader does; no constraint checking.

## Architecture

Header-only, `namespace x3d::codec`, mirroring `XmlReader.hpp`. New files under
`runtime/codecs/`:

- **`VrmlLexer.hpp`** — token stream over the input string. Tokens: `LBRACE`,
  `RBRACE`, `LBRACKET`, `RBRACKET`, `IDENT`, `STRING` (quoted, `\"`/`\\`
  escapes), `PERIOD`, `EOF`. Skips `#`-to-EOL comments (except the first-line
  header, handled before lexing) and whitespace/commas. This is the single
  shared primitive for both VRML97 and X3D ClassicVRML — there is currently **no**
  ClassicVRML reader, so building it here closes that gap too (a `VrmlReader`
  consuming `.x3dv` is the same parser minus the dialect remap).
- **`VrmlParser.hpp`** — recursive-descent parser producing `runtime::X3DDocument`.
  Drives the same construction pattern as `XmlReader`:
  1. `IDENT {` → `X3DNodeFactory::create(mapNodeName(IDENT))`.
  2. inside braces, `IDENT value...` → look up `FieldInfo` by mapped `x3dName`,
     parse the value region (scalar via `FieldValueIO`, node via recursion,
     `[ ... ]` for MF), set via the `set`/`setEnumString` thunk.
  3. `DEF Name Type {…}` → `Scene::define(Name, node)`; `USE Name` →
     `Scene::resolve(Name)`; `ROUTE` → push `runtime::Route`; resolve post-parse
     via `Scene::resolveRoutes()`.
  Containers: VRML97 child fields are **always explicit** (`children [...]`,
  `geometry X{}`, `appearance Appearance{}`), so children route by field name —
  no `containerField` guessing needed. The parser reuses the `FieldInfo`-by-name
  lookup; the XML reader's positional fallback is unnecessary here.
- **`Vrml97Dialect.hpp`** — the dialect layer: header recognition + the static
  VRML97→X3D mapping table (below). `VrmlReader` (entry point) = lexer + parser
  + this table; an X3DV reader is the same minus the table.

Entry point parallels `XmlReader`:

```cpp
class VrmlReader {
public:
  runtime::X3DDocument readDocument(const std::string &wrlText);
  // throws std::runtime_error on a non-VRML / VRML 1.0 header.
  // collects non-fatal issues in `warnings()`.
  const std::vector<std::string> &warnings() const;
};
```

## Header handling

The first line is consumed **before** lexing (it begins with `#`, which the lexer
would otherwise treat as a comment):

- `#VRML V2.0 utf8` → accept; set `doc.version` to a VRML97-equivalent marker and
  default `doc.profile = Immersive` (VRML97 ≡ Immersive baseline). Charset other
  than `utf8` → warning, proceed as UTF-8.
- `#VRML V1.0 …` → throw `std::runtime_error("VRML 1.0 not supported")`.
- `#X3D V3.x/V4.x …` → accept and **switch off** the VRML97 remap (it's actually
  ClassicVRML), so the same entry point reads `.x3dv` correctly.
- Missing/garbled first line → warning, assume VRML97, attempt parse.

On output the document carries no VRML97 header; writers emit `#X3D V4.0 utf8`
as today. The reader is a one-way VRML97→X3D bridge.

## VRML97 → X3D node/field mapping table

A single `static const` table in `Vrml97Dialect.hpp`, applied during lookup:
`mapNodeName(token)` and `mapFieldName(nodeType, token)`. VRML97 was deliberately
forward-compatible, so the table is **small** — most names are identical and pass
through unchanged. Known deltas:

| Kind | VRML97 | X3D | Notes |
|------|--------|-----|-------|
| access kw | `eventIn` | `inputOnly` | Script/PROTO interface decls |
| access kw | `eventOut` | `outputOnly` | |
| access kw | `field` | `initializeOnly` | |
| access kw | `exposedField` | `inputOutput` | |
| node | (none removed) | — | all 54 VRML97 nodes exist verbatim in X3D 4.0 |
| field | `Switch.whichChoice` | `whichChoice` | identical (listed to confirm) |
| field | `LOD.level` | `children` | X3D renamed `level`→`children`; alias |
| field | `Switch.choice` | `children` | X3D renamed `choice`→`children`; alias |
| field | `*.bboxSize/bboxCenter` | same | identical |
| literal | `TRUE`/`FALSE` | `true`/`false` | normalized in value parse |

Mapping rules:
- The table is **deny-by-omission-safe**: any token not in the table is passed
  through unchanged and resolved against reflection. This keeps the table to the
  handful of true renames rather than enumerating all 54 nodes.
- Access-type keywords are normalized in the PROTO/Script interface grammar only.
- `LOD.level` / `Switch.choice` are the only node-field aliases that matter for
  the corpus; both map to `children`. Implemented as
  `mapFieldName("LOD","level")→"children"` etc.
- Boolean literals `TRUE`/`FALSE` → `true`/`false` are handled in the value layer
  (a 2-entry normalization before `FieldValueIO::parseValue` for SFBool/MFBool).

The table is authored by hand (it is tiny and stable) rather than generated; it
ships with a unit test that asserts every mapped X3D target name resolves to a
real `FieldInfo`/factory type, so spec drift in the generated bindings fails CI.

## Script handling

Scripts are **parsed as data, not executed** (consistent with the project's
current no-eval stance and the XML reader). For a `Script { … }`:

- Instantiate `Script` via the factory.
- `url` (MFString) → set via reflection as a normal field; inline ECMAScript/
  Java/Python source given as a `"javascript: …"` / `"ecmascript: …"` string URL
  is preserved verbatim in the MFString — no CDATA concept in VRML97.
- Inline interface declarations
  (`eventIn SFFloat set_x`, `field SFInt32 n 0`, `eventOut …`) are dynamic,
  per-instance fields not present in the generated `Script` reflection table.
  They are captured into the node's `runtime::Proto`-style user-field list
  (the same structure used for PROTO interface fields) so they round-trip and so
  `IS`/`ROUTE` endpoints referencing them resolve. They are **not** turned into
  reflection fields and **not** evaluated.
- `directOutput`, `mustEvaluate`, `url` parse as their declared `Script` fields.

This means a Script-bearing `.wrl` loads, its ROUTEs validate against the
declared event names, and it serializes back out — but no behavior runs. An
execution engine is a later sub-project (see event-system design doc).

## Unmapped / unknown reporting

Failures are **non-fatal and collected**, never silent (this is the key
improvement over `XmlReader::applyAttribute`, which drops unknowns silently):

- **Unknown node type** (factory returns null after mapping): skip the node's
  entire `{…}` block (balanced-brace skip), push
  `"unknown node 'Foo' at line N (skipped)"` to `warnings()`. If the node was a
  `DEF`, the name is left unbound so later `USE` of it also warns.
- **Unknown field** (no `FieldInfo` after `mapFieldName`): skip just that field's
  value region, warn `"unknown field 'bar' on Foo at line N"`.
- **Unresolved DEF / ROUTE endpoint**: tolerated exactly as today — `resolveRoutes`
  leaves the `weak_ptr` empty; a warning is added per unresolved endpoint.
- **Value parse error** (bad token for the field type): warn with line + field,
  leave the field at its default (do not abort the document).
- **Hard errors** (only): bad header (VRML 1.0 / non-VRML), unbalanced
  braces/brackets at EOF, lexer EOF mid-token → `throw std::runtime_error`.

`warnings()` returns the accumulated list; callers (and tests) can assert on it.
A strict mode flag (`setStrict(true)`) promotes warnings to throws for CI fixtures
that must parse cleanly.

## Corpus fixtures (`.wrl`)

The `x3d-render` corpus has **640 `.wrl` files** (606 text + ~30 gzip). Reader
unit tests pin these representatives (paths absolute):

1. `$X3D_CORPUS_DIR/x3d-code/www.web3d.org/x3d/tools/Vrml97ToX3dNist/test/NewShape.wrl`
   — minimal Shape/Geometry/Appearance; smoke test of node+field+nesting.
2. `$X3D_CORPUS_DIR/x3d-code/www.web3d.org/x3d/tools/Vrml97ToX3dNist/test/TranslationTestScene.wrl`
   — broader node coverage; NIST VRML97→X3D reference (has an X3D translation to
   diff against).
3. `$X3D_CORPUS_DIR/x3d-code/www.web3d.org/x3d/tools/Vrml97ToX3dNist/test/Rollers.wrl`
   — `DEF`/`USE` + `ROUTE` + interpolators; exercises route resolution.
4. `$X3D_CORPUS_DIR/x3d-code/legacy/showcase/Examples/Other/Followers/Chasers.wrl`
   — `EXTERNPROTO` + field declarations + comments; PROTO carry-through.
5. `$X3D_CORPUS_DIR/x3d-code/legacy/showcase/Examples/Other/Followers/Dampers.wrl`
   — companion PROTO/comment-heavy file; whitespace/comment robustness.
6. `<x3d-render-workspace>/tests/floops/floops/s36/s36.wrl`
   — gzip-compressed (~1.47MB inflated); transparent-inflate path + large MF
   arrays.
7. `<x3d-render-workspace>/tests/floops/floops/s47/s47.wrl`
   — second gzip fixture (~1.27MB); confirms inflate is not file-specific.

Test assertions per fixture: (a) parses without throwing (strict for #1–#3),
(b) expected root node count / a sampled field value, (c) `warnings()` empty for
clean fixtures, (d) re-serialize via `VrmlWriter`/`XmlWriter` and re-read to prove
the produced `X3DDocument` is internally consistent (no VRML97↔VRML97 round-trip
is claimed — output is X3D). Fixture #2 additionally diffs against its committed
`…ToX3dTranslated.x3d` sibling where present.

## Out-of-scope / risks (carried forward)

- No PROTO/Script execution; instances are data only.
- No constraint/bounds validation (reflection thunks set blindly).
- Float precision follows `FieldValueIO` (`stof`/`stod`); same as existing codecs.
- gzip support needs an inflate dependency or a tiny bundled inflater; if a
  zlib dep is undesirable, gzip fixtures (#6, #7) are gated behind a build flag.
