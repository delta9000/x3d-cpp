# X3D Event System — Design & Browser Roadmap

**Date:** 2026-06-03
**Status:** Approved to build (user: "build it with the expectation that this will become the base for an x3d browser")
**Branch:** `modernize-x3d-spec`

## Goal

Build the X3D **event model** runtime: the cascade engine that propagates events
along ROUTEs in a single timestamp, delivers them to destination fields and
`inputOnly` handlers, and exposes a clock entrypoint to drive time-dependent
nodes. This is **sub-project #1** — the foundation every active-node behavior
(sensors, interpolators, scripts) will build on. It is explicitly designed as
the base for a full X3D browser, so its seams are extensibility points, not
closed implementations.

## Current state (what exists)

- `Route` (`runtime/X3DRoute.hpp`) — pure data: `fromNode/fromField/toNode/toField`
  + optional resolved `weak_ptr` endpoints. No propagation.
- `on<Name>(value)` — generated for every `inputOnly` field, but an **empty stub**
  (`{ (void)value; }`). Not registrable, never invoked.
- Reflection (`X3DReflection.hpp` `FieldInfo`): `get` thunk for readable fields
  (inputOutput, outputOnly), `set` thunk for `inputOutput` only. `inputOnly` and
  `outputOnly` have no write/deliver thunk.
- `Scene` holds `routes` + a DEF symbol table + `resolveRoutes()`.

So: event **sources** (outputOnly/inputOutput) are readable today; `inputOutput`
sinks are writable today; `inputOnly` sinks and `outputOnly` source-writes are
not yet reachable generically.

## Architecture

A node-agnostic runtime engine driven by the existing reflection `FieldTable`.
Nodes stay data-only; all event state lives in the engine, keyed by
`(X3DNode*, fieldName)`. This mirrors how the codecs are node-agnostic and keeps
generated headers free of per-node event bookkeeping.

New runtime files under `runtime/events/`:

- **`FieldAddress`** — `(X3DNode*, std::string field)` value type; hashable; the
  identity of a routable field endpoint.
- **`EventGraph`** — the route table: `FieldAddress → vector<FieldAddress>`
  (fan-out). Built from a `Scene`'s resolved routes (or via `addRoute`). Reverse
  lookup not needed for forward propagation.
- **`EventCascade`** — the engine:
  - `postEvent(node, field, std::any value)` — seed an event (an "initial event"
    in X3D terms).
  - `process()` — drain the cascade: for each pending event, write the value to
    the sink via reflection `set` (inputOutput) or deliver to the handler
    (inputOnly); if the sink field is itself the source of routes, enqueue the
    downstream events. **Loop-breaking:** each ROUTE fires at most once per
    cascade (X3D single-timestamp fan-out rule), tracked by a per-cascade visited
    set of route edges. This guarantees termination even with cyclic routes.
  - Single-threaded, synchronous, deterministic ordering (FIFO per timestamp).
- **`X3DExecutionContext`** — owns an `EventGraph` + the node/DEF table + a clock;
  convenience API `route(...)`, `setField(...)` (set + cascade), `tick(now)`.
  Extends/wraps `Scene`. This is the object a browser drives each frame.

### Clock & active nodes (browser seam)

`tick(double now)` advances the context's time and calls an **active-node update
protocol** for registered behavior objects, then runs the resulting cascade.
Foundation provides the protocol + registry; concrete behaviors are later
sub-projects. The seam: a behavior reads inputs / time, writes its node's
`outputOnly` fields, and the engine cascades them.

## Codegen changes (regenerates golden; expected)

1. **`inputOnly` delivery.** `on<Name>` dispatches to a private
   `std::function<void(const T&)>` (default empty); add `setOn<Name>Handler(fn)`.
   Populate the reflection `set` thunk for inputOnly to call `on<Name>(value)`.
   → the cascade delivers to inputOnly generically; browser code plugs behavior in.
2. **`outputOnly` source write.** An internal write path so behaviors can produce
   outputs the engine reads via the existing `get` thunk (reflection `set` thunk
   populated for outputOnly too). Public read-only contract for users is
   unchanged; the write path is for runtime/behavior internals.

Both are template/emitter changes → new golden sha256; regenerate + recommit +
update `tests/test_golden_tree.py` and the golden hash in memory.

## Error handling

- Routes to/from unresolved nodes (null `weak_ptr`) are skipped, not fatal
  (matches existing serialization tolerance for IMPORTed/forward refs).
- Type mismatch on delivery (`std::any_cast` failure) → the route is rejected at
  registration time when types are known; at delivery a bad cast is caught and
  reported, never crashes the cascade.
- Range-constrained sink setters already `throw std::out_of_range`; the cascade
  lets that propagate to the caller of `process()` (a bad event is a program
  error, consistent with the setter contract).

## Testing (TDD, ctest)

`runtime/events/tests/cascade_test.cpp`, incrementally:

1. **Increment 1** (no codegen): `inputOutput → inputOutput` route propagates a
   value; fan-out (one source → two sinks); cyclic route terminates (loop-break).
2. **Increment 2**: route to an `inputOnly` field invokes the registered handler
   exactly once.
3. **Increment 3**: writing an `outputOnly` field cascades to a sink.
4. **Increment 4**: `tick()` drives a `TimeSensor → Interpolator → Transform`
   chain; the Transform's translation changes over successive ticks.

## Browser roadmap (decomposition — later sub-projects)

The user chose to map the whole roadmap. Each row is an independent sub-project
with its own spec → plan → build cycle, layered on this foundation:

| # | Sub-project | Depends on | Notes / new deps |
|---|---|---|---|
| **1** | **Event cascade foundation (this doc)** | — | runtime only + small codegen |
| 2 | Interpolators (Position/Orientation/Color/Scalar/Coordinate/Normal…) | 1 | pure math; easiest behavior family |
| 3 | TimeSensor + time-dependent lifecycle (start/stop/loop/pause) | 1 | the canonical animation driver |
| 4 | Followers (Damper/Chaser families) | 1,3 | per-frame easing |
| 5 | Sequencers / triggers / boolean filters | 1 | small logic nodes |
| 6 | Pointing-device & environmental sensors (Touch, Proximity, Visibility, Collision) | 1 | **needs spatial scene graph, bounds, picking → renderer** |
| 7 | Script node + SAI | 1 | **needs embedded ECMAScript VM** (large external dep) |
| 8 | PROTO/ExternPROTO IS-event mapping at runtime | 1,7 | route events across proto interface |
| 9 | Networking: Anchor, Inline, LoadSensor, ExternProto fetch | 1 | I/O + threading |
| 10 | Time-dependent media: AudioClip/MovieTexture/Sound | 3 | **needs audio/video decode** |
| 11 | Particle systems | 1,3 | simulation |
| 12 | Rigid-body physics, n-body collision | 1 | **physics engine** |
| 13 | NURBS tessellation, CAD, Geospatial, Volume rendering | 1 | each specialized + renderer |
| 14 | Rendering pipeline (the actual browser display) | most | **out of this repo's generator scope; separate engine** |

Families 6, 7, 10, 12, 13, 14 each require a renderer, a VM, or a specialized
external subsystem; they are the bulk of a browser and are deliberately deferred.
The foundation (row 1) plus rows 2–3 deliver a working, demonstrable animation
cascade — the minimum proof that this is a real browser base.
