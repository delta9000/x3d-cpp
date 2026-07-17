---
title: "ADR-0002: Event API for inputOnly Fields"
summary: inputOnly fields are exposed as an event API (onX/setOnXHandler callbacks), not a write-only setX mutator.
tags: [adr, event-api, inputonly, field-access, codegen]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/event-cascade.md
  - ../subsystems/generated-bindings.md
  - ../subsystems/reflection.md
---

# ADR-0002: Event API for inputOnly Fields

## Status

Accepted — 2026-06-02

## Context

ISO 19775-1 defines four field access types: `inputOutput`, `outputOnly`, `initializeOnly`, and `inputOnly`. An `inputOnly` field is a pure ROUTE sink: it receives events but has no scene-author-readable value. It is never serialized and never returned from a `get`. The only spec-defined consumer of an `inputOnly` field is the node's own behavior — for example, `IntegerTrigger`'s `set_boolean` triggers integer output when `true` arrives.

During the modernization audit (2026-06-02), the initial audit recommendation was to expose `inputOnly` fields as write-only `setX()` mutators — the same surface used for `inputOutput` fields, just without a corresponding getter. The user rejected this for three interconnected reasons.

**A plain `setX()` implies value ownership.** A write-only setter suggests the node stores the value and lets a caller inspect or replay it later. `inputOnly` fields have no spec-defined persistent value; modeling them as stored state is a category error. It also creates an asymmetry trap: future callers see `setFoo()` on the public API and reasonably expect `getFoo()` to follow, but no getter is legal here.

**Behavior wiring needs a registration seam, not a call site.** The real consumer of an `inputOnly` event is always the node's internal behavior — a sensor reacting, a trigger firing, an interpolator stepping. A node-agnostic caller (a test, a SAI binding, a System) needs to plug in that behavior without subclassing the generated node. A handler callback registered at setup time (`setOnXHandler`) models this cleanly; a raw `setX()` call invoked each tick does not.

**The event cascade must be node-agnostic.** The cascade engine (`X3DEventCascade`) delivers events to field endpoints via the reflection `FieldTable` without knowing the concrete node type. For `inputOnly` fields, the cascade's `set` thunk must dispatch to whatever behavior is registered — not write a member variable that the node never uses. The event API is the exact shape the cascade needs: `on<Name>(value)` is the dispatch point the reflection thunk calls; `setOn<Name>Handler()` is the seam through which a System or test installs behavior.

The prior state (before the event system was built) had `on<Name>` generated as an empty stub (`{ (void)value; }`), with no registrable handler and no reflection delivery path — confirmed in `docs/superpowers/specs/2026-06-03-event-system-design.md` §Current State.

## Decision

`inputOnly` fields are exposed as an event API. For each `inputOnly` field named `foo` with C++ type `T`, the generator emits exactly:

- `void onFoo(const T& value)` — the dispatch point invoked by the event cascade; a no-op if no handler is registered.
- `void setOnFooHandler(std::function<void(const T&)> handler)` — the registration seam; replaces any prior handler.
- A private `std::function<void(const T&)> _onFooHandler{}` member — default-empty.

No getter is generated. No value-storage member is generated. The field appears in the reflection `FieldTable` with a `set` thunk that calls `onFoo(value)` and a null `get` thunk (not readable). No `setFoo()` write-only mutator exists anywhere in the public API.

The audit's alternative (`setX` write-only) is rejected. See `docs/superpowers/specs/2026-06-03-event-system-design.md` for the cascade architecture that consumes this shape; see `src/x3d_cpp_gen/templates/class_template.hpp.jinja` (the `{% if field.is_event %}` branch) and `src/x3d_cpp_gen/emit/descriptors.py` (`is_event`, `handler_name`, `handler_setter_name`, `handler_member`) for the generator implementation.

A concrete example: `IntegerTrigger.set_boolean` (access type `inputOnly`) generates `onSet_boolean(const SFBool&)` and `setOnSet_booleanHandler(std::function<void(const SFBool&)>)` in `generated_cpp_bindings/x3d/nodes/IntegerTrigger.hpp`. No `setSet_boolean()` exists.

## Consequences

**Positive:**

- The public API correctly models the X3D data model: `inputOnly` fields have no accessible value, no getter, no write-only setter that implies storage.
- Behavior is pluggable without subclassing: a System, test, or SAI binding calls `setOnFooHandler(...)` once at setup and the cascade delivers automatically thereafter.
- The reflection `set` thunk is a uniform dispatch point for the cascade; `inputOutput` fields write a member, `inputOnly` fields dispatch to a handler — both go through the same `FieldTable`-driven delivery path with no per-node switch.
- The generated golden headers are stable: the `_onFooHandler` member is brace-initialized to an empty `std::function`, which is a zero-cost sentinel.
- The handler registration seam is the natural integration point for the Script/SAI runtime: when an author Script ROUTEs into an `inputOnly` field, the ECMAScript backend installs the handler (same seam, no new API surface needed).

**Trade-offs / costs:**

- Callers familiar with "write-only setter" idioms may find `setOnFooHandler` surprising on first encounter. The docstring on `on<Name>()` explains the model.
- A node's own behavior must be installed via `setOnFooHandler` before events arrive; if a System forgets to register, events silently no-op (the `if (_onFooHandler)` guard is intentional, matching X3D's "event delivery to an unregistered handler is not an error" semantics).
- The handler holds a `std::function` per `inputOnly` field per node instance. For the typical scene this is negligible; nodes with many `inputOnly` fields (e.g. NURBS nodes) carry proportionally more overhead than a simple stored-value approach would.
- The audit's `setX` alternative would have been simpler to generate but is a worse model. That trade-off is accepted.

## Related

- [Architecture](../architecture.md) — the node-agnostic codec + cascade spine this decision serves.
- [Event Cascade subsystem](../subsystems/event-cascade.md) — the engine whose `set` thunks call `on<Name>()`.
- [Generated Bindings subsystem](../subsystems/generated-bindings.md) — the golden headers this decision shapes.
- [Reflection subsystem](../subsystems/reflection.md) — the `FieldTable` / `FieldInfo` that carries the null `get` and dispatch `set` thunks for `inputOnly` fields.

Generator implementation: `src/x3d_cpp_gen/emit/descriptors.py` (`is_event`, `handler_name`, `handler_setter_name`, `handler_member`) and `src/x3d_cpp_gen/templates/class_template.hpp.jinja` (`{% if field.is_event %}` block, `{% elif field.is_event %}` in the cpp template's reflection thunk). Event cascade delivery: `src/x3d_cpp_gen/templates/class_template.cpp.jinja` lines 36–39. Concrete example: `generated_cpp_bindings/x3d/nodes/IntegerTrigger.hpp`. Original event-system design (the cascade context): `docs/superpowers/specs/2026-06-03-event-system-design.md`.
