---
title: Execution Context
summary: Per-tick driver, field-write seam, and scene bridge that coordinate the runtime event loop.
tags: [subsystem, execution-context, tick, runtime, events]
updated: 2026-10-07
related:
  - ../architecture.md
  - ../subsystems/event-cascade.md
  - ../subsystems/routes.md
  - ../subsystems/dirty-bounds-transform.md
  - ../subsystems/sensors.md
  - ../subsystems/system-script-sai.md
---

# Execution Context

## Purpose

The Execution Context is the object a browser or SDK consumer drives each frame. It aggregates the route graph, the event cascade, the System registry, and all scene-level subsystems (transform hierarchy, bounds, binding stacks, pick, pointer, keyboard, head pose) into a single tick-addressable unit. Calling `tick(now)` is the sole entry point into the live event loop: it advances the clock, lets every registered System emit time-driven events, runs the event cascade to quiescence, fires post-cascade hooks (Script `eventsProcessed`), and then propagates dirty state through the transform and bounds subsystems. The Execution Context is the boundary between parse-time (a `Scene`) and runtime (live animated state).

Runtime entry normalizes explicitly authored fields with known dimensions once,
before scene systems consume them. `buildSceneGraph`, `buildFrom`, and
`refreshSceneTopology` share per-scene normalized-field marks, so repeated
setup and topology refreshes do not multiply conversion factors. Built-in
defaults stay canonical. After entry, `writeField`, event payloads and routed
updates use initial units directly. The implementation and field map live in
`runtime/UnitConversion.hpp`.

`visible` writes are active-traversal changes (`DirtyChildren | DirtyBounds`),
like active-child selection. This lets extraction remove hidden placements and
recover initially hidden branches without depending on existing render items.
Other Shape and scoped ClipPlane/LocalFog descriptor invalidation is handled by
the extractor's [replacement contract](extract.md#scoped-render-state-replacement-contract).

## Activation ownership and retirement

A mutable native node graph belongs to one active execution context. The host
owns that activation and serializes construction, ticks, field calls and
teardown; there is no hidden event loop and no concurrent-destruction guarantee.
`RuntimeSession` owns the document, context and extractor. A caller may retain a
node `shared_ptr` beyond the session, but that does not retain its runtime.

Native-installed bindable, interpolator (including spline, NURBS and geospatial),
follower, event-utility and H-Anim motion input handlers carry weak lifetime
leases. Behavior handlers require both the context and the individual `System`
to survive; bindable handlers require their context-owned `BindingSystem`.
Context-owned timing filters and key/trigger write listeners are guarded too.
After retirement, calling these retained native input handlers is a no-op:
readable field storage remains, and ordinary native storage setters still work.
The guards do not traverse retained nodes, retain the scene, or clear a handler
installed by a later activation. They also cover attachment failure before a
new system has reached `addSystem`, and session-constructor unwinding. Standalone
`BindingSystem` and all standard callback-owning systems retire in their
destructor bodies before member-owned captures or state can be destroyed; a
capture destructor that invokes a retained node therefore sees an inert handler.

`bool X3DExecutionContext::retireCallbacks() noexcept` is an idempotent, terminal
revocation seam. It returns `false`, leaving the activation unchanged, while a
native guarded input handler or `tick()`, `process`, or `writeField` is in flight.
The host must defer replacement/destruction until its current runtime call has
returned. Destroying the context/session reentrantly is a contract violation and
fails closed with `std::terminate`; the guards are not a lock or a mechanism to
make deletion from a running callback safe. No further runtime operations may
be made on a retired activation. The session destructor retires callbacks before
extractor teardown; the context destructor independently retires them before any
context-owned member is destroyed. The low-level context path therefore has the
same retained-node protection.

Custom `System` implementations must wrap escaping void handlers with
`ctx.guardCallback(*this, handler)` (or predicate filters with
`ctx.guardInputFilter(*this, filter)`) to opt into that protection. A custom
most-derived owner must also call the protected
`retireCallbacksBeforeDestruction()` at the start of its destructor, before its
members are destroyed. The `System` base destructor alone is too late for a
member/capture destructor that invokes an escaped handler. This applies equally
to further subclasses of built-in systems. Install custom handlers after full
construction; if a custom constructor installs handlers and then can throw, it
must retire them before member-unwinding begins. It does not
apply to unguarded callbacks supplied by an embedder, dangling native raw
pointers, concurrent native access, or arbitrary structural mutation. Projection selection is fixed per context; see [geospatial ownership](geospatial.md#world-ownership-and-migration).
Author fields now have the explicit scene ownership described below.

### Author-field ownership

Every fresh `Scene` owns a fresh `shared_ptr<DynamicFieldStore>` in
`scene.authorFields`. Shallow Scene copies deliberately share both node identity
and that store; they are not independent worlds. `ProtoDeclaration::authorFields`
retains its template/default fields after an EXTERN resolver's temporary document
dies. `RuntimeSession` binds its context to its owned document's store. Retaining only
a node does not retain that Scene store; retain the explicit shared owner too
when author fields must remain readable after world teardown.

For low-level setup, prefer `X3DExecutionContext ctx(doc.scene.authorFields)`.
An untouched default context binds once when `buildSceneGraph`, `buildFrom` or
`buildRoutes` receives a Scene. Calling `ctx.authorFields()` first fixes its fresh
standalone owner; later attempts to switch owners, or to bind while events are
pending or after a tick, throw before changing scene state. Rebuild all concrete
runtime consumers for this source/ABI change. Never replace a Scene's owner while
it is active.

Inline adoption shares author-field entries for its imported nodes, preserving
one value identity without sharing the entire store. Detachment drops only the
parent view: a retained child Scene or cached GeoLOD tile keeps its author data.
Redisplaying a cached tile imports those entries again before system attachment.
Authored GeoLOD rootNode fields use a per-LOD retained entry view while inactive,
then return to the active Scene on redisplay.
Conflicting live entries for one imported node are rejected. Copied FieldInfo
thunks are inert after their tracked node expires, even before any store lookup
sweeps the entry; they also reject calls on a different node identity.

The removed zero-argument `dynamicFieldStore()` has no global or thread-local
compatibility fallback. Use `*scene.authorFields`, `*declaration.authorFields`, or
`ctx.authorFields()` at the appropriate ownership boundary, and pass that store
to `effectiveFields(node, store)`. See [ADR-0057](../decisions/0057-scene-owned-author-fields.md)
for the clone and serializer migration. This does not establish general SAI,
Script, PROTO or Geo conformance.

### Process-wide diagnostics

`TransformSystem::localMatrixCallCount()` and
`X3DExecutionContext::pickCallCount()` return process-wide diagnostic totals,
not per-world measurements or semantic scene state. Their backing counters use
relaxed atomic increments and reads so separate owner-thread worlds can
contribute without a diagnostic data race. This does not make an individual
scene/context thread-safe, nor synchronize scene changes. The concurrent
`runtime_diagnostic_counters_test.cpp` test drives four distinct owner-thread
worlds and checks their combined totals after joining those threads. The two
public backing variables now have atomic type; callers should use the unchanged
`uint64_t` accessors rather than copying a backing counter object.

### Source compatibility and migration

`X3DExecutionContext`, `BindingSystem`, and `System` (therefore all built-in and
custom derived systems) explicitly delete copy construction, copy assignment,
move construction and move assignment. Their callbacks and internal references
bind to a stable owner address. The previous context and binding-system copies,
and stateless system copies, really compiled; they were not safe activation
clones. Context copy/move assignment already failed because its cascade holds
a reference; those deletions make the existing restriction explicit. Some
stateful systems already failed to instantiate copies because they own
`unique_ptr` state. `RuntimeSession` was already noncopyable/nonmovable.

Keep these owners in place or transfer `unique_ptr`/`shared_ptr` ownership,
rather than moving the owner object. To duplicate an activation, create a fresh
context and fresh systems, then attach them to a separately owned node graph.
To re-use retained nodes, first retire the previous activation outside all
runtime calls, then attach the replacement. This is a source-compatibility
tightening; downstream applications need a rebuild and call-site audit.

## Key files

| File | Role |
|---|---|
| `runtime/events/X3DExecutionContext.hpp` | Primary type: owns route graph, cascade, System list, post-cascade hooks, and all scene-level subsystem instances; exposes `tick()`, `buildSceneGraph`, `buildFrom`, `postEvent`, `writeField`, and pull surfaces |
| `runtime/events/X3DSceneBridge.hpp` | Free functions that bridge a parsed `Scene`'s DEF-named ROUTEs onto a context (`buildRoutes`), and convenience attach helpers for view-dependent, interpolator, event-utility, and key-device Systems |
| `runtime/events/CallbackLifetime.hpp` | Weak serial callback leases, terminal revocation and in-flight teardown checks |
| `runtime/events/X3DSystem.hpp` | `System` — the current behavior-family abstraction; `attach(node, ctx)` + `update(now, ctx)` |

## Interfaces and seams

### Exposed interface (`x3d::runtime::X3DExecutionContext`)

Lifecycle — parse to runtime:

```cpp
// Normalize authored known dimensional fields once, then build the
// scene-graph indices (transforms, bounds, bindings, pick).
// Must be called after parsing, before tick().
void buildSceneGraph(Scene &scene);

// Normalize any not-yet-normalized fields, then resolve and register the
// parsed Scene's DEF-named ROUTEs. Already normalized fields are skipped.
// Thin wrapper over buildRoutes() from X3DSceneBridge.hpp.
BridgeResult buildFrom(Scene &scene);
```

Per-frame driver:

```cpp
// Advance to time `now` (seconds). Calls every System::update, drains the
// cascade to quiescence (ISO 19775-1 §4.4.8.3 step 4 repeated-pass loop),
// fires post-cascade hooks, then propagates dirty transforms (local TRS changes
// AND DirtyChildren structural re-index) and bounds.
void tick(double now);

// Drain pending events without advancing the clock.
void process();

double now() const;
```

Route management:

```cpp
void addRoute(const FieldAddress &from, const FieldAddress &to);
void removeRoute(const FieldAddress &from, const FieldAddress &to);   // SAI §4.3.7
void clearRoutes();
```

System registration:

```cpp
void addSystem(std::shared_ptr<System> system);

// Convenience: inserts ScriptSystem first (prepareEvents before sensors)
// and wires its post-cascade eventsProcessed phase.
template <class ScriptSystemT>
void addScriptSystem(std::shared_ptr<ScriptSystemT> sys);

// Registers a hook run after the cascade drains each tick (ISO §29.2.4).
void addPostCascadeHook(std::function<void(X3DExecutionContext &)> hook);

```

Event injection and direct writes:

```cpp
void postEvent(X3DNode *node, const std::string &field, std::any value);
[[nodiscard]] FieldWriteResult
writeField(X3DNode *node, const std::string &field, std::any value);

enum class FieldWriteResult {
    Ok, NullNode, DetachedNode, UnknownField, NotWritable, TypeMismatch
};
```

`writeField` resolves generated and author fields through the context's owner.
Successful writes notify dirty tracking; unknown, detached, unwritable, or
wrongly typed writes return a distinct result without mutation. Timing input
filters can accept an ignored write as `Ok` without changing the value.


Input seams (consumer-to-runtime, between ticks):

```cpp
// Pointer / pointing-device sensor input:
void setPointer(const Ray &worldRay);
void setPointerButton(bool down);
void setPointerPresent(bool present);
const PointerState &pointerState() const;

// Keyboard input:
void setKey(int code, bool down);
void pushKeyCharacter(const std::string &c, bool down);
void pushActionKey(int code, bool down);
void pushModifierKey(int which, bool down);
void pushStringTerminator();
void pushStringDeletion();
void clearKeyEvents();
const KeyState &keyState() const;

// CAVE head-tracking:
void setHeadPose(const SFVec3f &pos, const SFRotation &ori);
const HeadPose &headPose() const;

// Per-viewpoint user navigation offset (§23.3.1):
const ViewpointOffset &viewpointOffset(X3DNode *vp) const;
void setViewpointOffset(X3DNode *vp, const ViewpointOffset &off);
```

Pull surfaces (read after tick):

```cpp
const DirtyTracker &dirtyTracker() const;
Mat4  worldTransform(const X3DNode *n) const;       // Transform node only (cached side-table)
Mat4  worldTransformAny(const X3DNode *n) const;    // any node (Transform = own world; non-Transform = nearest ancestor Transform's world; computed live)
Mat4  worldTransformUnder(const X3DNode *parent, const X3DNode *n) const; // Transform n through one parent edge; DEF/USE node => a world per parent
std::uint64_t transformRevision() const;            // monotonic TransformSystem::revision() — bump per world/index change
Aabb  localBounds(const X3DNode *n) const;
Aabb  worldBounds(const X3DNode *n) const;          // composes localBounds with the ancestor Transform's world (via worldTransformAny)
X3DNode *boundViewpoint() const;
X3DNode *boundNavigationInfo() const;
X3DNode *boundBackground() const;
X3DNode *boundFog() const;
X3DNode *boundBindable(const std::string &category) const;
void removeBoundNode(X3DNode *node);
Mat4 viewMatrix() const;                   // world-to-camera from bound Viewpoint
SFVec3f cameraWorldPosition() const;
SFVec3f cameraWorldUp() const;
PickResult pick(const Ray &worldRay) const;  // index-backed (see below)
Mat4 worldOf(const X3DNode *node) const;   // parent-group frame of a sensor node
```

`pick()` threads the live viewer pose and the context's `TransformSystem` into an index-backed `pickClosest`: the point index is rebuilt only when `transformRevision()` changes and its cached world AABBs refit only when the `BoundsSystem` revision changes, so a pick on an unchanged scene costs a broad phase over geometry-bearing placements instead of a whole-graph walk. Billboard placements are re-resolved per pick (view-dependent).

### Seam points

- `System` abstraction — behavior families implement `System::attach(X3DNode*, X3DExecutionContext&)` and `System::update(double, X3DExecutionContext&)`, then register via `addSystem`. The context calls every System's `update` each tick before draining the cascade. Event-driven systems (interpolators, event utilities) do all work in `attach`-wired inputOnly handlers and leave `update` a no-op.

- `postEvent` / inputOnly handlers — behaviors emit output events by calling `ctx.postEvent(node, field, value)`. The cascade delivers these to registered ROUTEs and wired inputOnly handlers within the same tick drain.

- `addPostCascadeHook` — `ScriptSystem` installs `runEventsProcessed` here so `Script::eventsProcessed()` fires after the batch cascade drains (ISO 19775-1 §29.2.4). Hooks may post further events, which are drained before `tick()` returns.

- `addChildren` / `removeChildren` — handled in the cascade (`EventCascade::editChildren`) for any node with an MFNode `children` field (§10.2.1): add appends nodes not already present, remove drops the listed ones, edits apply in delivery order, and a `children` event with the final value follows in the same cascade, so dirty tracking and `children_changed` ROUTEs see it.
- `classifyDirty` (private) — the cascade's field-delivery observer; maps any delivered `FieldAddress` to dirty flags (`DirtyField`, `DirtyLocalTransform`, `DirtyChildren`, `DirtyBounds`) on the owning node. `writeField` mirrors this classification for direct System writes. `DirtyChildren` (a `children`/`addChildren`/`removeChildren` write, or a `Switch.whichChoice` swap) is what drives `TransformSystem`'s structural re-walk each tick.

- `X3DSceneBridge.hpp` free functions — `buildRoutes(Scene&, X3DExecutionContext&)` resolves DEF-named ROUTEs to `FieldAddress` endpoints and calls `ctx.addRoute` after field, direction, and type checks. For expanded PROTOs it consults the declared interface before the primary node's fields, follows `IS` targets on either endpoint, and retains inherited `metadata` through the primary's current storage. Pre-resolved PROTO-body and Inline-internal routes bypass DEF-name lookup but receive the same physical endpoint checks through `effectiveFields()`, without another PROTO redirect lookup. `BridgeResult` counts added edges; `RouteError::scope` identifies the Scene, PROTO-body, or Inline route collection for its relative `index`. Dangling Scene DEFs are skipped silently. The attach helpers walk rendered roots and non-rendered PROTO peers via `detail::forEachNode`. `InlineRuntimeSystem` enrolls a newly loaded subtree and installs its dynamic routes through a separate path; it also refreshes transform, bounds, and pick indices before extraction. Unconnected PROTO interface fields use Scene-owned entries; this does not establish a general native SAI prototype object model.

- Consumer input seams — the context owns `PointerState`, `KeyState`, and `HeadPose` structs that the consumer writes between ticks via the `setPointer*`, `setKey*`, `push*Key*`, and `setHeadPose` methods. Systems read these via `ctx.pointerState()`, `ctx.keyState()`, and `ctx.headPose()` inside `update`.

### Tick loop invariants

The `tick(now)` implementation enforces two spec requirements:

1. Quiescence loop (ISO 19775-1 §4.4.8.3 step 4): Systems are updated and the cascade is drained in a `do { update all systems } while (cascade.process(false) != 0)` loop. A single timestamp and its route/output admission guards span the entire pass set.

2. Reentrancy guard: a `ticking_` flag is an implementation safety decision that causes a recursive `tick()` call (e.g. a System calling `tick()` from `update`) to silently no-op, protecting timestamp and dirty state from clobbering. This guard is not an ISO 19775-1 requirement; it is a defensive implementation choice.

### Field-write listeners

`addFieldWriteListener(FieldWriteListener)` registers a callback run after every field write, whether a cascade delivery or `writeField`, after dirty classification. Systems use it for nodes that react to an inputOutput write themselves: IntegerTrigger re-emitting `triggerValue` (§30.4.6) and key-device focus arbitration (§21.2). A listener may post events; they join the current timestamp.
## Related

- [Architecture](../architecture.md)
- [Event Cascade subsystem](../subsystems/event-cascade.md)
- [Routes subsystem](../subsystems/routes.md)
- [Dirty / Bounds / Transform subsystem](../subsystems/dirty-bounds-transform.md)
- [Sensors subsystem](../subsystems/sensors.md)
- [Script / SAI subsystem](../subsystems/system-script-sai.md)

- ISO 19775-1 §4.4.8.3 (event model, per-tick evaluation order) and §29.2.4 (Script `eventsProcessed` timing) are the normative grounding for the tick loop and post-cascade hook ordering.
