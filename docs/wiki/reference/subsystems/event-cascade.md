---
title: Event Cascade
summary: Per-tick event propagation engine — route-loop deduplication and timestamp quantum enforcement.
tags: [subsystem, event-cascade, routes, tick, dedup]
updated: 2026-10-07
related:
  - ../../explanation/architecture.md
  - routes.md
  - execution-context.md
---

# Event Cascade

The event cascade engine propagates field events along the ROUTE graph within a
single logical timestamp. It owns breadth-first delivery, per-ROUTE guards,
scoped generated-output admission, field-alias normalization, and the quiescence
signal used by the tick re-evaluation loop.

## Purpose and supported scope

[ISO/IEC 19775-1:2023 §4.4.8.3](https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/concepts.html#ExecutionModel)
limits each output field and each ROUTE to one event per timestamp. It does
not limit an inputOnly field to one incoming occurrence. Distinct fan-in ROUTEs
and repeated external inputOnly events, including equal values, must remain
separate handler deliveries.

- `postOutputEvent` is the explicit generated-output path.
- It validates a writable outputOnly endpoint against the context's effective
  field table, then queues without changing storage.
- When drained, the first admitted output wins; subsequent values for that
  field in the same cascade are dropped before the reflection setter,
  observer, or ROUTEs see them.
- The selected value therefore agrees with readback and every routed copy.
- First-admitted is this implementation's policy, not a uniquely mandated ISO
  selection among simultaneous events.
- The single-value and multi-value template families in
  `InterpolatorSystem.hpp`, plus the Spline/Squad/Ease, NURBS and GeoPosition
  systems, use this path.
- The six outputOnly Event Utilities types also use it, covering eight fields
  and both IntegerTrigger producer paths; BooleanToggle remains excluded.
- Attach-time first-key readback remains initialization without a posted
  event.
- Producers must not call a node emitter or write its backing field before
  calling `postOutputEvent`.

`postEvent` still supports external ingress and legacy System producers. It
preserves every direct seed occurrence and retains the old cap for routed
non-inputOnly destinations. Legacy output producers are not covered by the new
output guarantee. See the [findings register](https://github.com/delta9000/x3d-cpp/blob/main/docs/conformance/findings.yaml) for conformance status.

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

  // External input or legacy seed: preserve all accepted occurrences.
  void postEvent(X3DNode *node, const std::string &field, std::any value);

  // Generated outputOnly: enqueue without mutation; admit once before delivery.
  // Throws invalid_argument for unknown/unwritable/non-outputOnly endpoints.
  void postOutputEvent(X3DNode *node, const std::string &field, std::any value);

  // Open a new timestamp: clears the per-route fired_ and per-field produced_ sets.
  void beginTimestamp();

  // Drain pending events to quiescence.
  // freshTimestamp=true (default): begins a timestamp for an outermost drain.
  // Nested drains always retain the active timestamp, including on exceptions.
  // freshTimestamp=false: continues the current timestamp (tick re-eval loop).
  // Returns count of first-time field productions this call; 0 signals quiescence.
  std::size_t process(bool freshTimestamp = true);

  // Register a callback invoked after each successful field delivery.
  // X3DExecutionContext wires this to classifyDirty() for dirty-tracking.
  void setFieldObserver(std::function<void(const FieldAddress &)>);
};
```

### Seam points

- Field delivery via reflection — `EventCascade::deliver` (private) walks
  `node->fields()` and calls `info.set(*node, value)` for the matching entry.
  Any node with a reflection table (`X3DNode::fields()`) is automatically
  deliverable; no cascade-specific registration is needed. When the static table
  has no match, `deliver` falls back to the node's author fields in
  its explicitly supplied `DynamicFieldStore` (`authorFields()`) and writes through their synthesized
  `set` thunk. This is what lets a ROUTE whose sink is a `<field>` on a Script /
  ComposedShader actually deliver: `buildRoutes` resolves such sinks via
  `effectiveFields()` (static ∪ author), so without this fallback the edge
  validated but evaporated at delivery (see finding `SCRIPT-EVENTIN`).

- Field observer / dirty-tracking feed — `setFieldObserver` installs a
  single callback invoked after each successful delivery. `X3DExecutionContext`
  wires this to `classifyDirty(addr)` so the dirty-tracker is updated in
  lock-step with the cascade. Only one observer slot exists; it is reserved for
  dirty-tracking. The `x3d sim` field tracer uses snapshot-diff instead of this
  slot (see [ADR-0009: sim snapshot-diff](../../explanation/decisions/0009-sim-snapshot-diff.md)).

- Input filter — `X3DTimeDependentSystem` registers a shared filter for its
  attached nodes. The cascade checks it before recording a field production,
  writing the field, or forwarding ROUTEs. An active time-dependent node ignores
  `startTime` and `stopTime <= startTime` inputs (§8.2.4.3); direct runtime field
  writes use the same filter. The ordered stop-then-start restart is allowed.

- Timestamp lifecycle owned by execution context — `X3DExecutionContext::tick`
  calls `cascade_.beginTimestamp()` once, then loops
  `cascade_.process(false)` (continuing the same timestamp) after each System
  pass until the return value reaches zero. This implements ISO 19775-1 §4.4.8.3
  step 4: re-evaluate sensors + drain repeatedly within one tick. The reached-field
  bookkeeping (`produced_`) and generated-output cap persist across these drain
  calls. First-time reachability, rather than the raw number of input occurrences,
  determines whether another System pass is required.

- Dynamic route mutation during a cascade — `EventCascade::process` snapshots
  the sink list (copy, not reference) before invoking `deliver`, so a handler
  that calls `ctx.addRoute` / `ctx.removeRoute` mid-cascade (SAI §4.3.7) does
  not invalidate iterators. Routes added mid-cascade take effect from the next
  cascade.

- Script eventsProcessed hook — `X3DExecutionContext::addPostCascadeHook`
  installs a callback run _after_ the cascade drains each tick. `ScriptSystem`
  uses this for the §29.2.4 `eventsProcessed()` phase; that hook may post and
  drain further events. A `process()` call inside `tick()` continues that tick's
  guards, including this post-cascade phase. Nested `process()` calls outside a
  tick also preserve the enclosing cascade; an exception-safe depth guard allows
  only a subsequent outermost fresh call to start another cascade.

## Limitations

- Each standalone `process()` and each `tick(now)` opens a fresh logical
  timestamp; repeated numeric `now` values are not reconciled into one ISO
  timestamp, so outputs and ROUTEs can fire again at the same numeric time.
- `X3DTimeDependentSystem::emit`, used by `TimeSensorSystem`, posts plain
  events rather than `postOutputEvent`. Activation followed by completion in one
  update may queue both `isActive=true` and `isActive=false`.

## Conformance

See the [findings register](https://github.com/delta9000/x3d-cpp/blob/main/docs/conformance/findings.yaml) for conformance status.

## Related

- [Architecture](../../explanation/architecture.md)
- [Routes](routes.md)
- [Execution Context](execution-context.md)
- [ADR-0009: sim snapshot-diff](../../explanation/decisions/0009-sim-snapshot-diff.md)
