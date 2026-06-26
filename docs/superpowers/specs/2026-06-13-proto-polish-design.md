# PROTO Polish: Nested Instances + Encoding Capture/Emit Parity

**Date:** 2026-06-13
**Branch:** `modernize-x3d-spec`
**Status:** design approved, pre-implementation
**Predecessor:** `2026-06-12-proto-expansion-design.md` (M1 STEP 1 — PROTO/EXTERNPROTO expansion, COMPLETE at commit `511e972`)

## Context

M1 STEP 1 landed local PROTO + relative-file EXTERNPROTO expansion with full IS
wiring (value-forward, body routes, interface-event redirects), plus
`<ProtoInstance>` re-emit on the XML writer. It left four deferred polish gaps.
The user selected three of them for this push and explicitly **deferred the
fourth** (http/urn EXTERN resolver), which stays an embedder seam — shipping
network I/O in a headless SDK cuts against the renderer-agnostic / IO-free ethos
(see `x3d-cpp-gen-end-goal`: "asset-resolver seam — embedder supplies bytes by
URL").

This push is **entirely hand-written runtime** — `runtime/`, `runtime/parse/`,
`runtime/codecs/`, and the proto model header. **No codegen is touched, so the
golden header tree must stay byte-identical** (`mise run build` golden gate). Any
golden drift is a bug, not an intended change.

## Scope (the three selected items)

1. **Nested-in-body ProtoInstances** — expand a `<ProtoInstance>` that appears
   inside another proto's `<ProtoBody>`, once per outer instantiation.
2. **VRML/JSON reader PROTO capture** — JSON is the real gap; VRML97 `.wrl` is
   verification-only (inherits ClassicVrml's capture).
3. **Writer parity** — XmlWriter gains `<ProtoDeclare>` body re-emit; JsonWriter
   and VrmlWriter gain `ProtoInstance` **and** `ProtoDeclare` re-emit so all four
   encodings round-trip PROTO end to end.

**Out of scope (deferred, by decision):** http/urn EXTERN resolver (embedder
seam); IS-connected `fieldValue`s on a nested instance (documented deferral).

## Findings that shaped the design

- **Nested-in-body is broken in both readers today.** `XmlReader.hpp:303` (direct
  body child) and `:185` (under a body node), and `ClassicVrmlReader.hpp:292-296`,
  push a body-nested instance into the **flat** `scene.protoInstances` list with
  `parent` pointing at the **original, un-cloned** body node (or null). At
  `expandScene`: a null-parent body instance is spliced as a **scene root**
  (pollutes the document); a parented one targets the **template** body node, not
  each per-instantiation clone. So nested instances never appear inside each
  expanded copy of the outer proto.
- **VRML97 `.wrl` PROTO capture likely already works.** `Vrml97Reader : public
  ClassicVrmlReader` inherits `readDocument` whole; ClassicVrml has full PROTO
  capture and `mapAccessType` already handles VRML `eventIn`/`eventOut`/`field`/
  `exposedField` spellings. → verification + dialect edge cases only.
- **Only XmlWriter re-emits PROTO.** `JsonWriter`/`VrmlWriter` emit nothing for
  proto; XmlWriter re-emits `<ProtoInstance>` (from `scene.expandedSources`) but
  not the `<ProtoDeclare>` body.

## Design

### 1. Model (`runtime/X3DProto.hpp`)

Add to `ProtoBody`:

```cpp
std::vector<ProtoInstance> nestedInstances; // ProtoInstances inside this body
```

A `ProtoInstance` nested inside a `<ProtoBody>` belongs to the body **template**,
not the flat scene list. Each nested instance keeps its existing fields; `parent`
is the **original** body node it sits under (already a `cloneMap` key during
expansion) and `parentField` is the containerField slot. A direct body-root child
has a null `parent`.

### 2. Expansion engine (`runtime/X3DProtoExpand.hpp`)

In `expandInstance`, after the body deep-clone (the `cloneMap` of
`const X3DNode* -> clone` is already built), expand each
`decl->body.nestedInstances` entry:

- Recurse through `expandInstance(...)` using the **existing `ExpandGuard`**
  (depth++), so recursion/cycles are already bounded by `maxDepth`.
- Resolve placement against the clone: `clonedParent =
  cloneMap[nested.parent.lock().get()]`; then
  `proto_detail::attachToParent(clonedParent, nested.parentField, nestedPrimary)`.
- Keep the lenient backstop: a throwing nested expansion becomes a `ProtoWarning`
  and does not abort the outer expansion.

**Placement cases:**

- **Case A — nested under a regular body node** (`<Transform><ProtoInstance/>
  </Transform>`): the common, unambiguous case. Original parent is a real body
  node → present in `cloneMap` → attach to its clone. **Fully handled.**
- **Case B — direct body-root child (auxiliary), null parent:** expand
  best-effort; attach to the primary via the nested instance's containerField if
  resolvable, else keep alive and emit a `ProtoWarning`. **Lenient.**
- **Case C — nested instance is the body's *first* node (the primary itself is a
  proto instance):** expand it; its primary becomes the outer expansion's
  `primary`. Emit a `ProtoWarning` if this collides with other body nodes'
  expectations. **Lenient / best-effort.**

IS-connected `fieldValue`s on a nested instance are a **documented deferral**
(literal `fieldValue`s — the common case — are forwarded normally).

### 3. Readers route body-nested instances into the current `ProtoBody`

The body-routing rule is uniform: while reading a `<ProtoBody>`, a detected
`ProtoInstance` is appended to that body's `nestedInstances` (parent = original
body node, or null for a direct body-root child), **not** to
`scene.protoInstances`. At scene scope, behavior is unchanged.

- **ClassicVrml (`ClassicVrmlReader.hpp`):** `currentProtoBody` is already
  threaded (for IS connections). When non-null, route the instance detected in
  `parseNode` (`:286-298`) into `currentProtoBody->nestedInstances` instead of
  `scene.protoInstances`.
- **XmlReader (`XmlReader.hpp`):** thread a `runtime::ProtoBody* currentProtoBody`
  param through `readProtoBody -> readNode -> readChildren`. At the two instance
  sites (`:303`, `:185`), route into the current body when non-null.
- **JsonReader (`runtime/parse/JsonReader.hpp`) — new capture:** add
  ProtoDeclare / ExternProtoDeclare / ProtoInstance / fieldValue / IS handling
  mirroring XmlReader, including the same body-routing and the same
  declaration-resolution-for-fieldValue-typing logic (find local decl, else
  extern, else SFString fallback).

### 4. VRML97 `.wrl` — verify, don't rebuild

Add a TDD test proving a `.wrl` document with a PROTO declaration + instance reads
and round-trips through the inherited ClassicVrml path. Touch code only if a
dialect-specific edge case fails the test.

### 5. Writers — parity across all four encodings

- **XmlWriter (`XmlWriter.hpp`):** add `<ProtoDeclare>` / `<ExternProtoDeclare>`
  re-emit. Reconstruct `<ProtoInterface>` from `decl->interface` (each
  `<field name type accessType value/>`), `<ProtoBody>` from the **template**
  `body.nodes` + `body.routes`, and IS connections as `<IS><connect
  nodeField protoField/></IS>` emitted inside the matching body node (matched by
  `IsConnection.node` pointer). Re-emit the body **template**, never the expanded
  clones. **This updates the existing `proto_roundtrip_test`**, which currently
  asserts the body is *not* emitted (that assertion was the known-limitation
  marker and is now obsolete).
- **JsonWriter (`JsonWriter.hpp`) / VrmlWriter (`VrmlWriter.hpp`):** add
  `ProtoInstance` + `ProtoDeclare` re-emit in each encoding's native syntax,
  reaching parity with the XmlWriter. Instance re-emit reads from
  `scene.expandedSources` (matching XmlWriter's mechanism); declaration re-emit
  reads from `scene.protoDeclarations` / `scene.externProtoDeclarations`.

### 6. Verification

- TDD (red -> green) per component.
- Full `mise run build` (pytest + ctest, Ninja + ccache dev preset).
- **Golden byte-identical gate** — runtime-only change, so the sorted-*.hpp
  sha256 must equal the current golden unchanged.
- Corpus smoke vs the standing baseline for **zero parse regressions**:
  XML 16886/16891, ClassicVRML 90/90, VRML97 687/688, JSON 72/75
  (harness: `scripts/corpus_sweep.cpp`, corpus `<x3d-render-workspace>/testdata`).

## Execution shape

The model change (#1) lands first as the shared dependency. After that the
writers (×3), the JSON reader, and the expansion engine are largely independent —
a good fit for the subagent-driven execution this project already uses. Reader
body-routing (#3) depends on #1; the XmlWriter test update depends on #5's
XmlWriter change.

## Risks / watch-items

- **Golden drift** — if any change accidentally pulls into a generated template,
  the golden gate fails. Keep all edits in `runtime/**`.
- **`ProtoInstance` ambiguity** — there are two `ProtoInstance` types (generated
  `generated_cpp_bindings/ProtoInstance.hpp` node + hand-written
  `x3d::runtime::ProtoInstance` struct). Fully-qualify `x3d::runtime::ProtoInstance`
  in any TU that pulls in both.
- **`proto_roundtrip_test` expectation flip** — the test currently forbids the
  declaration body in XML output; #5 makes it required. Update the test as part of
  the same change, not separately.
- **Recursion depth** — nested expansion shares the existing `ExpandGuard`
  (`maxDepth = 32`); deeply self-referential protos terminate with a
  `RecursionLimit` warning rather than overflowing.
