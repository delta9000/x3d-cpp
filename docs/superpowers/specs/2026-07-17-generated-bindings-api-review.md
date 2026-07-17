# Generated C++ Bindings — API Ergonomics Review

**Date:** 2026-07-17
**Status:** external review, captured for triage (not yet actioned)
**Subsystem:** `generated_cpp_bindings/` (the codegen'd node layer, `src/x3d_cpp_gen/templates/`)
**Relates to:** ADR-0012 (value types stay arithmetic-free), ADR-0039 (namespace split
`x3d::core` / `x3d::nodes`), `docs/wiki/subsystems/reflection.md`, the `RuntimeSession`
CHANGELOG entry (the exact silent-setter trap it was built to prevent).

## Context

A holistic review of the generated node API's ergonomics — not spec conformance (that's
`docs/conformance/findings.yaml`), not structural correctness (that's the golden gate) —
but "if a new embedder sits down and starts calling these generated setters/getters, where
do they get burned?" Captured verbatim below for triage; no code changes made yet.

## Holistic verdict

The bindings are a competent data layer with one genuinely sharp edge and one genuine
type-safety hole. They're easy to read, easy to predict, and slightly too easy to misuse.

## What's genuinely good

**Total pattern uniformity.** Learn one node and you know all 680: get/set for
`inputOutput`, `setXUnchecked` for `initializeOnly`, `onX` + `setOnXHandler` for
`inputOnly`, `emitX` for `outputOnly`, plus `getDefaultX()`, `acceptableXNodeTypes()`, and
a `fields()` reflection table. The four X3D access types map to four visibly distinct API
shapes — the spec's field semantics are legible at the call site. That uniformity is the
real payoff of codegen and it delivers.

**The doc comments carry the spec.** Every accessor quotes the field's normative prose and
links the component clause. Reading `Transform.hpp` is reading §Grouping. For a
spec-driven SDK, that's the right content in the right place.

**The dual API can't drift.** Typed accessors for humans + type-erased thunks for
codecs/cascade, generated from the same model — the reflection row in `Transform.cpp` is
the typed setter, so the two paths can't disagree. Bounded enums are real `enum class`
vocabularies, not strings.

**Real usage confirms it works.** `examples/asset_import/emit.cpp` reads exactly like
you'd hope: `make_shared<PhysicalMaterial>()`, `setBaseColor(...)`, `setCoord(coord)`.
Verbose, obvious, no surprises.

## The friction, ranked

**1. Setters are silent — the sharpest edge.** `setTranslation()` just assigns
(`Transform.hpp:191`). No cascade seed, no change event. The runtime only observes writes
that go through `ctx.writeField`/reflection. So there are two write paths with different
observability semantics, and the typed setter — the one every C++ author instinctively
reaches for — is the invisible one. This is by design (data layer vs. behavior layer), and
extraction still sees the writes next tick, so scenes render fine — but ROUTE-driven
behavior silently doesn't react. The `RuntimeSession` CHANGELOG entry exists because of
this exact class of trap. This will be the most-repeated support question at 0.1.0.

**2. `SFNode` fields erase the child type.** `Appearance::setMaterial(const SFNode&)`
(`Appearance.hpp:315`) takes the base pointer — `setMaterial(std::make_shared<Transform>())`
compiles. Child-type correctness lives in `acceptableMaterialNodeTypes()` as strings,
checked nowhere in the setter. For a layer whose pitch is "the UOM constrains
declarations," this is the one place the type system gives up where it could have helped
(a `shared_ptr<X3DMaterialNode>` overload would be a template change, not a redesign).

**3. The value vocabulary is dumb past usefulness.** `SFVec3f` has no constructors and —
checked — no `operator==` anywhere in `X3Dtypes.hpp`. ADR-0012 rightly keeps arithmetic
out of core, but equality is vocabulary, not math. Every consumer will write their own
component-wise compare with their own epsilon habits.

**4. `getChildren()` returns `MFNode` by value** (`X3DGroupingNode.hpp:164`) — every read
copies the vector and bumps every refcount. Template uniformity chose safety over cost;
for grouping-heavy scenes polled per tick it's pointless churn. A `const MFNode&` getter
costs nothing.

**5. Smaller, owned smells:** `using namespace x3d::core` inside `namespace x3d::nodes` in
every header (ADR-0039 owns this; still injection into every consumer TU);
`getContainerFieldType()` returning generator noise like
`"containerFieldChoicesGroupLODShapeTransformSwitch"` — a UOM symbol mashup leaking into a
public API as if it were human-readable; a `dynamic_cast` in every reflection thunk
(correct, but the codecs' hot path pays it); and `emitHitNormal_changed` exposing the
`_changed` wire-naming convention as C++ API.

**6. Boilerplate ratio.** 340 headers, ~84k lines, ~247 lines each — `Transform` is 254
lines for five fields. The doc prose justifies much of it, but `getDefaultX` statics +
member-init + the same prose repeated on getter and setter makes IDE hover noisy. It reads
as generated because it is; fine, but worth a template diet someday.

## What I'd actually do

- **Pre-0.1.0 (cheap, high value):** `operator==`/`!=` on the `SF*` aggregates — one
  template change, removes a guaranteed consumer re-invention. `const MFNode&` for
  `MFNode` getters — same. Neither breaks API at 0.x where you're already breaking.
- **Document, don't fix:** the silent-setter split. One paragraph in the wiki's
  generated-bindings page saying "typed setters are the data layer; they never seed the
  cascade — use `ctx.writeField` when behavior must react." The design is defensible; the
  silence is the bug.
- **Post-0.1.0 (worth a card):** typed `SFNode` setter overloads
  (`shared_ptr<X3DMaterialNode>`), a `clone()` with named deep/shallow semantics
  (currently implicit shared-child copies), and deleting or renaming
  `getContainerFieldType()`.

## Bottom line

Nothing here is a release blocker — the layer is honest about what it is, and `emit.cpp`
proves a real tool can author with it without contortions. But #1 and #2 are the two
places a new embedder will burn an afternoon, and both are knowable today.
