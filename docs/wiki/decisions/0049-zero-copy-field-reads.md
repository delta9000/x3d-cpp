---
title: "ADR-0049: Zero-Copy Field Reads"
summary: Every generated getter returns const T& to its member, and FieldInfo gains a plain function-pointer view thunk that hands out the member's address plus its std::type_info. runtime/FieldRead.hpp wraps it as fieldPtr<T>, FieldRef<T> (boxed get() fallback for author fields) and forEachChildNode. Reflection reads and the runtime's scene walks borrow values instead of boxing and copying them. A full walk of an 82,001-node scene drops from about 11.7 ms to 7.0 ms.
tags: [adr, reflection, codegen, performance, api]
updated: 2026-09-26
related:
  - ../subsystems/reflection.md
  - ../subsystems/generated-bindings.md
  - 0014-dynamic-field-foundation.md
  - 0039-generated-binding-namespaces.md
---

# ADR-0049: Zero-Copy Field Reads

## Status

Accepted

## Context

Generic code reads fields through `FieldInfo::get`, a `std::function` that
returns the value boxed in a `std::any`. For an MF field that is a full copy
of the vector. For an `MFNode` it also bumps every child's refcount. The
runtime walks the scene graph generically, and each of those walks read every
node's `children` this way: transform indexing, bounds, bindables, picking,
pointing sensors, lights, extraction, Inline expansion and PROTO cloning.
Several of them run every tick.

The typed API had the same problem. Only `MFNode` getters returned a
reference (added earlier to stop child-vector copies). Every other getter,
including `MFVec3f getPoint()` and `SFString` getters, returned by value, so
there was no way to read a large field without copying it, typed or not.

## Decision

- **Getters return `const T&`.** Every generated getter returns a reference to
  its member. A reference to a member stays valid while the node lives.
  Setters are plain assignments, so `n.setX(n.getX())` is a safe
  self-assignment.
- **`FieldInfo::view`.** Each readable generated field gets a thunk
  `FieldView (*)(const X3DNode&)` that returns
  `{&node.getX(), &typeid(T)}`. It is a plain function pointer, so there is no
  `std::function` dispatch. It is null for `inputOnly` fields and for fields
  with no stored member: synthesized author fields (ADR-0014) and the
  hand-written `ExternalGeometry` table. It is the last `FieldInfo` member and
  defaults to null, so existing aggregate initializers still compile.
- **Runtime helpers** (`runtime/FieldRead.hpp`):
  - `fieldPtr<T>(node, field)` returns a borrowed `const T*`, or null if there
    is no view or the type does not match. A mismatch asserts in debug builds.
  - `FieldRef<T>` borrows when it can and otherwise boxes through `get`. It is
    neither copyable nor movable, because its fallback pointer aims into its
    own box.
  - `forEachChildNode(node, fn)` calls `fn(field, child)` for each non-null
    SFNode/MFNode child and never copies a vector or a `shared_ptr`.
- The duplicated SFNode/MFNode walks in the runtime now use
  `forEachChildNode` (TransformSystem, BoundsSystem, BindingSystem,
  PickSystem, PointingSensorSystem, LightSystem, SceneExtractor ×3,
  X3DSceneBridge, InlineExpand). `SceneExtractor::childrenOf` returns a
  reference, and PROTO cloning borrows the source's child lists.

The `std::any` `get`/`set` API is unchanged. Codecs and anything that needs an
owned value keep using it.

## Consequences

- A full generic walk of an 82,001-node scene (2,000 Transforms × 20 Shapes,
  each with a Box) went from about 11.7 ms to 7.0 ms (Release, median of five
  runs, same machine). What remains is the per-node field-table scan and the
  `dynamic_cast` inside each thunk.
- **Aliasing.** `const auto& x = n.getX();` used to extend the lifetime of a
  copy, and now it refers to the member. It sees later writes and must not
  outlive the node. A scan found no call site that bound a getter by reference
  and then wrote the same field. `auto x = n.getX();` still copies.
- `decltype(n.getX())` is now a reference type. One compile-time pin
  (`uom_type_pin_test.cpp`) now applies `remove_cvref_t`.
- Code that mutated a getter's temporary (a silent no-op before) no longer
  compiles.
- A borrowed pointer is only safe while nothing writes that field. The walks
  that switched to it are read-only, and `forEachChildNode` documents that
  `fn` must not write the field it was called for. Inline expansion already
  collected its sites before rewriting parents.
- The invariant "every readable generated field has a view whose type is the
  type `get` boxes" is checked for every factory node type
  (`runtime/parse/tests/field_view_test.cpp`).

## Related

- [Reflection Layer](../subsystems/reflection.md)
- [ADR-0014: Dynamic Field Foundation](0014-dynamic-field-foundation.md): author
  fields, which have no view and fall back to `get`
