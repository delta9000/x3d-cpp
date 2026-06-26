---
title: Event Cascade
summary: Per-tick event propagation engine — route-loop deduplication and timestamp quantum enforcement.
tags: [subsystem, event-cascade, routes, tick, dedup]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/routes.md
  - ../subsystems/execution-context.md
---

# Event Cascade

The event cascade engine propagates field events along the ROUTE graph within a
single X3D timestamp. It owns the breadth-first delivery loop, two independent
loop-breaking guards mandated by ISO/IEC 19775-1, field-alias normalization, and
the quiescence signal used by the tick re-evaluation loop.

## Purpose

X3D defines a _single-timestamp_ event model: every ROUTE that is reachable
from an initial set of field postings fires within the same logical instant, and
no field is produced more than once per timestamp. The event cascade subsystem
realises this contract. It sits between the behavior Systems (time sensors,
interpolators, script, pointing-device sensors) that _post_ events and the node
reflection layer that _receives_ them, ensuring delivery is breadth-first,
deduplicated, and bounded regardless of cycles or fan-in in the ROUTE graph.

The subsystem has no knowledge of scene structure, node types, or what the
fields mean; it operates entirely on `FieldAddress` endpoints and `std::any`
values routed through the `EventGraph`.

## Key files

| File | Role |
|---|---|
| `runtime/events/X3DEventCascade.hpp` | `EventCascade` — the cascade engine: breadth-first delivery loop, per-route and per-field guards, field-observer callback |
| `runtime/events/X3DEventGraph.hpp` | `EventGraph` — the ROUTE table (source `FieldAddress` → ordered sink list); `resolveFieldAlias` free function |
| `runtime/events/X3DFieldAddress.hpp` | `FieldAddress` — `(X3DNode*, std::string field)` pair with equality and `std::hash` |
| `runtime/events/X3DExecutionContext.hpp` | `X3DExecutionContext` — owns the graph + cascade, drives `tick()`, wires the field observer to dirty-tracking |

## Interfaces and seams

### Exposed interface

```cpp
// X3DFieldAddress.hpp — event endpoint
struct FieldAddress {
  X3DNode *node = nullptr;    // raw observer pointer; scene graph owns lifetime
  std::string field;           // canonical X3D field name (alias-normalized)
  bool operator==(const FieldAddress &) const;
};

// X3DEventGraph.hpp — ROUTE table
class EventGraph {
  void addRoute(const FieldAddress &from, const FieldAddress &to);
  void removeRoute(const FieldAddress &from, const FieldAddress &to);
  const std::vector<FieldAddress> &sinks(const FieldAddress &from) const;
  std::size_t routeCount() const;
  void clear();
};

// Also in X3DEventGraph.hpp — field-alias resolution
std::string resolveFieldAlias(const X3DNode *node, const std::string &name);
// Resolves set_xxx / xxx_changed aliases to the base inputOutput field name.

// X3DEventCascade.hpp — cascade engine
class EventCascade {
  explicit EventCascade(const EventGraph &graph);

  // Seed an event (direct post, always delivers, last-wins within a timestamp).
  void postEvent(X3DNode *node, const std::string &field, std::any value);

  // Open a new timestamp: clears the per-route fired_ and per-field produced_ sets.
  void beginTimestamp();

  // Drain pending events to quiescence.
  // freshTimestamp=true (default): calls beginTimestamp() first (standalone one-shot).
  // freshTimestamp=false: continues the current timestamp (tick re-eval loop).
  // Returns count of first-time field productions this call; 0 signals quiescence.
  std::size_t process(bool freshTimestamp = true);

  // Register a callback invoked after each successful field delivery.
  // X3DExecutionContext wires this to classifyDirty() for dirty-tracking.
  void setFieldObserver(std::function<void(const FieldAddress &)>);
};
```

### Seam points

- **Field delivery via reflection** — `EventCascade::deliver` (private) walks
  `node->fields()` and calls `info.set(*node, value)` for the matching entry.
  Any node with a reflection table (`X3DNode::fields()`) is automatically
  deliverable; no cascade-specific registration is needed.

- **Field observer / dirty-tracking feed** — `setFieldObserver` installs a
  single callback invoked after each successful delivery. `X3DExecutionContext`
  wires this to `classifyDirty(addr)` so the dirty-tracker is updated in
  lock-step with the cascade. Only one observer slot exists; it is reserved for
  dirty-tracking. The `x3d sim` field tracer uses snapshot-diff instead of this
  slot (see [ADR-0009: sim snapshot-diff](../decisions/0009-sim-snapshot-diff.md)).

- **Timestamp lifecycle owned by execution context** — `X3DExecutionContext::tick`
  calls `cascade_.beginTimestamp()` once, then loops
  `cascade_.process(false)` (continuing the same timestamp) after each System
  pass until the return value reaches zero. This implements ISO 19775-1 §4.4.8.3
  step 4: re-evaluate sensors + drain repeatedly within one tick. The per-field
  cap (`produced_`) persists across these drain calls, bounding the loop.

- **Dynamic route mutation during a cascade** — `EventCascade::process` snapshots
  the sink list (copy, not reference) before invoking `deliver`, so a handler
  that calls `ctx.addRoute` / `ctx.removeRoute` mid-cascade (SAI §4.3.7) does
  not invalidate iterators. Routes added mid-cascade take effect from the next
  cascade.

- **Script eventsProcessed hook** — `X3DExecutionContext::addPostCascadeHook`
  installs a callback run _after_ the cascade drains each tick. `ScriptSystem`
  uses this for the §29.2.4 `eventsProcessed()` phase; that hook may post and
  drain further events (calling `process()` internally on a fresh timestamp).

## How it is tested

- `ctest --preset dev -R x3d_event_cascade` — route propagation, fan-out,
  per-route loop-breaking, inputOnly delivery (`runtime/events/tests/cascade_test.cpp`).

- `ctest --preset dev -R x3d_cascade_conformance` — RTC-5 (fan-in delivers
  once per timestamp, cyclic re-drive broken by per-field cap) and RTC-6
  (tick re-evaluation loop terminates and resolves within one tick)
  (`runtime/events/tests/cascade_conformance_test.cpp`).

- `ctest --preset dev -R x3d_cascade_alias_audit` — field-alias normalization:
  `set_xxx` and `xxx_changed` aliases share the same per-field identity with the
  canonical `xxx` name, both in the ROUTE table and in the cascade's produced
  guard (`runtime/events/tests/cascade_alias_audit_test.cpp`).

- `ctest --preset dev -R x3d_cascade_observer` — `setFieldObserver` fires for
  every delivered field (seed and routed), verifying the dirty-tracking feed
  (`runtime/events/tests/cascade_observer_test.cpp`).

- `ctest --preset dev -R x3d_cascade_dynamic_route` — route added/removed
  during an active cascade takes effect on the next cascade (mid-cascade mutation
  safety) (`runtime/events/tests/cascade_dynamic_route_test.cpp`).

## Related specs and ADRs

- [Architecture](../architecture.md)
- [Routes](../subsystems/routes.md)
- [Execution Context](../subsystems/execution-context.md)
- Spec reference: ISO/IEC 19775-1:2023 §4.4.8.3 (event model and single-timestamp
  semantics), §4.4.2.2 (inputOutput field aliases), §4.3.7 (SAI addRoute/deleteRoute)
- ADR: [ADR-0009: sim snapshot-diff](../decisions/0009-sim-snapshot-diff.md) — why the single observer
  slot is reserved for dirty-tracking and the `x3d sim` tracer uses snapshot-diff
- `docs/superpowers/BACKLOG.md` (deprecated, historical) rows RTC-5 and RTC-6 — the conformance findings
  that drove the per-field cap and the re-evaluation loop
