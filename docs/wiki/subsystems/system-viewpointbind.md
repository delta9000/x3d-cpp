---
title: Viewpoint Binding System
summary: Binding stacks for Viewpoint, NavigationInfo, Background, and Fog — ISO 19775-1 §23 bind semantics, bind-time offset rules, and animated viewpoint transitions.
tags: [subsystem, viewpoint, binding-stack, navigation-info, background, fog]
updated: 2026-06-20
related:
  - ../architecture.md
  - ../subsystems/system-navigation.md
  - ../subsystems/system-viewdependent.md
  - ../subsystems/execution-context.md
  - ../subsystems/event-cascade.md
---

# Viewpoint Binding System

The Viewpoint Binding System implements the ISO/IEC 19775-1 §23.3.1 "Viewpoint
binding" rules for all four bindable categories: Viewpoint (and OrthoViewpoint),
NavigationInfo, Background, and Fog. It owns the per-category LIFO binding
stacks, the default-bind pass that runs at scene load time, the bind-time offset
protocol (jump / retainUserOffsets / stored relative transform), and the animated
viewpoint transition (BIND-05).

## Purpose

X3D defines a shared "binding stack" for each category of bindable node. At most
one node per category is bound at any time; `set_bind TRUE` pushes a node to the
top and makes it the active entry; `set_bind FALSE` pops it and restores the
previous top. The subsystem realises this protocol and enforces three additional
invariants required by the spec:

- **Authored fields are never mutated** by navigation or by a bind (§23.2.3,
  BIND-01). The user offset is process-local runtime state kept separately in
  `ViewpointOffset`, accumulated in `viewpointOffset(vp).local` inside
  `X3DExecutionContext`.

- **Bind-time offset rules** (BIND-04/07/08): when a viewpoint becomes bound,
  `ViewpointBindSystem` decides what happens to its stored offset based on
  `jump` and `retainUserOffsets`. If `jump=false` the effective camera does not
  move across the bind; if `jump=true` and `retainUserOffsets=false` the offset
  is reset so the camera snaps to the authored pose; if `retainUserOffsets=true`
  the stored offset is left in place.

- **Animated transitions** (BIND-05): unless the governing `NavigationInfo` has
  `transitionType[0] = TELEPORT` or `transitionTime = 0`, the effective camera
  animates from the old position/orientation to the new one over `transitionTime`
  seconds, then posts `transitionComplete` to the NavigationInfo. A
  rotation-only bind (same eye position, different orientation) also triggers the
  animation.

The subsystem is split into three orthogonal layers, each independently testable:
`BindingStack` (pure LIFO stack logic), `BindingSystem` (scene enrolment,
default-bind, and `set_bind` handler wiring), and `ViewpointBindSystem`
(post-cascade hook that applies the offset rules and drives transitions).

## Key files

| File | Role |
|---|---|
| `runtime/scene/BindingStack.hpp` | `BindingStack` — one bindable category's LIFO stack; top = bound node; emits `(node, bool)` events through a caller-supplied `Emit` callback on every transition |
| `runtime/scene/BindingSystem.hpp` | `BindingSystem` — owns one `BindingStack` per category; walks the scene on `enrollScene`, wires each bindable node's `set_bind` handler, and default-binds the first of each category on `bindDefaults` |
| `runtime/events/ViewpointBindSystem.hpp` | `ViewpointBindSystem` + free function `attachViewpointBind` — post-cascade hook; applies jump/retainUserOffsets offset rules, starts/advances animated transitions, dispatches BIND-02 NavigationInfo bind-chain |
| `runtime/events/ViewpointOffset.hpp` | `ViewpointOffset` — a single `Mat4 local` field: the per-viewpoint relative viewing transformation (§23.3.1); process-local, not part of the scene graph |
| `runtime/events/X3DExecutionContext.hpp` | Owns `BindingSystem bindings_`, `HeadPose head_`, and `std::unordered_map<X3DNode*, ViewpointOffset> offsets_`; exposes the full view-matrix seam and the binding query API consumed by renderers and navigation |

## Interfaces and seams

### Exposed interface

The subsystem exposes three distinct groups of API, all on `X3DExecutionContext`
(declared in `runtime/events/X3DExecutionContext.hpp`):

**Binding queries (read-only; called by navigation, extract, rendering):**

```cpp
X3DNode *boundViewpoint()       const; // top of the "Viewpoint" stack
X3DNode *boundNavigationInfo()  const; // top of the "NavigationInfo" stack
X3DNode *boundBackground()      const; // top of the "Background" stack
X3DNode *boundFog()             const; // top of the "Fog" stack
X3DNode *boundBindable(const std::string &category) const; // generic lookup
```

**View-matrix and camera-space API (called by extract, navigation, picking):**

```cpp
Mat4    viewMatrix()              const; // world->camera; effective = worldOf·pose·offset·head
SFVec3f cameraWorldPosition()    const; // camera-to-world origin
SFVec3f cameraWorldUp()          const; // +Y of the camera frame in world space

const ViewpointOffset &viewpointOffset(X3DNode *vp) const;
void setViewpointOffset(X3DNode *vp, const ViewpointOffset &off);
```

The `viewMatrix()` formula is:

```
cam = worldOf(vp) · T(position) · R(orientation) · offset.local · T(head.position) · R(head.orientation)
viewMatrix = cam⁻¹
```

**Head-pose seam (called by the CAVE consumer per frame):**

```cpp
void setHeadPose(const SFVec3f &pos, const SFRotation &ori);
const HeadPose &headPose() const;
```

**Node removal (called when a scene node is deleted at runtime):**

```cpp
void removeBoundNode(X3DNode *node); // BIND-06: behaves as set_bind FALSE → pops the stack
```

**Wire-up (called once after `buildSceneGraph`):**

```cpp
// In ViewpointBindSystem.hpp — registers the post-cascade hook:
void attachViewpointBind(X3DExecutionContext &ctx);
```

### Seam points

- **Post-cascade hook** — `attachViewpointBind` calls
  `ctx.addPostCascadeHook(...)`, passing a shared `ViewpointBindSystem` instance.
  `X3DExecutionContext` stores all hooks in `postCascade_` (a
  `std::vector<std::function<void(X3DExecutionContext&)>>`) and calls them after
  every cascade drain in `tick()`. This is the same hook slot used by
  `ScriptSystem` for `eventsProcessed()`.

- **BindingSystem ↔ execution context decoupling** — `BindingSystem` never
  includes `X3DExecutionContext.hpp`. It receives a `Poster` callback
  (`std::function<void(X3DNode*, std::string, std::any)>`) and a `Clock`
  callback (`std::function<double()>`) on `enrollScene`; in production these
  forward to `ctx.postEvent` and `ctx.now()`. This keeps `BindingSystem` free of
  include cycles with the context.

- **BindingStack ↔ BindingSystem decoupling** — `BindingStack` never touches
  field values. Callers supply an `Emit` callback that is called with
  `(node, true/false)` on every bound/unbound transition; `BindingSystem` wires
  this to post `isBound` and `bindTime` events back through the cascade.

- **Default bind at load time** — `BindingSystem::bindDefaults()` writes
  `isBound` and `bindTime` directly via reflection (bypassing the event cascade)
  because at load time no cascade is active. This is the §23.3.1 "during loading"
  case; the spec requires `bindTime` to be emitted even here.

- **ViewpointBindSystem ↔ NavigationSystem** — `ViewpointBindSystem` reads
  `NavigationInfo.transitionTime` and `transitionType` via
  `geombounds::getField`; it also posts `set_bind TRUE` to the viewpoint's
  associated `NavigationInfo` (BIND-02) by calling `ctx.postEvent`. The
  NavigationSystem consumes `boundNavigationInfo()` independently; the two
  systems do not include each other.

- **CAVE consumer head-pose** — `ViewpointOffset` is explicitly documented as
  process-local (not serialized to the scene graph), so each CAVE wall process
  owns its own offset map and head pose while receiving the same authored
  viewpoint fields from the master. See `runtime/events/ViewpointOffset.hpp`.

## How it is tested

- `ctest --preset dev -R x3d_geometry_scene` (doctest case: `binding_stack_test`) — pure stack logic: push, pop,
  move-to-top, already-top no-op, non-top removal, empty-stack unbind, event
  emission order (`runtime/scene/tests/binding_stack_test.cpp`).

- `ctest --preset dev -R x3d_geometry_scene` (doctest case: `binding_system_test`) — enrol scene, default-bind,
  `set_bind TRUE/FALSE` fired through wired handler, `isBound` delivered via
  `ctx.process()`, category independence (`runtime/scene/tests/binding_system_test.cpp`).

- `ctest --preset dev -R x3d_geometry_scene` (doctest case: `binding_stack_audit_test`) — AUD-BIND-STACK regression:
  empty-stack unbind, rapid bind/unbind transitions, `bindTime` fractional-second
  precision, `isBound`-before-`bindTime` event ordering, re-binding top is no-op
  (`runtime/scene/tests/binding_stack_audit_test.cpp`).

- `ctest --preset dev -R x3d_events_tests` (doctest case: `viewpoint_offset_test`) — CONF-VIEWNAV Phase 1 / user-offset
  model: offset moves effective camera without touching authored fields (BIND-01);
  head-pose composition; EXAMINE navigation leaves authored position/orientation
  byte-unchanged; `OrthoViewpoint` navigates (BIND-03); `rotationFromMatrix`
  round-trip (`runtime/events/tests/viewpoint_offset_test.cpp`).

- `ctest --preset dev -R x3d_events_tests` (doctest case: `viewpoint_bind_test`) — CONF-VIEWNAV Phase 2 / bind-time
  offset rules: `jump=false` keeps effective view continuous (BIND-04);
  `jump=true` + `retainUserOffsets=false` snaps to authored pose (BIND-07/08);
  `retainUserOffsets=true` preserves stored offset on rebind; BIND-02 NavigationInfo
  dispatch; BIND-06 node removal pops stack; BIND-05 animated transition (midpoint
  check + `transitionComplete`); per-viewpoint NavigationInfo TELEPORT overrides
  bound NavigationInfo; rotation-only bind animates
  (`runtime/events/tests/viewpoint_bind_test.cpp`).

### BIND-09: Pop restores stored offset (§23.3.1 r6.3)

A pop (set_bind FALSE on the bound viewpoint, or deletion of the bound node)
must restore the popped-to viewpoint's stored user offset — not reset it like
a fresh push. The `jump`/`retainUserOffsets` fields govern push binds only
(rule 5.1); a pop (rule 6.3) always restores.

The fix adds a `BindTransition` side-channel (`None`/`Push`/`Pop`) from
`BindingSystem` to `ViewpointBindSystem`:

- `BindingSystem` sets `Push` on `set_bind TRUE` / `bindDefaults`, and `Pop`
  on `set_bind FALSE` / `removeNode` (only when the node was the bound top).
- `X3DExecutionContext` holds `lastVpTransition_`, set via a `TransitionSink`
  callback wired at `enrollScene`.
- `ViewpointBindSystem::onPostCascade` reads the kind, passes it to `onBind`,
  and resets it to `None`. `onBind`'s `Pop` branch skips the offset reset,
  preserving the stored offset; the animation block still runs (from the old
  camera to the restored target over `transitionTime`).

Tests: `test_pop_restores_stored_offset`, `test_push_still_resets` (negative
control), `test_pop_animates_over_transition_time`.

## Related specs and ADRs

- [Architecture](../architecture.md)
- [Execution Context](../subsystems/execution-context.md)
- [Navigation System](../subsystems/system-navigation.md)
- [View-Dependent System](../subsystems/system-viewdependent.md)
- [Event Cascade](../subsystems/event-cascade.md)
- Spec: ISO/IEC 19775-1:2023 §23.3.1 — Viewpoint binding and the "stored relative
  transformation" (offset) model; the `jump`, `retainUserOffsets`, `transitionTime`,
  `transitionType`, and `transitionComplete` rules. §23.2.3 — authored
  `position`/`orientation` fields must not be mutated by navigation (BIND-01).
  §7.2.2 — general bindable node protocol (set_bind TRUE/FALSE, LIFO stack,
  isBound, bindTime on every transition).
- `docs/conformance/findings.yaml` — CONF-VIEWNAV Phase 2 conformance findings
  BIND-01..BIND-08 (the finding set that drove the implementation split into
  BindingStack / BindingSystem / ViewpointBindSystem / ViewpointOffset).
