---
title: "ADR-0004: Virtual Mixins for AbstractObjectTypes"
summary: AbstractObjectType mixins inherit public virtual and retain their real fields, enabling correct diamond-inheritance in the generated bindings.
tags: [adr, virtual-mixins, abstract, multiple-inheritance, bindings]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/generated-bindings.md
---

# ADR-0004: Virtual Mixins for AbstractObjectTypes

## Status

Accepted — decided 2026-06-02, implemented in Phase 4 (commit `ca69e35`)

## Context

The X3D 4.0 UOM defines two tiers of abstract type:

- **AbstractNodeTypes** (`X3DGroupingNode`, `X3DChildNode`, …): normal spec inheritance; every node has exactly one primary base via `<Inheritance baseType="…">`.
- **AbstractObjectTypes** (`X3DBoundedObject`, `X3DUrlObject`, `X3DMetadataObject`, `X3DPickableObject`, `X3DProgrammableShaderObject`, `X3DFogObject`): cross-cutting interfaces mixed into concrete and abstract nodes via `<AdditionalInheritance baseType="…">`. They carry *real, spec-defined fields* — `X3DBoundedObject` owns `bboxCenter`/`bboxSize`/`bboxDisplay`/`visible`; `X3DUrlObject` owns `url`/`load`/`autoRefresh`/`autoRefreshTimeLimit`/`description`.

Two implementation questions had no single obvious answer:

**1. Should the fields live on the mixin or be copy-pasted into every concrete node?**

Copy-pasting would avoid multiple inheritance entirely — the simplest approach. But the UOM has 51 nodes that carry `AdditionalInheritance`, many sharing the same mixin. Duplicating fields into every node would violate DRY, diverge from the spec's declared structure, and double-count field descriptors in the reflection table (breaking accessors and codecs). The spec's AbstractObjectType is a shared unit of field ownership; the binding should reflect that exactly.

**2. What inheritance specifier?**

If all bases use `public` (non-virtual), any node that reaches a shared ancestor through more than one path gets multiple copies of that ancestor — the classic C++ diamond problem. Phase 4 exposed this concretely: `LayoutGroup` lists primary base `X3DGroupingNode` (which inherits `X3DChildNode` and `X3DBoundedObject`) and also lists `X3DNode` explicitly as an `AdditionalInheritance`. Without virtual, `X3DNode` appears twice in `LayoutGroup`'s object, producing `-Winaccessible-base` and ambiguous accessor calls. Compiler error, not a warning.

An alternative considered during the Phase 1 audit was to expose `inputOnly` AbstractObjectType fields as write-only `setX` stubs (the auditor's suggestion). The user overrode this: real fields with getters AND setters. That choice reinforced the need for a single, shared subobject — the field members must exist exactly once per object.

The spec also raised a subtlety around the reflection API. `X3DNode` is the single reflection root: it declares the pure virtual `fields()`, `nodeTypeName()`, and related reflection methods that every generated node overrides. AbstractObjectTypes are *rootless* — they do not reach `X3DNode` through their own base chain. If they were allowed to inherit the reflection declarations, a concrete node mixing one in alongside `X3DNode` would see two unrelated bases declaring the same virtual, causing ambiguous override errors.

## Decision

We decided that AbstractObjectType mixins are generated with `public virtual` inheritance everywhere — both at their own declaration and at every node that lists them under `AdditionalInheritance` — and retain their real, spec-defined fields with full public accessors.

Concretely:

- `X3DBoundedObject`, `X3DUrlObject`, `X3DMetadataObject`, `X3DPickableObject`, `X3DProgrammableShaderObject`, and `X3DFogObject` are standalone classes (no primary base, `base_type=None`) parsed from `<AbstractObjectTypes>` in the UOM.
- The Jinja template (`src/x3d_cpp_gen/templates/class_template.hpp.jinja`) emits every base — primary and additional — as `public virtual <Base>`. The `virtual_bases` list passed to the template is `[node.base_type] + list(node.additional_base_types)`.
- The `_reaches_root` check in `src/x3d_cpp_gen/backends/cpp_header.py` determines whether a class should declare/override the reflection virtuals. AbstractObjectTypes do not reach `X3DNode`, so `emit_reflection=False` for them: they are plain interface classes with no reflection virtuals. Concrete nodes and AbstractNodeTypes, which all transitively reach `X3DNode`, emit the reflection override, giving exactly one declarer per concrete object.
- Field disambigation in the reflection table: inherited fields are qualified by the earliest ancestor that declares them (identified by `_ancestors()` + `own_field_names`), so the reflection thunk calls the right subobject's accessor even under a diamond — e.g. `static_cast<X3DBoundedObject*>(this)->getBboxCenter()`.

Example: `Anchor : public virtual X3DGroupingNode, public virtual X3DUrlObject` inherits `bboxCenter` through `X3DGroupingNode → X3DBoundedObject` and `url` through `X3DUrlObject`, with both mixin subobjects shared and unambiguous. `LayoutGroup : public virtual X3DNode, public virtual X3DGroupingNode` collapses the two paths to `X3DNode` into a single subobject.

## Consequences

**Positive:**

- Diamond inheritance is resolved correctly at compile time with no ambiguous-accessor errors and no `-Winaccessible-base` warnings under `-Wall`. All 340/342 headers compiled clean after Phase 4.
- Each mixin's fields exist exactly once per concrete object, matching the spec's ownership model. `anchor.getUrl()` and `anchor.getBboxCenter()` call through the same base subobject regardless of how many paths reach that mixin.
- The reflection layer can safely qualify inherited accessor calls (`static_cast<X3DBoundedObject*>(this)->getBboxCenter()`), making codec round-trips correct for all 51 multi-inheritance nodes without special-casing.
- AbstractObjectTypes being `emit_reflection=False` keeps the virtual reflection API single-declared on `X3DNode`. No ambiguous pure-virtual error can arise, regardless of how a mixin is composed into a class.
- The mixin structure directly mirrors the spec's AbstractObjectType taxonomy, so future UOM additions (e.g. if X3D 4.1 adds a new cross-cutting interface) are handled automatically by the parser and template without code changes.

**Trade-offs / costs:**

- C++ virtual inheritance has a small runtime cost (each virtual base has a vptr slot and uses pointer adjustment on static_cast). For a retained-mode scene graph where node construction is infrequent and field access is not on the per-frame hot path, this cost is negligible — but it is nonzero.
- The `_reaches_root` graph walk and `_ancestors` resolver add a modest layer of complexity to the backend. The `inherited_from` qualifier logic (choosing which ancestor to qualify a reflected field by) is not trivial to read or audit. Comments in `cpp_header.py` lines 46–65 and 119–141 document the invariant.
- Phantom fields (9 fields in the 4.0 UOM that the UOM marks `inheritedFrom` a class that never declares them) must be detected and dropped from the reflection table — a special case exposed by the disambiguition walk. Documented in `src/x3d_cpp_gen/backends/cpp_header.py` lines 131–140.

## Related

- [Architecture](../architecture.md)
- [Generated Bindings](../subsystems/generated-bindings.md)
- Decisions memory: `~/.claude/projects/.../memory/x3d-cpp-gen-decisions.md` — the original binding decision ("AbstractObjectType mixins → inherit `public virtual`, keep their real fields")
- Phase 4 commit `ca69e35` — "Fix the LayoutGroup -Winaccessible-base diamond: emit all bases as `public virtual`"
- `src/x3d_cpp_gen/backends/cpp_header.py` — `_reaches_root`, `_ancestors`, `emit_reflection` logic (lines 46–65, 119–165)
- `src/x3d_cpp_gen/parser.py` — `parse_node(is_mixin=True)` for `<AbstractObjectTypes>`, `build_dependency_graph` (lines 121–146, 208–223)
- `src/x3d_cpp_gen/templates/class_template.hpp.jinja` line 24 — the `public virtual` emission loop
- `generated_cpp_bindings/X3DBoundedObject.hpp` — example mixin with real spec fields (`bboxCenter`, `bboxSize`, `visible`)
- `generated_cpp_bindings/X3DUrlObject.hpp` — example mixin with constrained fields (`autoRefresh`, `autoRefreshTimeLimit`)
- `generated_cpp_bindings/LayoutGroup.hpp` — the canonical diamond case (`X3DNode` reached via two paths)
- `generated_cpp_bindings/Anchor.hpp` — concrete node inheriting both `X3DGroupingNode` and `X3DUrlObject`
