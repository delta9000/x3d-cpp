# CONF-VIEWNAV — Viewpoint user-offset model (design)

**Date:** 2026-06-19  **Status:** approved (autonomous execution)
**Closes:** BIND-01…08 (the CONF-VIEWNAV cluster). CAVE-critical — see
[[x3d-cpp-gen-cave-consumer]].

## Problem

`NavigationSystem` mutates the bound Viewpoint's authored `position`/`orientation`
via `ctx.writeField` (BIND-01). ISO/IEC 19775-1 §23.2.3 is explicit: *"Navigation is
performed relative to the viewpoint's location and does **not** affect the location
and orientation values of an X3DViewpointNode."* The authored pose is the world-model
truth — in the CAVE it is synced from the master to every wall — so navigation must
never touch it. The browser must instead keep a separate **user offset**.

## Core principle

```
effectiveView = ancestorWorld ∘ T(authoredPos)·R(authoredOri) ∘ navOffset ∘ headPose
                └──────────── synced world model ───────────┘ └──── process-local ───┘
```

The authored pose (+ ancestor transforms) is the synced truth; navigation accumulates
a per-viewpoint `navOffset`; CAVE head-tracking supplies a per-process `headPose`. The
viewer's actual pose is the composition. This single idea resolves all of BIND-01…08.

## 1. State (process-local, NOT in the scene graph)

- **`ViewpointOffset`** — a rigid `{SFVec3f posOffset; SFRotation oriOffset}` in the
  viewpoint's local frame. Stored per-viewpoint in a runtime-owned map keyed by
  `X3DViewpointNode*` (per-viewpoint so `retainUserOffsets` and un-jump have a home).
  New header `runtime/events/ViewpointOffset.hpp`; the store lives on
  `X3DExecutionContext` (`offsets_`).
- **`HeadPose`** — a new process-local input seam `{SFVec3f position; SFRotation
  orientation; unsigned long revision}`, set by the consumer between ticks (mirrors
  `PointerState`/`KeyState`). One per context (the viewer's head). New header
  `runtime/events/HeadPose.hpp`; held on `X3DExecutionContext` (`head_`).

Both are deliberately outside the synced scene graph: the CAVE master broadcasts
`ancestor ∘ authoredPose`; each wall process owns its own `navOffset` + `headPose`.

## 2. Effective-view API (transparent recompute)

`X3DExecutionContext::viewMatrix()` becomes:

```
cam = worldOf(vp) · T(authoredPos) · R(authoredOri) · offsetMatrix(vp) · headMatrix()
view = cam.inverse()
```

`cameraWorldPosition()`/`cameraWorldUp()` derive from `viewMatrix()` (unchanged). So
`SceneExtractor`, `ViewDependentSystem`, and CAVE walls consume the **effective** view
with zero API change. `boundViewpoint()` still returns the authored node with pristine
fields. New API:
- `const ViewpointOffset &viewpointOffset(X3DNode *vp) const`
- `void setViewpointOffset(X3DNode *vp, const ViewpointOffset &)`
- `void setHeadPose(const SFVec3f &pos, const SFRotation &ori)` + `const HeadPose &headPose() const`

`offsetMatrix(vp)` = `T(posOffset)·R(oriOffset)`; `headMatrix()` = `T(head.pos)·R(head.ori)`
(identity when unset).

## 3. NavigationSystem rewrite (BIND-01, BIND-03)

EXAMINE/FLY/LOOKAT accumulate into `ctx.setViewpointOffset(vp, …)` instead of
`ctx.writeField(vp, "position"/"orientation")`. The cast becomes `X3DViewpointNode`
(not `Viewpoint`) so OrthoViewpoint/GeoViewpoint navigate (BIND-03). Examine orbits the
offset about the authored `centerOfRotation` (local-frame pivot); the authored fields
stay author-owned and CAVE-syncable. LOOKAT animates the offset toward the framing pose
(reusing the transition machinery, §4 below) rather than writing fields.

`centerOfRotation` is read (the orbit pivot) but not rewritten by EXAMINE; LOOKAT may
set it as the framed-object center (it is an `inputOutput` field whose authored value is
a navigation hint, §23.4.4) — documented as the one authored field LOOKAT updates.

## 4. Bind-time rules — `ViewpointBindSystem` (BIND-02/04/05/06/07/08)

A System applying §23.3.1 rules 5–9 to the **offset**, never the fields. It observes
Viewpoint `isBound` transitions (via the existing BindingSystem events) and the
`set_bind` edges.

- **BIND-08** stored relative transform: on push-down (a viewpoint leaving the top),
  store its current effective offset with it (already in `offsets_`); on pop, the next
  viewpoint's stored offset is reapplied for un-jump.
- **BIND-07** jump: on bind, `jump=TRUE` → the view snaps to the new authored pose
  (offset → identity unless retained); `jump=FALSE` → the new offset is chosen so the
  **effective camera is unchanged across the bind** — i.e. `newOffset =
  (worldOf(newVp)·T(pos)·R(ori))⁻¹ · oldEffectiveCam`, where `oldEffectiveCam` is the
  pre-bind effective camera matrix (`viewMatrix().inverse()`). Honors a `jump` value
  change before unbind (§23.3.1 final ¶ — un-jump follows the value at unbind time).
- **BIND-04** retainUserOffsets: `TRUE` keeps the viewpoint's stored offset on rebind;
  `FALSE` resets to the authored pose (offset → identity) on (re)bind.
- **BIND-02** navigationInfo: on bind, dispatch `set_bind TRUE` to the viewpoint's
  `navigationInfo` SFNode (if present) so its NavigationInfo becomes bound.
- **BIND-05** transition: `transitionType`/`transitionTime` animate the offset from its
  current value to the bind target over `transitionTime`; emit `transitionComplete` on
  the bound NavigationInfo when done (LINEAR/TELEPORT now; ANIMATE deferred per
  NAV-EXTRA). Generalizes the existing LOOKAT animation to every bind.
- **BIND-06** delete-detach: a `System::detach(X3DNode*)` virtual (default no-op; shared
  with ENV-06) lets a deleted bound node behave as `set_bind FALSE`. `BindingSystem`
  exposes a `removeNode(node)` that pops it from the stack with the proper isBound/
  bindTime events; the runtime calls it on node teardown.

## 5. CAVE fit

The master owns the authored pose (synced via the existing RenderItem/camera delta);
each wall process applies its local `navOffset` + `headPose` to derive its view, then
its own off-axis projection (BYO — the renderer's projection matrix, separate from the
view matrix). Head-tracking flows through `setHeadPose`. Because offsets/head are
process-local and the authored fields are pristine, walls cannot diverge on the world
model while still each rendering their own head-correct off-axis view.

## 6. Phasing (one spec, three implementable plans)

- **Phase 1** — `ViewpointOffset` + `HeadPose` state, effective-view recompute,
  NavigationSystem→offset, `setHeadPose` seam. **Closes BIND-01**; CAVE head-tracking
  works. (BIND-03 X3DViewpointNode cast folds in here — trivial.)
- **Phase 2** — `ViewpointBindSystem`: jump / retainUserOffsets / stored-relative-
  transform (BIND-04/07/08).
- **Phase 3** — navigationInfo dispatch (BIND-02), bind transitions generalized
  (BIND-05), delete-detach (BIND-06).

Each phase is golden-byte-identical (codegen-free), ctest- and conformance-gate-clean,
and closes its BIND findings in `docs/conformance/findings.yaml`.

## 7. Testing (per phase, TDD)

- Phase 1: navigation leaves authored `position`/`orientation` **byte-unchanged**;
  `viewMatrix()` reflects pose∘offset∘head; `setHeadPose` composes; OrthoViewpoint
  navigates (BIND-03).
- Phase 2: `jump=FALSE` bind keeps the world view continuous; `jump=TRUE` snaps;
  `retainUserOffsets` retains vs. resets on rebind; pop re-applies the stored relative
  transform (un-jump).
- Phase 3: bind dispatches `set_bind` to `navigationInfo`; a non-LOOKAT bind animates
  and emits `transitionComplete`; deleting a bound viewpoint pops the stack (isBound
  FALSE on it, TRUE on the next).

## 8. Non-goals

- Off-axis per-wall projection math + transport/genlock — BYO (CAVE infra), not the SDK.
- ANIMATE transition curve (spline ease) — deferred (NAV-EXTRA).
- WALK/collision — deferred (NAV-COLLISION); the offset model is collision-free.
- The head-pose → stereo eye separation — the consumer derives per-eye from `headPose`.
