# Viewnav residuals — BIND-09, NAV-LOOKAT-SCALE, NAV-FLY-ROLL (design)

**Date:** 2026-06-21  **Status:** approved (autonomous execution)
**Closes:** the three deferred minors remaining after the CONF-VIEWNAV cluster
shipped (`docs/conformance/findings.yaml` lines 978-999). Predecessor design:
[[2026-06-19-conf-viewnav-user-offset-design]].

## Problem

Three conformance findings survived the CONF-VIEWNAV ship either because they
were out of that cluster's scope (BIND-09) or because they pre-dated the
offset model and were deferred without root-causing (NAV-LOOKAT-SCALE,
NAV-FLY-ROLL). All three are low/minor severity; the per-finding overhead of a
separate design + plan + PR is disproportionate, so they are bundled here as a
single viewnav-residuals workflow with one conformance-view regeneration at the
end.

### BIND-09 — pop does not apply §23.3.1 r6.3 un-jump
`ViewpointBindSystem::onPostCascade` (`runtime/events/ViewpointBindSystem.hpp:27`)
detects a bind change by polling `ctx.boundViewpoint()` and comparing to
`lastVp_`. It cannot distinguish a push (`set_bind TRUE` on a non-bound vp) from
a pop (`set_bind FALSE` on the bound vp, or deletion of the bound vp) — both
change the bound node A→B. `onBind` then applies the `jump`/`retain` decision
tree (`ViewpointBindSystem.hpp:86`), so on a pop the popped-to B, if
`jump=TRUE, retain=FALSE` (the defaults), has its offset **reset to zero**,
discarding B's stored relative transform. §23.3.1 r6.3 says a pop must **restore
the popped-to vp's stored offset**, ignoring its `jump`/`retain`.

### NAV-LOOKAT-SCALE — non-uniform ancestor scale mis-sizes LOOKAT framing?
Audit note (line 991): *"LOOKAT framing distance mixes a world-space radius with
a local-frame eye placement, so a non-uniformly-scaled ancestor Transform
mis-sizes the framed object."* The audit is labelled "Pre-existing" without root
cause. Code trace of `beginLookat` (`NavigationSystem.hpp:297-324`):

- `radius` is computed from `ctx.worldBounds(pick.node)` — world-space. ✓
- `dir` is `center − camWorld` — both world-space. ✓
- `d = radius / tan(fov/2)` — world-space distance. ✓
- `targetWorld = center − dir·d` — world-space camera target. ✓
- `targetLocal = parentInv.transformPoint(targetWorld)` — round-trip into
  viewpoint-local frame. `parentInv` is `ctx.worldOf(vp).inverse()`, the true
  world-to-local map; `transformPoint` is affine-correct for non-uniform scale.

The composition `worldOf(vp) * targetLocal == targetWorld` holds (lossless
round-trip), so the world camera ends up at `targetWorld`, distance `d` from the
world center. **The math looks correct.** The fix is therefore: **write a
verification test first; if it passes, mark the finding INVALID (RTC-8 style);
if it fails, root-cause from the failing assertion and apply the audit's
suggested fix** (compute framing distance in local frame, or scale by ancestor
factor).

### NAV-FLY-ROLL — FLY accumulates roll on long mixed drags
`flyUpdate` (`NavigationSystem.hpp:277`):
```cpp
q = quatMul(quatMul(Ryaw, Rpitch), q);   // incremental composition
```
Each frame's `Rpitch` is about the *current* local right (already rotated by
the accumulated `q`), so any yaw not exactly about world-up, or any cross-axis
coupling, drifts roll into `q`. Over a long mixed drag the horizon tilts.
§23.4.4: FLY is strictly yaw + pitch, no roll.

## Approach

**Approach A (chosen):** one design doc, one plan, three TDD sub-tasks, one
conformance-view regen at the end. NAV-LOOKAT-SCALE is handled as "write the
verification test first" — if it passes, mark INVALID and skip the fix; if it
fails, fix it. This collapses the alternative "drop LOOKAT-SCALE upfront" (C)
into the bundle as a contingency gate, and avoids the three-PR overhead (B) for
three low/minor items.

## 1. NAV-LOOKAT-SCALE verification (Phase 1)

### Construct
- A `Transform` with non-uniform `scale=(2, 0.5, 1)` parenting a `Box` of size
  `(1,1,1)`. World AABB half-diagonal = `(1, 0.25, 0.5)`, world radius
  `≈ 1.146`.
- A `Viewpoint` positioned to pick the box.
- Dispatch LOOKAT (button-down edge with a ray hitting the box).

### Assertions
1. The world camera position after LOOKAT completes is at world distance
   `d = radius / tan(fov/2)` from the box's world center, within `1e-3`.
2. The box's world AABB fits inside the view frustum at that distance
   (conservative sphere-contains-box check).

### Outcome
- **Both pass** → finding is INVALID. Mark `status: invalid` in
  `findings.yaml` with verification commit + ctest name (mirrors RTC-8).
- **Either fails** → root-cause from the failing assertion and apply the
  audit's suggested fix (compute framing distance in local frame, or scale
  `d` by the relevant ancestor factor).

### Test location
`runtime/events/tests/navigation_test.cpp`. New ctest case
`x3d_navigation_lookat_scale`.

## 2. BIND-09 push-vs-pop signal + pop restore-stored-offset (Phase 2)

### Signal — decoupled side-channel on `X3DExecutionContext`
The two systems already communicate only through `ctx` (BindingSystem posts via
`poster_`; ViewpointBindSystem polls `boundViewpoint()`). Add one small
accessor:

```cpp
// X3DExecutionContext.hpp
enum class BindTransition { None, Push, Pop };
BindTransition lastViewpointBindTransition() const { return lastVpTransition_; }
void setLastViewpointBindTransition(BindTransition k) { lastVpTransition_ = k; }
```

`BindingSystem`'s emit lambdas (`BindingSystem.hpp:64-67` and `:96-100`) set
this when emitting a viewpoint transition:
- `BindingStack::bind` (set_bind TRUE on a non-top node) → `Push`
- `BindingStack::unbind` of the top (set_bind FALSE on the bound, or delete of
  the bound) → `Pop`
- `BindingStack::unbind` of a non-top (set_bind FALSE on a stacked-but-not-bound
  node) → no transition; `None`
- `BindingStack::pushDefault` (startup) → `Push`

The emit signature itself stays the same; `BindingSystem` writes the kind based
on which `BindingStack` method it called.

### Fix in `onBind`
Pass `BindTransition kind` through `onPostCascade`→`onBind`:

```cpp
void onBind(X3DExecutionContext &ctx, X3DNode *vp, BindTransition kind) {
  if (!vp) return;
  const bool jump   = geombounds::getField<bool>(*vp, "jump", true);
  const bool retain = geombounds::getField<bool>(*vp, "retainUserOffsets", false);

  if (kind == BindTransition::Pop) {
    // §23.3.1 r6.3: pop RESTORES the popped-to vp's stored offset.
    // (No-op here: the offset store already holds it — leave it alone.)
  } else if (!jump) {
    if (haveCam_) driveCameraTo(ctx, vp, lastCam_); // continuous
  } else if (!retain) {
    ctx.setViewpointOffset(vp, ViewpointOffset{}); // snap to authored pose
  } // retain=TRUE: keep stored offset

  // ... BIND-05 transition animation unchanged (a pop still animates) ...
}
```

Reset `lastVpTransition_ = None` at end of `onPostCascade` so it can't leak
into a later cycle.

### Tests (`runtime/events/tests/viewpoint_bind_test.cpp`)
1. **POP restores stored offset.** Stack [A, B] (B bound on top), both
   `jump=TRUE, retain=FALSE`. Capture A's stored offset at setup (after A had a
   user pan). Post `set_bind FALSE` on B. Assert A becomes bound AND A's offset
   equals the captured value (not zero).
2. **PUSH still resets (negative control).** Same stack [A, B], B bound. Post
   `set_bind TRUE` on A. Assert A becomes bound AND A's offset is zero (snap to
   authored pose — confirms push path still applies jump/retain).
3. **Pop animates over transitionTime.** Stack [A, B], B bound. Set B's
   `navigationInfo.transitionTime=0.5`. Post `set_bind FALSE` on B. Assert: at
   t=0 the camera is at A's stored offset; at t=0.5 it has reached the target;
   `transitionComplete` fired on the named nav.

### Non-goal
`BindTransition` is currently only the kind. If a future finding needs prev/now
node pointers, extend then.

## 3. NAV-FLY-ROLL yaw/pitch decompose (Phase 3)

### Fix
Don't accumulate the quaternion directly. Decompose the *desired* view into yaw
θ (about world-up) and pitch φ (about local-right-after-yaw), reconstruct `q`
each step from those two scalars:

```cpp
// flyUpdate — new persistent state: yaw_, pitch_ (members of NavigationSystem).
// Initialize lazily from the current effective orientation on first FLY tick or
// after a non-FLY→FLY mode switch (so we don't snap on entry).
if (!flyOrientValid_) {
  decomposeLookRotation(effOri(ctx, vp), up, yaw_, pitch_);
  flyOrientValid_ = true;
}

if (dragging && (dx != 0.0f || dy != 0.0f)) {
  yaw_   += -dx * kRotScale;
  pitch_ += -dy * kRotScale;
  // Clamp pitch to ±(π/2 − ε) so we don't flip past vertical.
  pitch_ = std::clamp(pitch_, -kHalfPi + 1e-3f, kHalfPi - 1e-3f);
  changed = true;
}

// Reconstruct orientation: yaw about world-up, then pitch about local right.
Quat Ryaw   = axisAngleQuat(up, yaw_);
SFVec3f right = norm(rotateByQuat(Ryaw, SFVec3f{1,0,0}));
Quat Rpitch = axisAngleQuat(right, pitch_);
q = quatMul(Rpitch, Ryaw);
```

The translation keys (lines 286-290) still move along `q`-rotated axes —
unchanged.

### Decompose helper
Given the current effective orientation as a quaternion (looking along `-Z`
rotated by q), recover yaw/pitch:
- `forward = rotateByQuat(q, {0,0,-1})`
- `yaw_ = atan2(-forward.x, -forward.z)` (heading in world XZ plane; sign
  convention matching `Ryaw = axisAngleQuat(up, -dx*...)`)
- `pitch_ = asin(clamp(forward.y, -1, 1))` (positive when looking up)
- Roll is discarded by construction (the reconstructed `q` has zero roll).

Add as a private static helper
`decomposeLookRotation(SFRotation ori, SFVec3f up, float &yaw, float &pitch)`.

### State lifetime
`flyOrientValid_` reset to `false` whenever:
- Mode switches from non-FLY to FLY (in the existing `mode(nav)` dispatch,
  `NavigationSystem.hpp:133` area).
- A viewpoint bind happens (the bound vp changed → re-decompose from the new
  effective orientation next FLY tick). Easiest: clear the flag in
  `NavigationSystem` if `ctx.boundViewpoint()` differs from a cached
  `lastFlyVp_`.

This avoids stale yaw/pitch carrying across viewpoint jumps.

### Tests (`runtime/events/tests/navigation_test.cpp`)
1. **No roll after long mixed drag.** FLY mode, drag pattern: yaw +90°, pitch
   +30°, yaw −90°, pitch −30° (a closed-loop mixed sequence). At the end, assert
   the roll component of the effective orientation is `< 1e-4` (i.e. the
   world-up vector, rotated by q, has x and z components `< 1e-4`).
2. **Pitch clamps at vertical.** FLY mode, drag with `dy` large enough to pitch
   past +90°. Assert final pitch is clamped to `π/2 − ε` (effective forward.y
   ≈ 1, no flip).
3. **Mode switch re-decomposes.** EXAMINE→FLY transition with a non-zero roll
   induced by EXAMINE orbit (orbit can roll if the up vector was tilted). On
   entering FLY, assert the first FLY frame has zero roll (decompose discards
   the EXAMINE-induced roll).
4. **Viewpoint bind re-decomposes.** Mid-FLY, bind a different viewpoint (which
   resets the camera). Assert next FLY tick re-derives yaw/pitch from the new
   orientation (no snap, no stale roll carry-over).

### Non-goal
Banking/roll-input support. Some flight-sim variants want roll on a third axis;
X3D FLY is strictly roll-free per §23.4.4 ("only yaw and pitch"). No roll input
is added.

## Phasing

Three independent phases, each its own commit (mirrors the CONF-VIEWNAV
cadence):

| Phase | Findings | Touches | Closure signal |
|---|---|---|---|
| **P1: NAV-LOOKAT-SCALE verification** | NAV-LOOKAT-SCALE | test-only | If test passes → `status: invalid` (with verification commit + ctest name, RTC-8 style). If fails → root-cause + fix, then `status: closed`. |
| **P2: BIND-09 pop-restore** | BIND-09 | `X3DExecutionContext.hpp` (BindTransition accessor), `BindingSystem.hpp` (emit sets kind), `BindingStack.hpp` (pass kind out of `bind`/`unbind`/`pushDefault`), `ViewpointBindSystem.hpp` (onBind branches on kind), 3 new tests | `status: closed` with commit SHA |
| **P3: NAV-FLY-ROLL decompose** | NAV-FLY-ROLL | `NavigationSystem.hpp` (yaw_/pitch_/valid_ members, decompose helper, rewrite `flyUpdate` drag block, clear flag on mode/bind switch), 4 new tests | `status: closed` with commit SHA |

### Ordering rationale
P1 first because its outcome (invalid vs. real bug) might inflate P3's scope —
if LOOKAT is actually broken and the fix touches the same `NavigationSystem.hpp`
region, P3 wants to know first. P2 second because it's isolated to the bind
pipeline (no overlap with nav math). P3 last because it's the most invasive nav
change and benefits from P1/P2's green baseline.

## Test plan (cumulative)

- **Existing:** `ctest` full suite (no regressions in `viewpoint_bind_test`,
  `viewpoint_offset_test`, `navigation_test`, `binding_stack_test` if present).
- **P1 new:** `x3d_navigation_lookat_scale` — non-uniform ancestor Transform +
  Box + LOOKAT; assert world camera distance = `radius/tan(fov/2)` and box AABB
  fits frustum.
- **P2 new:** `x3d_viewpoint_bind_pop_restores_offset` (3 cases listed in §2).
- **P3 new:** `x3d_navigation_fly_no_roll_after_mixed_drag`,
  `x3d_navigation_fly_pitch_clamp`,
  `x3d_navigation_fly_redecompose_on_mode_switch`,
  `x3d_navigation_fly_redecompose_on_bind`.
- **Conformance regen:** `mise run conformance` after P3 (single regen, not
  per-phase — matches Approach A's bundling).
- **Wiki:** `docs/wiki/subsystems/system-viewpointbind.md` updated only if P2
  changes user-observable bind behavior (it does — pop now restores). One
  paragraph addendum.

## Non-goals

- CAVE/exercise coverage of viewpoint pop stacks (CAVE doesn't exercise them;
  the fix ships without changing that).
- Roll-input banking for FLY (out of scope; §23.4.4 forbids).
- WALK/LOOKAT navigation modes' orientation handling (only FLY).
- Cleaning the stale `BACKLOG.md` line 474 row — separate cleanup task, not
  bundled.
- Adding a `BindTransition` enum field for prev/now node pointers (YAGNI; only
  the kind is needed).
- Refactoring `BindingStack::Emit` signature — kept stable; kind flows through
  `ctx`.

## Verification before completion

Each phase: `mise run build` + `ctest --output-on-failure` + targeted new
tests. Final phase: `mise run conformance` regenerated, `git diff
docs/conformance/findings.yaml` shows the 3 findings flipped to `closed` (or
`invalid` for P1), `git status` clean. Only then commit.
