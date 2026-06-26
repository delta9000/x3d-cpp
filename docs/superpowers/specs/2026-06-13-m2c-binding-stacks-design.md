# M2c — Binding Stacks (Viewpoint / NavigationInfo / Background / Fog)

**Date:** 2026-06-13
**Branch:** `modernize-x3d-spec`
**Status:** approved (user: "workflow for m2c all in one no questions" — autonomous design)
**Milestone:** M2c. Roadmap `2026-06-07-architecture-validation-and-resequencing.md` §3
Step 2 (highest-leverage M2 content: binding-stack nodes = 24.5% of corpus). Sits on
the M1 event cascade; independent of M2a/M2b (no transform/bounds dependency).

## Context

Bindable nodes (Viewpoint, NavigationInfo, Background, Fog families) follow X3D's
binding-stack protocol (ISO/IEC 19775-1 §7.2.2): per category, a stack of nodes; the
**top is the active ("bound") node**; `set_bind` events push/move/pop; `isBound`
(outputOnly SFBool) and `bindTime` (outputOnly SFTime) fire on transitions. Exactly
one node per category is bound at a time. M2c maintains **which node is bound** per
category and drives the isBound/bindTime events; the *effect* of a binding (camera
pose, fog params, background) is the consumer/extraction concern (M2.5), not M2c.

**Runtime-only, golden byte-identical** (`7226b3a07e744b4aea1c3d9f34897384e5d8c085cb0c79743b8a23872c7183c0`).
No codegen — the bind state lives in a side structure; isBound/bindTime use the
existing generated emit/handler API.

## Verified surface

- `X3DBindableNode` (generated abstract base) exposes: `setOnSet_bindHandler(std::function<void(const SFBool&)>)`
  (register the inputOnly `set_bind` handler), `emitIsBound(const SFBool&)`,
  `emitBindTime(const SFTime&)`, `getIsBound()`/`getBindTime()`. The reflection `set`
  thunk for `set_bind` calls `onSet_bind(value)` → the registered handler; the thunks
  for `isBound`/`bindTime` call the emits. So a cascade delivery of `set_bind` invokes
  the handler, and `ctx.postEvent(node,"isBound",…)` / `…"bindTime"…` route + emit.
- Bindable concrete types: `Viewpoint`, `OrthoViewpoint`, `GeoViewpoint`,
  `NavigationInfo`, `Background`, `TextureBackground`, `Fog`. Abstract bases present:
  `X3DViewpointNode`, `X3DBackgroundNode`.
- `SFBool` = `bool`, `SFTime` = `double` (X3Dtypes.hpp).
- `X3DExecutionContext`: `postEvent(node, field, std::any)`, `now()`, `tick(now)` (runs
  systems → cascade → transforms → bounds). M2c plugs in beside `transforms_`/`bounds_`.

## Design

### 1. Category mapping

A bindable node's **stack category** is its bindable family (all viewpoint subtypes
share one stack; both background subtypes share one):
- `dynamic_cast<X3DViewpointNode*>` → `"Viewpoint"`
- `dynamic_cast<X3DBackgroundNode*>` → `"Background"`
- else the node's `nodeTypeName()` (→ `"NavigationInfo"`, `"Fog"`, or any future
  bindable type, data-driven).
Bindable detection: `dynamic_cast<X3DBindableNode*>(node) != nullptr`.

### 2. `BindingStack` (`runtime/scene/BindingStack.hpp`)

Pure stack logic for one category, emitting transitions through a caller-supplied
callback `using Emit = std::function<void(X3DNode* node, bool bound)>` (decouples it
from the context — testable standalone):
- `top()` → back of stack (the bound node) or null.
- `bind(node, emit)` (`set_bind TRUE`): if `node` is already top, no-op; else remove
  any existing occurrence, push to top; if there was a different previous top emit
  `(prev,false)`, then emit `(node,true)`.
- `unbind(node, emit)` (`set_bind FALSE`): if `node` is top → pop, emit `(node,false)`,
  and if a new top remains emit `(newTop,true)`; if `node` is not top → just remove it
  (no isBound transition).
- `pushDefault(node, emit)`: push + emit `(node,true)` (startup default-bind).
- **§23.3.1 (corrected, RTC-9):** the caller's `emit` callback fires `bindTime` on
  **both** the `true` and `false` cases — every isBound transition, including unbind
  and the load-time default bind.

### 3. `BindingSystem` (`runtime/scene/BindingSystem.hpp`)

Context-owned manager (like TransformSystem/BoundsSystem — NOT a per-tick `System`,
since it is purely event-driven via handlers):
- `unordered_map<std::string, BindingStack> stacks_` + `unordered_map<std::string,
  std::vector<X3DNode*>> enrolled_` (per-category enrolment order).
- `enrollScene(const Scene&, X3DExecutionContext&)`: DFS the scene graph (node-typed
  fields, guarding `if(!f.get) continue;`); for each bindable node, record it in its
  category and wire its `set_bind` handler:
  ```
  bindable->setOnSet_bindHandler([this, node, cat](const SFBool& v){
    Emit emit = [this](X3DNode* t, bool bound){
      // §23.3.1: bindTime fires on EVERY isBound transition — bind AND unbind.
      ctx->postEvent(t, "isBound",   std::any(SFBool(bound)));
      ctx->postEvent(t, "bindTime",  std::any(SFTime(clock_())));
    };
    v ? stacks_[cat].bind(node, emit) : stacks_[cat].unbind(node, emit);
  });
  ```
  (capture the context by pointer; it owns the BindingSystem and outlives the handlers.)
- `bindDefaults()`: for each category with no current top, push the first enrolled
  node as default; its `emit` at build time (no cascade running) sets `isBound` AND
  `bindTime` directly via reflection (`set` thunk → `emitIsBound` / `emitBindTime`).
  **§23.3.1 case 1 ("during loading")**: bindTime must be written at load time; the
  clock value at the `buildSceneGraph` call is captured and written directly.
- `X3DNode* bound(const std::string& category) const` → that stack's top (null if none).

### 4. `X3DExecutionContext` integration + pull API

- Own a `BindingSystem bindings_`.
- `buildSceneGraph(scene)` also calls `bindings_.enrollScene(scene, *this);
  bindings_.bindDefaults(*this);` (after transform/bounds build).
- No `tick` change needed — `set_bind` flows through the existing cascade and invokes
  the wired handlers during `cascade_.process()`; the handler `postEvent`s isBound/
  bindTime into the same drain (they route onward and the dirty classifier records them
  as `DirtyField`).
- Pull API: `boundViewpoint()`, `boundNavigationInfo()`, `boundBackground()`,
  `boundFog()` convenience wrappers over `bindings_.bound(category)`.

### Scope boundaries

- **In:** category mapping; BindingStack (bind/move/unbind/default + isBound/bindTime);
  BindingSystem (enroll + handler wiring + defaults + bound query); context integration;
  pull API; tests. **The cascade-driven `set_bind` → bound-switch → isBound/bindTime
  path works end to end.**
- **Out / backlog (M2C-BIND-1):** per-Layer binding stacks (LayerSet/Layer give each
  layer independent stacks) — M2c uses one stack per category (the default layer). Trigger:
  LayerSet/Layer component implementation.
- **Not M2c (consumer/M2.5):** the *effect* of a binding — deriving the active camera
  pose from the bound Viewpoint, fog/background parameters for a renderer. M2c only
  maintains WHICH node is bound and fires the events.

## File structure

| File | Responsibility |
|------|----------------|
| `runtime/scene/BindingStack.hpp` (new) | one-category stack: bind/unbind/pushDefault/top with an Emit callback |
| `runtime/scene/tests/binding_stack_test.cpp` (new) | bind/move/unbind/default transitions + emit order |
| `runtime/scene/BindingSystem.hpp` (new) | category map, enroll + set_bind handler wiring, defaults, bound query |
| `runtime/scene/tests/binding_system_test.cpp` (new) | enroll two viewpoints, set_bind handler switches bound + isBound |
| `runtime/events/X3DExecutionContext.hpp` (modify) | own BindingSystem; build hook; pull API |
| `runtime/events/tests/m2c_tick_test.cpp` (new) | end-to-end: set_bind via cascade/tick switches the bound node + fires isBound |
| `CMakeLists.txt` (modify) | register 3 new test executables |

## Testing

1. **BindingStack:** push A (default, A bound) → bind B (A→false, B→true, B top) → bind A
   (B→false, A→true, A moved to top) → unbind A (A→false, B→true, B top) → unbind B (B→false,
   stack empty). Assert the exact emit sequence via a recording Emit.
2. **BindingSystem:** enroll two `Viewpoint`s; `bindDefaults` → `bound("Viewpoint")`==VP1,
   VP1.getIsBound()==true. Fire VP2's set_bind handler with TRUE → bound==VP2, VP2 isBound true,
   VP1 isBound false. Categories independent: a `NavigationInfo` and a `Viewpoint` don't interact.
3. **m2c_tick (end-to-end):** scene with VP1, VP2 under the root; `buildSceneGraph` →
   `boundViewpoint()`==VP1. `postEvent(VP2,"set_bind",SFBool(true))`, `tick` →
   `boundViewpoint()`==VP2 and VP2.getIsBound()==true, VP1.getIsBound()==false.
   `postEvent(VP2,"set_bind",SFBool(false))`, `tick` → bound back to VP1.

## Verification

- `mise run build` + `uv run pytest` green; **golden byte-identical**; M2a/M2b/event/animation
  tests unaffected (M2c only adds a build hook + cascade handlers; no `tick` reorder).

## Risks / watch-items

- **Handler lifetime:** the `set_bind` handler captures the context pointer; the context
  owns `bindings_` and outlives the handlers within a session — fine for the SDK lifecycle.
  Re-binding/clearing on scene teardown is a future concern (document, not block).
- **dynamic_cast coupling:** BindingSystem intentionally `dynamic_cast`s to the generated
  `X3DBindableNode`/`X3DViewpointNode`/`X3DBackgroundNode` — appropriate, since binding is
  defined precisely on those families (unlike the geometry-generic M2a/M2b).
- **postEvent during cascade drain:** the handler runs inside `cascade_.process()` (delivering
  set_bind) and posts isBound/bindTime into the same `pending_` queue — they drain in the same
  tick, matching how interpolator handlers chain. Verify with the m2c_tick test.
- **Default-bind emit has no cascade:** `bindDefaults` runs at build with no active cascade, so
  its Emit sets `isBound` via the reflection `set` thunk directly (not `postEvent`).
