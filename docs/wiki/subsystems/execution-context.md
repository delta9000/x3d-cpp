---
title: Execution Context
summary: Per-tick driver, field-write seam, and scene bridge that coordinate the runtime event loop.
tags: [subsystem, execution-context, tick, runtime, events]
updated: 2026-06-20
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

The Execution Context is the object a browser or SDK consumer drives each frame. It aggregates the route graph, the event cascade, the active-node/System registry, and all scene-level subsystems (transform hierarchy, bounds, binding stacks, pick, pointer, keyboard, head pose) into a single tick-addressable unit. Calling `tick(now)` is the sole entry point into the live event loop: it advances the clock, lets every registered System emit time-driven events, runs the event cascade to quiescence, fires post-cascade hooks (Script `eventsProcessed`), and then propagates dirty state through the transform and bounds subsystems. The Execution Context is the boundary between parse-time (a `Scene`) and runtime (live animated state).

## Key files

| File | Role |
|---|---|
| `runtime/events/X3DExecutionContext.hpp` | Primary type: owns route graph, cascade, System list, post-cascade hooks, and all scene-level subsystem instances; exposes `tick`, `buildSceneGraph`, `buildFrom`, `postEvent`, `writeField`, and pull surfaces |
| `runtime/events/X3DSceneBridge.hpp` | Free functions that bridge a parsed `Scene`'s DEF-named ROUTEs onto a context (`buildRoutes`), and convenience attach helpers for view-dependent, interpolator, event-utility, and key-device Systems |
| `runtime/events/X3DActiveNode.hpp` | `ActiveNode` — the legacy per-node behavior protocol (deprecated; wrapped by `ActiveNodeAdapter` inside `X3DExecutionContext`) |
| `runtime/events/X3DSystem.hpp` | `System` — the current behavior-family abstraction; `attach(node, ctx)` + `update(now, ctx)` |

## Interfaces and seams

### Exposed interface (`x3d::runtime::X3DExecutionContext`)

**Lifecycle — parse to runtime:**

```cpp
// Build the scene-graph indices (transforms, bounds, bindings, pick).
// Must be called once after parsing, before tick().
void buildSceneGraph(Scene &scene);

// Resolve and register a parsed Scene's DEF-named ROUTEs.
// Thin wrapper over buildRoutes() from X3DSceneBridge.hpp.
BridgeResult buildFrom(Scene &scene);
```

**Per-frame driver:**

```cpp
// Advance to time `now` (seconds). Calls every System::update, drains the
// cascade to quiescence (ISO 19775-1 §4.4.8.3 step 4 repeated-pass loop),
// fires post-cascade hooks, then propagates dirty transforms and bounds.
void tick(double now);

// Drain pending events without advancing the clock.
void process();

double now() const;
```

**Route management:**

```cpp
void addRoute(const FieldAddress &from, const FieldAddress &to);
void removeRoute(const FieldAddress &from, const FieldAddress &to);   // SAI §4.3.7
void clearRoutes();
```

**System registration:**

```cpp
void addSystem(std::shared_ptr<System> system);

// Convenience: inserts ScriptSystem first (prepareEvents before sensors)
// and wires its post-cascade eventsProcessed phase.
template <class ScriptSystemT>
void addScriptSystem(std::shared_ptr<ScriptSystemT> sys);

// Registers a hook run after the cascade drains each tick (ISO §29.2.4).
void addPostCascadeHook(std::function<void(X3DExecutionContext &)> hook);

// Deprecated: wraps an ActiveNode in a one-node System via ActiveNodeAdapter.
[[deprecated]] void addActiveNode(std::shared_ptr<ActiveNode> node);
```

**Event injection (push surface):**

```cpp
// Seed an event into the cascade (consumed on the next process/tick drain).
void postEvent(X3DNode *node, const std::string &field, std::any value);

// Direct field write that also classifies dirty — use instead of raw info.set
// when a System needs to poke a field outside the cascade (M2C-3 fix).
void writeField(X3DNode *node, const std::string &field, std::any value);
```

**Input seams (consumer-to-runtime, between ticks):**

```cpp
// Pointer / pointing-device sensor input (M2.5 input seam):
void setPointer(const Ray &worldRay);
void setPointerButton(bool down);
void setPointerPresent(bool present);
const PointerState &pointerState() const;

// Keyboard input (M2D PDS-4):
void setKey(int code, bool down);
void pushKeyCharacter(const std::string &c, bool down);
void pushActionKey(int code, bool down);
void pushModifierKey(int which, bool down);
void pushStringTerminator();
void pushStringDeletion();
void clearKeyEvents();
const KeyState &keyState() const;

// CAVE head-tracking (CONF-VIEWNAV):
void setHeadPose(const SFVec3f &pos, const SFRotation &ori);
const HeadPose &headPose() const;

// Per-viewpoint user navigation offset (§23.3.1):
const ViewpointOffset &viewpointOffset(X3DNode *vp) const;
void setViewpointOffset(X3DNode *vp, const ViewpointOffset &off);
```

**Pull surfaces (read after tick):**

```cpp
const DirtyTracker &dirtyTracker() const;
Mat4  worldTransform(const X3DNode *n) const;       // Transform node only (cached side-table)
Mat4  worldTransformAny(const X3DNode *n) const;    // any node (Transform = own world; non-Transform = nearest ancestor Transform's world; computed live)
Aabb  localBounds(const X3DNode *n) const;
Aabb  worldBounds(const X3DNode *n) const;          // composes localBounds with the ancestor Transform's world (via worldTransformAny)
X3DNode *boundViewpoint() const;
X3DNode *boundNavigationInfo() const;
X3DNode *boundBackground() const;
X3DNode *boundFog() const;
X3DNode *boundBindable(const std::string &category) const;
void removeBoundNode(X3DNode *node);       // BIND-06: pop deleted bound node
Mat4 viewMatrix() const;                   // world-to-camera from bound Viewpoint
SFVec3f cameraWorldPosition() const;
SFVec3f cameraWorldUp() const;
PickResult pick(const Ray &worldRay) const;
Mat4 worldOf(const X3DNode *node) const;   // parent-group frame of a sensor node
```

### Seam points

- **`System` abstraction** — behavior families implement `System::attach(X3DNode*, X3DExecutionContext&)` and `System::update(double, X3DExecutionContext&)`, then register via `addSystem`. The context calls every System's `update` each tick before draining the cascade. Event-driven systems (interpolators, event utilities) do all work in `attach`-wired inputOnly handlers and leave `update` a no-op.

- **`ActiveNode` (deprecated)** — the legacy one-node behavior protocol; wrapped by `ActiveNodeAdapter` inside the context. New code implements `System` directly.

- **`postEvent` / inputOnly handlers** — behaviors emit output events by calling `ctx.postEvent(node, field, value)`. The cascade delivers these to registered ROUTEs and wired inputOnly handlers within the same tick drain.

- **`addPostCascadeHook`** — `ScriptSystem` installs `runEventsProcessed` here so `Script::eventsProcessed()` fires after the batch cascade drains (ISO 19775-1 §29.2.4). Hooks may post further events, which are drained before `tick` returns.

- **`classifyDirty` (private)** — the cascade's field-delivery observer; maps any delivered `FieldAddress` to dirty flags (`DirtyField`, `DirtyLocalTransform`, `DirtyChildren`, `DirtyBounds`) on the owning node. `writeField` mirrors this classification for direct System writes (M2C-3).

- **`X3DSceneBridge.hpp` free functions** — `buildRoutes(Scene&, X3DExecutionContext&)` resolves DEF-named ROUTEs to `FieldAddress` endpoints and calls `ctx.addRoute`, validating with **three rejection categories** (unknown field, wrong direction, type mismatch). It also registers pre-resolved PROTO-body and Inline-internal routes directly (they bypass DEF-name resolution) and applies PROTO interface `IS` redirects via `scene.protoRedirects`. Returns `BridgeResult` (count of routes added + `RouteError` diagnostics for rejected routes; dangling DEFs are skipped silently — not counted as a rejection). The attach helpers (`attachViewDependent`, `attachInterpolators`, `attachEventUtilities`, `attachKeyDeviceSensors`) walk the scene once via `detail::forEachNode` and offer every node to each System's `attach`.

- **Consumer input seams** — the context owns `PointerState`, `KeyState`, and `HeadPose` structs that the consumer writes between ticks via the `setPointer*`, `setKey*`, `push*Key*`, and `setHeadPose` methods. Systems read these via `ctx.pointerState()`, `ctx.keyState()`, and `ctx.headPose()` inside `update`.

### Tick loop invariants

The `tick(now)` implementation enforces two spec requirements:

1. **Quiescence loop** (ISO 19775-1 §4.4.8.3 step 4): Systems are updated and the cascade is drained in a `do { update all systems } while (cascade.process(false) != 0)` loop. A single timestamp spans the entire pass set; the per-field-per-timestamp cap (`RTC-5`) prevents infinite loops by bounding productions to the finite field set.

2. **Reentrancy guard**: a `ticking_` flag is an implementation safety decision that causes a recursive `tick()` call (e.g. a System calling `tick` from `update`) to silently no-op, protecting timestamp and dirty state from clobbering. This guard is not an ISO 19775-1 requirement; it is a defensive implementation choice.

## How it is tested

- `ctest --preset dev -R x3d_events_tests` (doctest case: `m2b_tick_test`) — `runtime/events/tests/m2b_tick_test.cpp`: verifies that `buildSceneGraph` + `tick` correctly compute world and local bounds for a translated Shape, and that a cascade-delivered field change updates them.

- `ctest --preset dev -R x3d_event_scene_bridge` — `runtime/events/tests/scene_bridge_test.cpp`: validates ROUTE resolution (DEF names to `FieldAddress`), rejection diagnostics (unknown field, wrong direction, type mismatch), silent skip of dangling DEFs, and an end-to-end parse + `buildFrom` + `tick` animation cycle (TimeSensor → PositionInterpolator → Transform).

- `ctest --preset dev -R x3d_events_tests` (doctest case: `write_field_test`) — `runtime/events/tests/write_field_test.cpp`: verifies `writeField` updates the field value and classifies dirty identically to a cascade-delivered event (M2C-3 regression).

- `ctest --preset dev -R x3d_events_tests` (doctest case: `tick_audit_test`) — `runtime/events/tests/tick_audit_test.cpp`: covers tick-loop correctness edge cases — empty context, timestamp persistence, recursive-tick guard, system ordering, quiescence detection under re-posting, and post-cascade hook ordering.

- `ctest --preset dev -R x3d_events_tests` (doctest case: `cascade_observer_test`) — `runtime/events/tests/cascade_observer_test.cpp`: exercises the cascade field-delivery observer (the `classifyDirty` seam).

- `ctest --preset dev -R x3d_events_tests` (doctest case: `cascade_conformance_test`) — `runtime/events/tests/cascade_conformance_test.cpp`: conformance coverage for the cascade driving the context.

- Additional coverage via higher-level tests: `x3d_pointing_sensor_test`, `x3d_navigation_test`, `x3d_event_utility_test`, `x3d_key_device_sensor_test`, `x3d_viewpoint_bind_test`, and all scene and script tests exercise the context as their driver.

## Related specs and ADRs

- [Architecture](../architecture.md)
- [Event Cascade subsystem](../subsystems/event-cascade.md)
- [Routes subsystem](../subsystems/routes.md)
- [Dirty / Bounds / Transform subsystem](../subsystems/dirty-bounds-transform.md)
- [Sensors subsystem](../subsystems/sensors.md)
- [Script / SAI subsystem](../subsystems/system-script-sai.md)
- Spec: `docs/superpowers/specs/2026-06-20-project-wiki-design.md`
- ISO 19775-1 §4.4.8.3 (event model, per-tick evaluation order) and §29.2.4 (Script `eventsProcessed` timing) are the normative grounding for the tick loop and post-cascade hook ordering.
- BACKLOG items: M2C-3 (writeField dirty seam), M2.5 (input seam), M2D (keyboard + nav), CONF-VIEWNAV (viewMatrix formula), RTC-5/RTC-6 (timestamp cap + quiescence loop) — all closed; see `docs/superpowers/BACKLOG.md` (deprecated, historical).
