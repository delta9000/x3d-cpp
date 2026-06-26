---
title: "ADR-0009: sim Uses Snapshot-Diff Field Tracing"
summary: The x3d sim field tracer operates by snapshot-diff rather than a cascade observer, preserving the single observer slot for dirty-tracking.
tags: [adr, sim, snapshot-diff, field-tracer, cascade-observer]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/cli-suite.md
  - ../subsystems/event-cascade.md
---

# ADR-0009: sim Uses Snapshot-Diff Field Tracing

## Status

Accepted — 2026-06-20

## Context

`x3d sim` (the headless behavior simulation command) needs to report, per tick, which node fields changed value. Two mechanisms exist in principle:

**Option A — cascade observer.** `X3DEventCascade` exposes exactly one per-field delivery observer slot: `setFieldObserver(std::function<void(const FieldAddress&)>)`. When the cascade delivers an event to a field it calls `observer_` with the field's address. A tracer could register itself here and record every write the cascade makes.

**Option B — snapshot-diff.** Before and after each tick, walk every node's readable non-node fields (via the `X3DNode::fields()` reflection thunk), format their values, and diff the two snapshots to produce `(node, field, newValue)` deltas.

The cascade observer slot is not free. `X3DExecutionContext::buildSceneGraph()` binds it unconditionally:

```cpp
cascade_.setFieldObserver(
    [this](const FieldAddress &a) { classifyDirty(a); });
```

This binding routes every cascade delivery into `classifyDirty`, which feeds the `DirtyTracker` — the mechanism that drives incremental world-transform and bounding-volume propagation (`TransformSystem`, `BoundsSystem`). The dirty-tracking path is the execution context's core correctness invariant: every cascade write must reach `classifyDirty` or the transform/bounds state goes silently stale (M2C-3 was a documented bug of exactly this kind).

`setFieldObserver` takes a single `std::function` — there is no chain, no multi-observer list. Replacing the binding would evict `classifyDirty`, breaking all dirty-driven propagation. Chaining (wrapping the new callback to also call the old) would require the tracer to hold an interior reference to the execution context's private dirty-tracking path, coupling `sim_tracer.hpp` to internals not exposed by `X3DExecutionContext`'s public interface. The `X3DExecutionContext` API does not expose a way to "also call this observer" without replacing the installed one.

Sensor and interpolator output fields (`isActive`, `enterTime`, `value_changed`, `fraction_changed`, etc.) are `inputOutput` or `outputOnly` fields whose in-memory stored values the cascade updates when it delivers. Since the reflection `get` thunk reads the stored value, a post-tick snapshot naturally captures the cascade's writes — no special-casing is needed for the "interesting" outputs that CI cares about.

## Decision

We decided that the `x3d sim` field tracer uses **snapshot-diff** (Option B), not a cascade observer. After each `ctx.tick()`, `x3d::sim::FieldTracer::diff()` snapshots every readable non-node field value on every scene node via reflection (`fields()` + `get` thunks), diffs the snapshot against the previous tick's snapshot, and emits `FieldChange{node, field, value}` deltas. The cascade observer slot is left undisturbed, bound exclusively to `classifyDirty` as installed by `buildSceneGraph`.

The tracer implementation lives in `tools/x3d-cli/sim_tracer.hpp`, namespace `x3d::sim`. Node identity for the trace is deterministic: DEF-named nodes use their DEF name; anonymous nodes get a stable `<Type>#<index>` synthesized from a document-order DFS over `scene.rootNodes`, with per-type counters, so golden-file output is reproducible across runs.

## Consequences

**Positive:**

- The tracer is **non-invasive**: it touches no core state, holds no reference to `X3DExecutionContext` internals, and requires no modification to `X3DEventCascade` or `X3DExecutionContext`. It is implemented entirely in a self-contained header (`sim_tracer.hpp`) with no new core dependencies.
- The dirty-tracking loop (`classifyDirty` → `DirtyTracker` → `TransformSystem`/`BoundsSystem`) is **preserved exactly**. There is no risk of the tracer accidentally evicting the dirty feed.
- **Sensor and interpolator outputs surface naturally.** `isActive`, `enterTime`, `value_changed`, `fraction_changed`, and all other `outputOnly`/`inputOutput` stored values are written by the cascade and readable via reflection, so the snapshot captures them without any special-casing.
- The mechanism is **deterministic by construction**: node iteration order is the DFS walk order (fixed per scene), field iteration order is `fields()` order (fixed per type), and `formatValue` is deterministic. Same scene + same drivers → byte-identical `--json` trace across runs. This is directly testable (the self-oracle runs sim twice and asserts equal output).
- The trace is **broad by default** (every changed non-node field per tick) and narrowable to `--watch DEF.field` specs with zero overhead on unmonitored fields.
- Adding future observable behavior (e.g. a physics-backed `RigidBodySystem`) costs nothing: once the system writes node fields through the execution context's `writeField` or `postEvent` paths, the snapshot picks up those changes automatically.

**Trade-offs / costs:**

- **Per-tick overhead scales with scene size.** Each tick, `diff()` formats the string representation of every readable non-node field on every node, then compares strings. For large scenes this is measurable CPU cost. `sim` is a headless CI/debug harness, not a production render loop, so this is acceptable; a future high-frequency physics harness might need a different approach.
- **Only committed field values are visible.** If a system updates an internal C++ member that is not exposed through the `fields()` reflection thunks, the snapshot will not capture it. The convention for all spec-defined outputs is that they are reflected, but an incorrectly instrumented custom system could produce invisible side effects. This is a correctness requirement for the reflection layer, not a new one, but `sim` makes it more visible.
- **The cascade observer approach is not reachable without an API extension.** If a future subsystem needs per-delivery, sub-tick observability (not just end-of-tick diffs), the observer slot must be upgraded to a multi-observer list in `X3DEventCascade`. The snapshot-diff design does not block this, but it does not solve it either.

## Related

- [Architecture](../architecture.md)
- [CLI Suite](../subsystems/cli-suite.md) — `x3d sim` is one of the CLI suite subcommands; golden trace registration + `mise run cli-gate` regression machinery
- [Event Cascade](../subsystems/event-cascade.md) — the cascade's single observer slot is the constraint this ADR works around; `setFieldObserver` signature and `observer_` in `runtime/events/X3DEventCascade.hpp`
- Design spec: `docs/superpowers/specs/2026-06-20-x3d-sim-design.md` — the authoritative `sim` design; Unit 3 "Field-change tracer" names the snapshot-diff fallback and its rationale explicitly
- `tools/x3d-cli/sim_tracer.hpp` — the tracer implementation; the file-top comment block restates this ADR's reasoning as load-bearing inline documentation
- `tools/x3d-cli/sim_runtime.hpp` — `attachFullRuntime()` + the physics-attach hook; the "wire full runtime" helper that `sim` calls before the tick loop
- `runtime/events/X3DExecutionContext.hpp` — `buildSceneGraph()` lines binding `cascade_.setFieldObserver` to `classifyDirty`; `writeField` and `classifyDirty` documentation
- `runtime/events/X3DEventCascade.hpp` — `setFieldObserver(std::function<void(const FieldAddress&)>)` declaration; the single `observer_` slot
- `runtime/scene/DirtyTracker.hpp`, `TransformSystem.hpp`, `BoundsSystem.hpp` — the dirty-tracking machinery that depends on the observer staying bound
