# Parser Gap Fix Plan — corpus-sweep failures

**Date:** 2026-06-03
**Branch:** `modernize-x3d-spec`
**Scope:** `x3d-cpp-gen` readers only (`runtime/parse/*`, `runtime/codecs/Xml*`).
No codegen changes, no golden-file changes. PROTO-instance bodies parse to the
existing carried structural record (`Scene.protoInstances`), never full
expansion.

## Source of the diagnostics

A corpus sweep over `<x3d-render-workspace>/testdata/...` ran the four
readers against ~985 conformance files. Failures were bucketed by encoding and
signature. This plan triages those buckets into **genuine `x3d-cpp-gen` parser
gaps** (fixed here) versus **out-of-scope noise** (recorded, not fixed).

### Out of scope — do NOT touch in this plan

| Diagnostic bucket | Why out of scope |
|---|---|
| `groundAngle/cutOffAngle/beamWidth/shininess above maximum` (28 files) | These "above maximum" range errors are emitted by the **renderer** at `<x3d-render-workspace>`, not by `x3d-cpp-gen`. This repo performs **no** field-range validation (verified: no `minimum`/`maximum`/`clamp`/`above maximum` strings in `generator/` or `runtime/`). The parser accepts these files. Fix belongs in the renderer's validator (use full-precision π/2 = `1.5707963267948966` and `<=` at the boundary). Noted for the renderer owner; nothing to change here. |
| `VRML 1.0 not supported` (1 file) | Intentional rejection. VRML 1.0 is a different grammar; `Vrml97Dialect` correctly refuses it. No change. |
| `JsonLite: non-X3D config files` — `package.json`, `launch.json` (2 files) | Test-corpus pollution (npm/VS Code config, not X3D-JSON). Correct rejection. No change. |
| `JsonLite: malformed JSON` (1 file) | Genuinely invalid JSON in the corpus. Correct rejection. No change. |
| `XML: EOF before closing tag` (2 files) | Genuinely truncated/unclosed X3D in the corpus. Correct rejection. No change. |

Everything below is a genuine reader gap in this repo.

---

## Ordered fix list

Ordered by corpus impact (files unblocked), highest first.

### Fix 1 — gzip inflate before sniffing (biggest win: ~30 files)

**Problem.** `X3DParse::parseFile()` (`runtime/parse/X3DParse.hpp:81-83`)
detects the gzip magic `0x1f 0x8b` and **throws** instead of inflating. ~30
valid `.wrl`/`.x3d`/`.x3dv` corpus files are gzip-stored and never reach a
reader. `Encoding::sniffByExtension` already strips `.gz`/`.gzip` and
`sniffByContent` returns `Unknown` for a gzip payload, so the only missing piece
is in-memory inflation.

**Files to change**
- **Add** `runtime/parse/tinfl.h` — bundled public-domain DEFLATE decompressor.
  Use the single-file `tinfl` from miniz (Unlicense / public domain, by Rich
  Geldreich / based on Sean Barrett's work, https://github.com/richgel999/miniz).
  Header comment must document provenance + license. ~700 lines, zero deps.
- **Add** `runtime/parse/Inflate.hpp` — header-only C++ wrapper:
  `std::string inflateGzip(std::string_view compressed)`.
  - Validate magic `0x1f 0x8b`, method byte `0x08`.
  - Parse the RFC-1952 gzip header: byte 3 = flags (FEXTRA/FNAME/FCOMMENT/FHCRC),
    skip optional fields accordingly, then the 10+ byte fixed header.
  - Hand the raw DEFLATE stream to `tinfl` (raw-deflate mode), growing the output
    buffer until `TINFL_STATUS_DONE`.
  - Return `std::string`; throw `std::runtime_error("inflateGzip: ...")` on
    truncation/corruption. CRC32 check optional (recommended, cheap).
- **Modify** `runtime/parse/X3DParse.hpp` `parseFile()`: replace the throw with
  ```cpp
  if (isGzip(bytes))
    bytes = inflateGzip(bytes); // then fall through to sniff(path, bytes)
  ```
  `sniff()` already prefers extension when content is Unknown, and the inflated
  bytes now content-sniff correctly, so no further branching is needed. Add
  `#include "Inflate.hpp"`.
- **Modify** `CMakeLists.txt`: `tinfl.h` + `Inflate.hpp` land in
  `runtime/parse/` and are picked up by the existing header install glob —
  confirm they are covered (they are header-only; no new target). No
  `target_link_libraries` change; the whole point of bundling is to avoid an
  `-lz` client dependency. (Optional follow-up, NOT in this fix: a
  `find_package(ZLIB)` compile-guard fast path. Skip for now to keep the
  header-only promise.)

**Approach notes.** Keep it header-only and dependency-free, matching the
`XmlLite.hpp`/`JsonLite.hpp` bundling pattern. Inflate happens once, in
`parseFile()`, before any sniff — `parseDocument(text)` (already-in-memory) is
unchanged since callers there pass decompressed text.

**Corpus fixtures to test against**
- `legacy/showcase2008/2008/Examples/Other/Octaga/chamber.wrl`
- `legacy/showcase2008/2008/Examples/Other/Octaga/GlassBunny.wrl`
- `legacy/showcase2008/2008/Examples/Other/3Dnpvei/3dnpveiprotos.wrl`
- `tests/floops/floops/s03/s03.wrl` … `s06/s06.wrl`

**Test to add.** `runtime/parse/tests/reader_test.cpp::testGzipInflate()`:
take an existing committed fixture (e.g. `data/wrl/NewShape.wrl`), gzip its bytes
in-process is not available without a deflater — instead **commit a small gzip
fixture** `runtime/parse/tests/data/gzip/HelloWorld.wrl.gz` (gzip of the existing
`data/x3dv/HelloWorld.x3dv` content re-saved as VRML, or any tiny valid VRML97).
Test asserts:
1. `inflateGzip(rawGzBytes)` returns text whose first line starts with `#VRML` /
   `#X3D`.
2. `parseFile(".../HelloWorld.wrl.gz")` returns a document with the expected root
   node type reachable via reflection (round-trip parity with the uncompressed
   sibling).
Add a second fixture `data/gzip/spinner.x3dv.gz` to cover the ClassicVRML path.
Wire both into the `reader_test` `add_test` data dir (or a new
`data/gzip` arg) in `CMakeLists.txt`.

---

### Fix 2 — unknown-node / PROTO-instance / EXTERNPROTO body `{` handling in the shared VRML core (~11 files)

These four signatures all surface from one shared weakness in
`ClassicVrmlReader.hpp` (also reached by `Vrml97Reader`, which subclasses it):

- `expected '{' (unknown-node body open)` (5) — header- and mid-file
  `EXTERNPROTO` and deep nesting.
- `expected node type name, got '}'` (2 ClassicVRML + 2 VRML97) — empty/incomplete
  MFNode list.
- `expected node type name, got '['` (2) — array-valued field where the model
  exposes a node-typed field (NURBS controlPoint etc.).
- `expected proto instance field name, got '{'` (1) — node-valued proto-instance
  field assigned inline.
- `expected IMPORT inlineDEF.importedDEF, got '{'` (1) — see Fix 3; the `{`
  recovery here keeps the parser alive.

**Root cause.** Several spots assume a stricter shape than the corpus uses and
`throw` instead of skip-and-recover. The codec's stated contract (header comment,
lines 26-29) is *"unknown node types / unknown field names are skipped
gracefully; only genuinely unrecoverable malformation throws."* These cases are
recoverable and currently throw.

**Files to change** — `runtime/parse/ClassicVrmlReader.hpp` only (shared by both
text readers; no Vrml97Reader change needed).

**Approach — three targeted recoveries, each preserving the carry-structure rule:**

1. **`skipBalancedBraceBlock` — tolerate a missing `{`** (covers
   `expected '{' (unknown-node body open)`). When an unknown node type is hit and
   the next token is not `{` (e.g. an `EXTERNPROTO`-declared name used as a node,
   or a malformed body), do not throw: `warn(...)` and return without consuming a
   block, OR if a `[`/value follows, route to `skipFieldValue`. Concretely, in
   `parseNode()` the `if (!node)` branch (line 288-293) should only call
   `skipBalancedBraceBlock` when `tok.peek().isPunct('{')`; otherwise warn and
   return `nullptr` so the caller continues.

2. **`parseNode()` / `applyNodeField()` MFNode element loop — tolerate a stray
   value/`]`/`}` where a node type is expected** (covers
   `node type name, got '}'` and `got '['`). In `applyNodeField`'s MFNode loop
   (lines 410-423) and in `captureInstanceNodeValue`/`captureNodeDefault`, before
   calling `parseNode`, guard: if `peek()` is `]`, `}`, a `Number`, a `String`, or
   `[`, then `skipFieldValue(tok)`-or-`break` instead of recursing into
   `parseNode` (which would throw at `expectWord("node type name")`). The SFNode
   path already has this guard (lines 396-401); replicate it for MFNode and for
   the proto node-value captures. Net effect: an empty/partial MFNode list and an
   X3D-3.x-as-raw-array field both degrade to "skip the value", matching the
   SFNode tolerance already shipped.

3. **`parseProtoInstance()` field loop — tolerate a node-valued field whose
   declaration is unknown** (covers `proto instance field name, got '{'`). At
   line 605 `expectWord("proto instance field name")` throws if the value shape
   bled past the field-name position. Harden the conservative branch (lines
   626-640): when the declaration is unknown and the next token after the field
   name is `{` (an inline node body) or a node type followed by `{`, capture it as
   a node value via `captureInstanceNodeValue(..., SFNode/MFNode)` into
   `fv.nodeValue` rather than as a raw token. Keep the result a
   `runtime::ProtoFieldValue` on the carried `ProtoInstance` — **no expansion**.

   To make this robust, add a small lookahead helper
   `bool nextIsNodeValue(tok)` — true when `peek()` is `DEF`/`USE`/`NULL`/`[`, or
   an `Identifier` whose following token is `{`. Use it to choose between
   `captureInstanceNodeValue` and the scalar/raw branch.

**Invariant to preserve.** Proto-instance bodies must still produce a
`runtime::ProtoInstance` appended to `scene.protoInstances` with
`fieldValues`/`nodeValue` populated — the structural record the model already
carries. Do not begin expanding the proto body into the node graph.

**Corpus fixtures to test against**
- `Basic/NURBS/originals/enter.wrl`, `trimmednurbs.wrl`,
  `animated_patch22.wrl`, `4ducks.wrl` (`{`/`}`/`[` recoveries)
- `Basic/NURBS/originals/animated_patch.wrl`, `nrbduck_0.wrl` (`got '['`)
- `Basic/NURBS/originals/animatedtrimmed.wrl` (proto-instance node value)
- `Bitmanagement/airbus/AirbusFamilies.wrl` (EXTERNPROTO body `{`)
- `Skin/BoxManJoeAnim.reworked1.x3dv` and the HumanoidAnimation `.x3dv` files
  (empty/incomplete MFNode)

**Test to add.** Extend `runtime/parse/tests/classic_vrml_reader_test.cpp` with
`testRecoverableMalformation()` using hand-authored snippets (no large corpus
files needed for the unit gate):
- An MFNode field with an empty list `geometry [ ]` and one with a stray scalar
  `children [ 0 0 0 ]` → parses without throwing; node count as expected (0).
- An unknown node type used **without** a `{ }` body inside a Group → skipped,
  Group still parsed, sibling after it still reached.
- A proto instance `MyProto { shape DEF S Shape { } }` where `MyProto` is
  declared with a single `field MFNode shape` → asserts
  `scene.protoInstances[0].fieldValues[0].nodeValue.size() == 1` and that the
  child carries DEF `S` (structural capture, not expansion).
Add a second snippet to `vrml97_reader_test.cpp` (the same recovery via the
subclass) covering one NURBS-style `got '['` case.

---

### Fix 3 — IMPORT statement variants (~2 files)

**Problem.** `parseImport()` (`ClassicVrmlReader.hpp:485-501`) only accepts the
single-line dot form `IMPORT inlineDEF.importedDEF [AS name]`. Two corpus
deviations break it:
- A non-standard **block** form `IMPORT { inlineDEF X importedDEF Y AS Z }`
  (1 `.x3dv` file).
- An IMPORT whose spec is immediately followed by a `{` (1 `.wrl` file:
  `inlinedxmlchris.wrl`, line 724) — i.e. an inline-node value bled into the
  IMPORT slot, currently throwing `expected IMPORT inlineDEF.importedDEF, got '{'`.

**Files to change** — `runtime/parse/ClassicVrmlReader.hpp` `parseImport()`.

**Approach.**
- After `tok.next() // IMPORT`, peek: if the next token is `{`, parse the block
  form. Consume `{`, then read key/value pairs until `}`: accept the recognized
  keys `inlineDEF`, `importedDEF`, `AS`/`as` (each followed by a word) and ignore
  unknown keys (consume the word). Populate `runtime::Import` from whatever keys
  appeared. This is a tolerant, forward-compatible reader of the block variant —
  it does not have to be a strict grammar.
- Keep the existing single-line dot path unchanged for the common case.
- Guard the single-line path: if after reading the spec word the parser is mid
  statement and a stray `{` follows (the `inlinedxmlchris.wrl` case), do not
  throw — `warn(...)` and `skipBalancedBraceBlock` (or `skipFieldValue`) to
  recover, then keep the partial `Import`. The IMPORT is still recorded
  structurally; the malformed trailing node is skipped.

Result: `scene.imports` carries the structural record in all three shapes; the
parser never aborts the whole document over an IMPORT variant.

**Note on "inline field-level DEF" (the 10-file `.x3dv` bucket).** The diagnostic
describes `material MAT Material {...}` failing because the parser allegedly wants
`material DEF MAT Material {...}`. **The current code already handles bare
`DEF name Type {}` via `parseNode()` (lines 262-265).** The actual failure shape
in those files is the *three-token without `DEF`* sequence
`fieldName defName Type {}`. This is **not legal ClassicVRML** (a DEF requires the
`DEF` keyword), so the right behavior is tolerant recovery, **not** inventing a
new grammar that guesses DEF semantics from a bare identifier (that would
mis-parse legitimate `fieldName Type {}` whenever a node type name is unknown).
Fix 2's `applyNodeField` hardening already makes these degrade to a skipped value
instead of a hard throw. **Do not** add speculative inline-DEF inference; record
this decision here and let Fix 2 absorb the bucket. If, after Fix 2, specific
files still fail with a recoverable shape, handle them as a follow-up with real
file evidence rather than from the inferred root-cause text.

**Corpus fixtures to test against**
- `stylesheets/java/examples/SmokeTestProgramOutput_CommandLine.x3dv` (block IMPORT)
- `Basic/Inline/originals/inlinedxmlchris.wrl` (IMPORT followed by `{`)
- The `JavaLBExamples/TouchSensorIsOverEvent.x3dv` and
  `SmokeTestProgramOutput.x3dv` files (the bare-three-token bucket — should now
  parse via Fix 2 recovery, asserted as "no throw").

**Test to add.** In `classic_vrml_reader_test.cpp::testImportVariants()`:
- single-line `IMPORT Inl.Imported AS Local` → existing parse, assert
  `scene.imports[0]` fields.
- block `IMPORT { inlineDEF Inl importedDEF Imported AS Local }` → assert the
  same `runtime::Import` is produced.
- `IMPORT Inl.Imported { ... }` malformed → parser does not throw; one import
  recorded; the document's following statement still reached.

---

### Fix 4 — XML DTD internal subset in the prolog (3 files)

**Problem.** `XmlLite::skipProlog()` (`runtime/codecs/XmlLite.hpp:190-196`) skips
a `<! ... >` declaration with `src_.find('>', pos_)` — the **first** `>`. A
DOCTYPE with an internal subset —
`<!DOCTYPE X3D PUBLIC "..." "..." [ <!ENTITY ...> <!ELEMENT ...> ]>` — contains
`>` characters **inside** the `[ ... ]` subset. The skipper stops at the first
inner `>`, leaving `pos_` mid-subset; the next non-space char is not `<`, so
`parse()` throws `XML: expected root element` (3 corpus files).

**Files to change** — `runtime/codecs/XmlLite.hpp` `skipProlog()` only. (This is
the XML reader used by `XmlReaderAdapter`; the in-scope "fixFiles" entries that
pointed at corpus `.x3d` paths are test inputs, not code — the code fix lives
here.)

**Approach.** In the `<!` branch, detect an internal subset: scan forward from
`pos_`; if a `[` appears before the closing `>`, skip to the matching `]` first
(tracking only `[`/`]` depth — the subset has no nested `[`, but be defensive),
then skip to the `>` that follows. If no `[` appears before `>`, keep the current
fast path. Pseudocode:
```cpp
} else if (src_[pos_ + 1] == '!') {
  std::size_t close = src_.find('>', pos_);
  std::size_t bracket = src_.find('[', pos_);
  if (bracket != npos && (close == npos || bracket < close)) {
    std::size_t endSubset = src_.find(']', bracket);
    if (endSubset == npos) throw std::runtime_error("XML: unterminated DTD subset");
    close = src_.find('>', endSubset);
  }
  if (close == npos) throw std::runtime_error("XML: unterminated <! ...>");
  pos_ = close + 1;
}
```
No DTD *validation* is added — entities/elements declared in the subset are still
ignored (consistent with the "structure-only XML, no DTD validation" contract in
the XmlLite header). We only need to skip the subset correctly so the root
element is found.

**Corpus fixtures to test against**
- `stylesheets/test/SpinGroupInternalSubsetDeclaration.x3d`
- `tools/canonical/test/testFiles/TestInternalDTDQuadTreeExamples.x3d`
- `tools/canonical/test/testFiles/TestInternalSubsetDeclaration.x3d`

**Test to add.** In the XML reader test (`runtime/parse/tests/reader_test.cpp`,
or the codecs XML test), `testDoctypeInternalSubset()`: feed
```xml
<?xml version="1.0"?>
<!DOCTYPE X3D PUBLIC "ISO//Web3D//DTD X3D 4.0//EN" "x3d-4.0.dtd" [
  <!ENTITY foo "bar">
  <!ELEMENT baz EMPTY>
]>
<X3D profile="Interchange" version="4.0"><Scene><Shape/></Scene></X3D>
```
through `parseDocument(..., Encoding::XML)` and assert the root resolves and a
`Shape` is reachable — i.e. no `expected root element` throw.

---

## Execution order & gating

1. **Fix 1 (gzip)** — largest unblock; isolated to `parseFile` + two new headers.
2. **Fix 2 (VRML core recovery)** — shared `ClassicVrmlReader.hpp` hardening;
   benefits both text readers.
3. **Fix 3 (IMPORT variants)** — small, depends on Fix 2's recovery helpers
   (`skipBalancedBraceBlock`/`skipFieldValue`) being the chosen recovery path.
4. **Fix 4 (XML DTD subset)** — independent; can land any time.

Each fix adds a focused unit test (hand-authored snippets + one committed binary
gzip fixture for Fix 1) so the CI gate doesn't depend on the external
`<x3d-render-workspace>` corpus. The corpus fixtures listed per fix are the
**manual** re-run validation set, not committed into this repo.

**No codegen, no golden-file regeneration.** All changes are in the runtime
parse/codec layer and its tests.

## Expected impact

| Fix | Files unblocked (approx) |
|---|---|
| 1 gzip | ~30 |
| 2 VRML core recovery | ~11 (+ absorbs the 10-file bare-DEF `.x3dv` bucket as no-throw) |
| 3 IMPORT variants | ~2 |
| 4 XML DTD subset | 3 |

Out-of-scope buckets (renderer range validation, VRML 1.0, non-X3D config
files, truncated files) account for the remainder and are intentionally left.
