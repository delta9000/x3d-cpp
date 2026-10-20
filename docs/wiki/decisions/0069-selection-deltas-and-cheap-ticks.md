---
title: "ADR-0069: Selection deltas and cheap ticks"
summary: Retain placement identity across LOD/Switch selection, separate local bounds from world motion, and notify actual light changes.
tags: [adr, runtime, extraction, performance]
updated: 2026-10-18
related:
  - ../guides/native-host.md
  - ../subsystems/extract.md
  - ../subsystems/dirty-bounds-transform.md
---

# ADR-0069: Selection deltas and cheap ticks

Proposed; renumber on merge. Keep the existing host loop:

```cpp
session.tick(now);
auto changes = session.delta();
```

Previously, a Switch `whichChoice` change or a per-path LOD level change removed
and added the whole extraction baseline. Publish the active placement difference
instead, retaining IDs and immutable meshes for unaffected paths. A first
`level_changed` announcement with unchanged selection publishes no membership
change. Record empty selections too, so a previously empty branch can activate.

Separate `DirtyActiveChild` from `DirtyChildren`: selection changes preserve
node-reference lifetimes and scene indexes. SFNode/MFNode edits, visibility,
scene topology revisions, and existing descriptor invalidation rules keep the
conservative replacement path. Those edits take precedence within a mixed tick.
Consumers must still apply removals before additions and consume every tick.

Reuse the bounded active-scene traversal, retaining its path index and immutable
mesh caches while rebuilding active reverse dependencies. This handles nested
selections and shared nodes under different parents without the old subtree
helper's single cached entry frame. It costs O(visited active scene paths) on a
selection tick; this proposal does not promise subtree-only CPU work. Mesh
caches that lose their active dependencies stay cached as dormant under their own
source index; a field edit on a dormant geometry or its direct source evicts it
and advances its content identity, so inactive edits cannot revive stale bytes
under an old `GeomId`. An unedited level returns with its previous `GeomId` and
mesh, so an LOD boundary crossing does not rebuild meshes. (The first version of
this proposal evicted on every deselection, which rebuilt and re-versioned the
mesh on each crossing; the review of 2026-10-19 found the retention claim held
only for placements that stayed active.) HAnimSegment-displaced variants are
still evicted on deselection.

The walk re-emits each surviving record's material, textures, shader and skin
binding. Compare them with the published values and list changed records in
`updatedMaterial` (`updatedSkinPose` for a skin binding), so a texture that
resolved during a selection tick is not changed silently.

Scoped light/fog indices can shift when branches change. Republish records whose
indices change with remove-before-add under their retained ID. External packed
geometry is re-resolved by this traversal and receives a geometry update even
when its path survives. Direct `fullSnapshot()` retains its authoritative reset
semantics. Snapshot descriptor references remain borrowed until the next full
snapshot or selection refresh.

Compare complete world-resolved `LightDesc` lists for `lightsChanged`. Record
light paths, including disabled lights, to avoid light walks for unrelated
animation. The light collection records them itself (`LightDependencies`), so
they match the graph it reads: subtrees the geometry walk skips (`visible=FALSE`,
`Collision.proxy`, a GeoLOD `rootNode`) are covered, as is each LOD choice there
that follows the viewer. Initial full snapshots set the bit; replacement deltas compare against
the last published values. Camera/background/fog bits remain conservative. Scalar
light edits retain the existing snapshot-index contract.

The CAVEOS probe measured about 32 ms per runtime tick on `bound_500.x3d`, with
8032 descendant Transform dirty entries and only 20 extracted placements. Source
inspection finds that ancestor motion recomposed every descendant's unchanged
local TRS, then bounds propagation treated world-only dirtiness as local-bound
invalidation and scanned child fields. Cache local transform matrices in the
index; recompute them on authored local writes. Invalidate only parent unions
for local TRS motion and ignore world-only bounds seeds. Keep world propagation
through inactive levels, preserving runtime queries. Skip environmental-sensor
reachability work when no such sensors are attached.

The scene routes TimeSensors through `temple-position` into
`lod-position.translation`; its text animations change material colors. LOD
range checks and routed event ordering remain intact. No speedup has been
measured for this proposal. `x3d_runtime_tick_profile FILE N_TICKS` separates
systems/cascade, post-cascade hooks, transforms, bounds, and extraction at 30 Hz
from zero, with a fixed viewer. Optional
`X3DExecutionContext::tick(now, TickTimings*)` enables clock reads only for
profiling; existing one-argument calls keep their form.

Regression cases in `scene_extractor_selection_delta_test.cpp`, registered in
`x3d_extract_tests`, cover stable unrelated IDs, shared LOD paths, empty Switch
selection/reactivation, inactive geometry edits, light notifications, and local
matrix/bounds work. They have not been built or run. CAVEOS's unchanged-light
comparison in `x3d_vsg.cpp:315-322` can be removed after integration verification;
its generic structural-rebaseline lifetime handling remains necessary.
