---
title: "ADR-0014: Dynamic Field Foundation for Script Author Fields"
summary: Author Script <field> declarations are stored as per-instance dynamic fields in a side-table; event cascade and codecs route over effectiveFields() to see both static and author fields.
tags: [adr, dynamic-fields, script, effective-fields, author-fields]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/system-script-sai.md
  - ../subsystems/routes.md
---

# ADR-0014: Dynamic Field Foundation for Script Author Fields

## Status

Accepted — implemented in commit `20d6382` (Phase 0, 2026-06-17), completed across Phase 1 commits `6418b0f`, `f4af46d`, `bedef06`, `f4e11ad`, and Phase 2 integration `f18da0d`.

## Context

A Script node's interface is defined entirely by its **author-declared fields** — `<field name accessType type>` children (XML), `eventIn`/`eventOut`/`field` interface declarations (VRML), or `-field` members (JSON). These exist only in the document. The generator's `FieldTable` is a static per-type table covering only spec-defined fields (`url`, `directOutput`, `mustEvaluate`); author fields appear nowhere in it.

Every consumer that resolves field identity walks `node.fields()` and matches by `x3dName`. This includes ROUTE endpoint resolution in `X3DSceneBridge::findField` and script get/set/route in `SaiContext::findField`. Before this decision, every reader silently dropped `<field>` declarations, every ROUTE targeting an author field was rejected as "unknown field", and file-authored Script nodes were completely inert — their handlers never fired.

Two structural constraints shaped the choice:

- **`X3DNode::fields()` is `virtual const FieldTable&` returning a reference to a function-local `static` table** — one table shared across all instances of a type. It cannot hold per-instance author fields; a per-instance channel is required.
- **The `std::any` field-value channel already exists.** `FieldInfo.get`/`set` thunks box and unbox values through `std::any` (defined in `generated_cpp_bindings/x3d/core/X3DReflection.hpp`). Author field values can reuse this same channel exactly, so routing and cascade delivery treat author fields identically to generated ones.

The early spec (`docs/superpowers/specs/2026-06-05-dynamic-field-foundation-spec.md`) proposed placing `std::vector<DynamicFieldSlot> userFields_` on the `X3DNode` base and emitting the additions via the class template (Option A). That approach was revisited before implementation. See the decision below.

## Decision

**Author-declared Script fields are stored in a process-global side-table keyed by node identity (`const X3DNode*`), not on the `X3DNode` base class itself.** The foundation is two interface seams in `runtime/events/DynamicField.hpp`:

1. **`AuthorFieldDecl`** — a neutral, encoding-agnostic struct every reader emits for one author field declaration: `{ x3dName; X3DFieldType type; AccessType access; std::any initialValue }`. This decouples the four readers from the storage model.

2. **`DynamicFieldStore`** — a per-node side-table mapping `const X3DNode*` to a `NodeEntry` holding a live `std::unordered_map<std::string, std::any>` value store and a vector of synthesized `FieldInfo` entries. The synthesized `FieldInfo` obeys the existing reflection contract exactly: `get` is empty for `inputOnly`; `set` is empty for `outputOnly`/`initializeOnly`. Readers call `dynamicFieldStore().addAuthorField(node, decl)` at parse time.

3. **`effectiveFields(const X3DNode&)`** — a free function (not an `X3DNode` method) that returns the node's static `fields()` table concatenated with its author `FieldInfo`s from the process-global store. The two field-resolution sites — `X3DSceneBridge detail::findField` (ROUTE endpoint resolution) and `SaiContext::findField` (SAI get/set/route) — switch from `node.fields()` to `effectiveFields(node)`. Other `node.fields()` callers (extraction, bounds, range-validate, material/texture) remain on `fields()` because they operate exclusively on generated geometry/appearance nodes and never encounter author fields; the switch is deliberate, not blanket.

The type-safety guard `anyMatchesFieldType` (added 2026-06-17, AUD-MEM-1) validates the declared `X3DFieldType` against the boxed `std::any` type on every `setValue` call, counting drops rather than crashing, to make invariant violations observable.

**The key architectural call: a side-table instead of a generator change.** The alternative — regenerating `X3DNode` to emit `userFields_` + a virtual `effectiveFields()` as Option A specified — was rejected because the generated layer is codegen-frozen (golden byte-identical is a hard invariant). Touching the class template would churn all 343 generated headers and produce a new golden SHA for a concern that is Script-local in practice. The side-table preserves byte-identical golden, matches existing precedent (`TransformSystem.world_`, `BoundsSystem` memo, `PickSystem` path cache), and keeps the foundation out of the generated layer entirely. `effectiveFields` is a free function precisely because the storage is a side-table — it has no `X3DNode` member to call.

## Consequences

**Positive:**

- Golden byte-identical is preserved across all 343 generated headers. No codegen run is needed to adopt or extend this foundation.
- Author-field ROUTEs resolve and deliver through the existing cascade machinery with no new event-propagation code: synthesized `FieldInfo` thunks obey the reflection contract, so `X3DEventCascade::deliver` and `X3DSceneBridge` treat them identically to spec fields.
- The mechanism is node-agnostic. `DynamicFieldStore::addAuthorField` accepts any `X3DNode`; ProtoInstance, shader, and other node types with author interfaces can reuse it in later work without further base-class changes.
- All four encodings (XML, ClassicVRML, VRML97, JSON) converge on the same `AuthorFieldDecl` seam, keeping writer round-trip straightforward: each writer iterates `effectiveFields()` and emits author field declarations as encoding-appropriate syntax.
- The `typeMismatchDrops()` counter (AUD-MEM-1) makes internal boxing-invariant violations observable without aborting.

**Trade-offs / costs:**

- Side-table lifetime is keyed by `const X3DNode*`. This is safe for the load→tick→extract model (author fields live as long as their Script node, which the document owns), but dynamic node removal would require an explicit `erase(node)` call to keep the table tidy. Dynamic removal is out of scope (post-v1, tracked under M2C-2), but the hazard is named.
- The process-global `dynamicFieldStore()` singleton means tests must call `clear()` for isolation. The singleton was chosen over threading a store reference through `X3DSceneBridge` and `SaiContext` because both are stateless/per-Script constructed with no store handle, and widening their constructors or signatures would have been a larger change for a document-scoped concern.
- `effectiveFields()` builds a temporary concatenated `FieldTable` on every call. Sites that resolve fields frequently (the cascade inner loop on `deliver`, `SaiContext::findField`) pay a small allocation per lookup. For the target load→tick→extract model with realistic Script counts this is negligible; it becomes relevant only under high-frequency dynamic field lookup, which is not a current use case.
- The original spec (Option A) envisioned `effectiveFields()` cached per instance on the node. The side-table design does not cache; the free function rebuilds on each call. A future cache layer could be added to the `NodeEntry` without changing any consumer.

## Related

- [Architecture](../architecture.md)
- [Script / SAI Runtime subsystem](../subsystems/system-script-sai.md)
- [Routes subsystem](../subsystems/routes.md)
- Implementation spec: `docs/superpowers/specs/2026-06-05-dynamic-field-foundation-spec.md`
- Un-tabling design (Phase 0–2 fan-out plan): `docs/superpowers/specs/2026-06-17-script-cdata-untabling-design.md`
- Runtime header: `runtime/events/DynamicField.hpp`
- Consuming sites: `runtime/events/X3DSceneBridge.hpp` (`detail::findField`), `runtime/script/SaiContext.hpp` (`findField`)
