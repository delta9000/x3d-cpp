---
title: "ADR-0011: ECS-Flavored Architecture, Not Full ECS"
summary: The OOP node model and reflection remain the source of truth; ECS-flavored System abstractions are adopted only for behavior dispatch, not as a full entity-component system.
tags: [adr, ecs, architecture, oop, system-abstraction]
updated: 2026-06-20
related:
  - ../architecture.md
---

# ADR-0011: ECS-Flavored Architecture, Not Full ECS

## Status

Accepted — 2026-06-03 (original decision); reaffirmed 2026-06-07 by the architecture deep-research validation.

## Context

When the event/behavior runtime was being designed (2026-06-03), the core question was whether to adopt a full Entity-Component System (ECS) — replacing or wrapping X3D nodes with entities and components — or to keep the OOP node model and reflection layer as the authoritative representation.

Three forces created the tension:

**The spec mandates node identity.** ISO 19775-1 defines X3D as a retained-mode scene-graph where DEF-named nodes are shared by reference (`USE`), and events route between named fields via `ROUTE`. Node identity and typed field access are first-class; the spec's execution model is defined in terms of cascading events between node fields at a shared timestamp. A full ECS replaces this per-instance identity with entity ids and sparse component tables — a fundamentally different data model.

**The generator's output is the moat.** The generator emits spec-correct, golden-locked C++ node classes with typed fields, getters/setters, and reflection thunks. A full ECS would either duplicate this layer (maintaining a shadow mapping from entities to typed node data) or discard it (losing the spec-correct codegen moat that the project's conformance claim depends on). Neither outcome is acceptable.

**ECS batching is attractive for hot paths.** For behavior dispatch over large numbers of nodes of the same type (interpolators, TimeSensors), iterating a packed array is more cache-friendly than chasing `shared_ptr` chains through a heterogeneous scene graph. The performance intuition behind ECS is sound; the question is scope.

A deep-research validation pass (25 claims verified, 0 refuted — see `docs/superpowers/specs/2026-06-07-architecture-validation-and-resequencing.md`) confirmed that no major X3D runtime uses ECS: X3DOM, Xj3D, FreeWRL, and Castle Game Engine all retain the OOP node model as the authoritative representation. Castle in particular is the closest prior art — a retained-mode X3D-node model manipulated in place, production-proven. The same validation confirmed that USD/Hydra's architecture (scene delegate → render index → render delegate) follows the same node-as-truth + systems-update-in-place + renderer-as-consumer pattern, further de-risking the node-model spine.

## Decision

We decided to keep the OOP node model and reflection layer as the single source of truth, and to adopt ECS-flavored patterns **only** for behavior dispatch — not as a full entity-component system that replaces or wraps nodes.

Concretely:

- The `System` abstraction (`runtime/events/X3DSystem.hpp`) is an ECS-flavored behavior family: it owns a collection of nodes of one type and either iterates them in `update(now, ctx)` (time-driven) or wires an inputOnly handler in `attach(node, ctx)` and leaves `update` a no-op (event-driven). All state is held in side tables keyed by `const X3DNode*` or dense ids — never written back into node fields by the system itself.
- `System` generalizes the earlier per-node `ActiveNode` protocol (`runtime/events/X3DActiveNode.hpp`); a System owning its enrolled nodes contiguously is the explicit seam for future data-oriented batching within `update()` without touching the node model.
- A real ECS projection — moving node data into flat component arrays — is deferred until profiling a large dynamic scene demonstrates that OOP traversal is the actual bottleneck. It is a possible runtime-side optimization, not a structural commitment.

## Consequences

**Positive:**

- The spec-correct generated bindings, golden gate, reflection layer, and conformance claim are fully preserved. Systems are consumers of node data, not an alternative data model layered over it.
- The `System` abstraction already enables cache-friendly batching for homogeneous node families (e.g. iterating 1,000 Interpolator nodes in a tight `update()` loop), capturing the ECS performance benefit at hot paths without the structural cost.
- No DEF/USE sharing or ROUTE wiring is disrupted: nodes remain `shared_ptr`-identified, ROUTEs address fields by name, and the event cascade operates on the node model directly.
- Aligns with every shipping X3D runtime (Castle, X3DOM, Xj3D, FreeWRL) and with USD/Hydra's validated scene-delegate-as-truth pattern.
- A future ECS projection is not foreclosed — it remains available as a per-System optimization under the existing `System` seam, without requiring an architectural change.

**Trade-offs / costs:**

- Per-System update loops still resolve behavior state through `X3DNode` typed fields (the spec-typed getters), not a flat SOA buffer. For the current corpus size and tick rates this is not a bottleneck; at very high node counts a future system could hold a shadow cache, but that complexity is deferred.
- The `ActiveNode` protocol is now a legacy seam (predates `System`); both coexist. The `System` pattern is the current idiom; `ActiveNode` is documented as the predecessor and exists for backward compatibility with any embedder code that registered raw `ActiveNode` objects before the `System` abstraction landed.

## Related

- [Architecture](../architecture.md) — the full system map; §4 describes the behavior System set and both System styles (time-driven and event-driven).
- `runtime/events/X3DSystem.hpp` — the `System` base class; the comment "A System owning its nodes contiguously is the seam for future data-oriented batching" is the ECS-flavored seam.
- `runtime/events/X3DActiveNode.hpp` — the predecessor per-node `ActiveNode` protocol that `System` generalizes.
- `docs/superpowers/specs/2026-06-07-architecture-validation-and-resequencing.md` — the deep-research validation that confirmed "ECS-flavored batching only at hot paths, NOT a full ECS" (§1, finding 3) and the Castle/USD/Hydra prior-art sweep.
- `docs/superpowers/specs/2026-06-03-event-system-design.md` — the original event-system design where the node-as-truth + System decision was first made.
