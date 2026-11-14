---
title: Extraction delta contracts
summary: "What each kind of scene change costs and reports through RenderDelta: update cost, content identity, selection, structural and scoped replacement, visibility hints and geometry liveness."
tags: [subsystem, extract, render-delta]
updated: 2026-11-14
related:
  - extract.md
  - ../native-host-contract.md
  - ../../explanation/decisions/0069-selection-deltas-and-cheap-ticks.md
---

# Extraction delta contracts

The API these contracts describe is in [Extraction pipeline](extract.md); the
host-side rules are in the [native host contract](../native-host-contract.md).
Selection deltas are decided in
[ADR-0069](../../explanation/decisions/0069-selection-deltas-and-cheap-ticks.md).

## Scalar material update cost

- A scalar appearance-subtree edit refreshes each affected placement once per
  tick, even when several of its source nodes are dirty.
- It retains the reverse dependency index: only SFNode/MFNode replacements can
  change those edges, and those already use a structural baseline.
- This avoids rescanning a shared material's N-element placement vector once
  for each of N placements (#144).
- The performance regression uses a counted, non-rendered metadata child to
  prove scalar refresh does not walk unchanged dependency subtrees; structural
  replacement and subsequent scalar writes remain covered.

## Geometry content identity

- A host cache keyed by `GeomId` can upload each immutable AoS payload once.
- The extractor interns payload variants within each geometry owner: identical
  TextureTransform parameters share their cached mesh and key; different baked
  UVs or HAnimSegment deformation produce distinct content versions.
- The bake cache includes the raw deformation payload as an input, so equal
  texture transforms do not collapse different Segment positions (#137).
- Geometry invalidation evicts that owner's current payload lookup, then
  assigns fresh content identities.
- Versions are never reused inside a baseline, even when only one Segment's
  placement changes.
- `fullSnapshot()` resets them under the existing host replacement/eviction
  protocol.
- Exhausting the uint32 identity space throws with a request to take a fresh
  full snapshot rather than aliasing an old resource.
- Consumer-cache and shared-Segment regressions verify actual mesh bytes, not
  just distinct CPU pointers.

## View-dependent placements

- `delta()` checks recorded LOD selections in each placement's current world
  frame, including selections whose branch emits no mesh.
- Switch and LOD selection changes use a bounded active-scene walk retaining
  path IDs and immutable mesh caches.
- The add/remove lists contain the active-set difference; unaffected
  placements retain their IDs and mesh references.
- Dependencies are rebuilt for active paths, so nested selections and DEF/USE
  placements remain independent.
- Deselected geometry keeps its raw mesh cache as *dormant*, with its own
  source index: a `DirtyField` on the geometry or a direct source node evicts
  it and advances its content version, so later reactivation reads current
  fields.
- An unedited level comes back with the same `GeomId` and mesh reference, so a
  viewer sitting on an LOD range boundary does not rebuild or re-upload on
  each crossing.
- HAnimSegment-displaced variants are still evicted on deselection.
- Scoped light/fog index changes republish only records whose indices change;
  surviving external packed items receive geometry updates because the
  resolver is called again.
- The walk also re-derives every surviving record's material, textures, shader
  and skin binding: a record whose payload differs (for example, a texture
  that finished resolving) is listed in `updatedMaterial` (`updatedSkinPose`
  for the skin binding).
- See
  [ADR-0069](../../explanation/decisions/0069-selection-deltas-and-cheap-ticks.md).
- The node-level `level_changed` event alone cannot represent all USE
  placements.
- Billboard descendants recompute their per-path transforms from the current
  tracked eye/up and authored ancestor transforms.
- A camera-only change emits `updatedTransform` only when the resulting matrix
  changes and retains immutable mesh payloads.
- No-motion ticks do not add Billboard transform updates.
- Geometry that becomes nonempty again also recomputes its current path frame
  before its addition is published, including tracked motion while it was
  absent.
- Regression: `runtime/extract/tests/scene_extractor_view_delta_test.cpp`
  (#136).

## Structural delta replacement contract

- Any `DirtyChildren` (SFNode/MFNode writes, `visible` changes, and
  `LayerSet.order` edits), a scalar `HAnimHumanoid` configuration edit, or a
  scene topology revision replaces the extraction baseline once.
- Switch/LOD selection uses `DirtyActiveChild` and the retained-path walk
  described under [View-dependent placements](#view-dependent-placements);
  structural edits in the same tick take precedence: `removed` contains every
  previously live ID and `added` contains the complete current snapshot.
- Apply removals before additions; dense IDs can occur in both.
- Release their live content-cache entries before accepting new records;
  actual GPU destruction remains fence-controlled by the consumer.
- IDs, borrowed references and content-version counters must not be treated as
  persistent across this boundary.
- The snapshot resets caches and content versions.
- The full walk uses the existing traversal budget and may report a partial
  view through `budgetExceeded()`.
- It intentionally costs O(visited scene paths) plus mesh/material extraction,
  including unchanged placements, on structural ticks.
- Scalar geometry/material/TRS ticks retain the incremental and shared-mesh
  paths except for humanoid configuration and the descriptor dependencies
  described below.
- This avoids stale dependencies after geometry replacement, forgotten
  reattachment of a removed path, last-writer-only USE-group placement
  rebuilding, and walking raw path pointers after a removed subtree has
  already been destroyed.

`fullSnapshot()` called directly is also authoritative replacement, but returns
only `added`; the caller clears its previous state. `delta()` twice without a
new tick is empty. Consumers must consume each tick or explicitly rebaseline;
there is no retained history of missed ticks.

## Scoped render-state replacement contract

`Shape.castShadow`, `ClipPlane` fields and `LocalFog` fields do not have a dedicated
per-item update bucket in `RenderDelta`. Changes to their cached dependencies take
the same remove-all-before-add-all replacement path. `visible` is active
traversal state and is classified as `DirtyChildren`, including an initially hidden
branch that has no emitted item or reverse item dependency yet.

- The dependency sets include Shape records (also recognized-empty geometry),
  every encountered ClipPlane and LocalFog before testing `enabled`, and all
  static Transform ancestors of each scoped descriptor placement.
- Ancestor TRS changes therefore refresh world-space planes and world-scaled
  fog ranges, including every USE placement.
- Dependencies are rebuilt on each replacement.
- They are collected by the existing clip/fog walks, not by an additional
  per-tick scene traversal.
- For ClipPlanes below a Billboard, the extractor also retains the
  per-placement scope frame (including disabled planes and scopes without
  items).
- Each tick compares only those stored paths against the current tracked
  eye/up; a changed frame replaces the baseline.
- Billboard scalar edits participate in the scoped node dependencies too.
- This adds O(recorded view-dependent clip-scope paths × path depth) checking;
  stable views and scenes without those scopes do not gain replacement walks.
- Unrelated transforms, including a Shape's frame below an already established
  clip/fog scope, remain incremental and retain their mesh allocations.
- Dirty tracking is node-granular: any `DirtyField` on an indexed Shape or
  scoped source/frame conservatively replaces the snapshot.
- A scoped frame's local/world transform dirtiness does too, even for disabled
  descriptors or empty scopes.
- This deliberately costs a bounded full walk and re-extraction of unchanged
  content on those ticks; it is a correctness fallback, not an
  O(changed-items) claim.
- Replacement refreshes `snapshotLocalFogs()` and item indices together and
  sets `fogChanged`.
- Consumers must not interpret a plain `updatedTransform` or `updatedMaterial`
  as permission to refresh unrelated clip or shadow descriptors.
- `scene_extractor_state_delta_test.cpp` exercises posted events through
  `tick()` and compares a channel-respecting consumer mirror against a
  separate fresh extractor: visibility, shadow flags, clip edits and frame
  changes, local fog color/type/range/enabled and scale, disabled/empty/hidden
  scopes, shared placements, unrelated TRS, coalesced edits, and
  camera-only/shared-empty Billboard clip scopes with independent expected
  world-plane values.
- Shared enclosing LocalFog groups also have explicit expected per-placement
  ranges before and after ancestor scale changes.
- This establishes delta/snapshot consistency and those snapshot semantics.
- The OpenGL example clips with these planes (`clip_plane_gl_test.py`) and
  fogs each item with its tagged LocalFog (`local_fog_gl_test.py`).

## Optional visibility-limit hints

- `RenderItem::beyondVisibilityLimit` is an origin-distance hint, for both AoS
  and packed placements: distance from the current world origin to the tracked
  eye is strictly greater than positive `Viewpoint.farDistance`, or, if that
  is not positive, positive `NavigationInfo.visibilityLimit`.
- Nonpositive effective limits leave the hint false; equality is false.
- This is neither a bounds test nor a frustum/occlusion guarantee.
- Geometry is still emitted and hosts may ignore it.
- The hint has no narrow `RenderDelta` channel.
- Before consuming a tick, the extractor compares relevant live hints with
  their current values.
- An actual boolean change uses the existing bounded remove-all-before-add-all
  replacement.
- A host must apply removals before additions and read the complete newly
  added records; plain transform/material/camera notifications do not grant
  unrelated descriptor updates.
- This catches far/fallback changes (including binding switches and
  finite-to-unlimited clearing), eye motion and ancestor TRS.
- Added/revived geometry gets its current hint even when the inputs changed
  while that placement was empty.
- Packed emission now uses this same rule instead of always initializing the
  hint to false.
- Unlimited-to-unlimited ticks do no per-item hint checking.
- With a finite limit, changed eye/up/limit inputs require O(live items)
  distance comparisons; static placements reuse their stored world origins.
- Dirty-frame and Billboard placements instead recompose current paths,
  sharing a local-matrix cache (O(affected path lengths)).
- Stable inputs inspect only dirty-frame dependencies; Billboard scalar edits
  also check the recorded Billboard placements.
- No hint change means no extra replacement or mesh allocation, including
  ordinary tracking ticks.
- A crossing does cost a bounded full scene walk and re-extraction of
  unchanged meshes, just like other unsupported per-item descriptor updates;
  this is an explicit correctness fallback, not a fine-grained state channel
  or a replacement on every head update.

`scene_extractor_visibility_delta_test.cpp` checks explicit expected hint values
and a channel-respecting mirror against independent snapshots, including shared
placements, disabled/fallback limits, view motion, bindings, TRS, empty activation
and packed geometry. Billboard transform/LOD refresh remains the separate
view-dependent extraction contract; hint checks use current Billboard frames.

## Incremental geometry ownership and liveness

- A Coordinate, Normal, Color or other direct content child can feed several
  geometry owners.
- Each dirty source resolves to all of those owners, and multiple dirty
  sources coalesce into one cache invalidation and content-version advance per
  owner per tick.
- Each owner's placements rebuild from that owner's primitive type; placements
  with identical geometry and bake parameters continue to share immutable mesh
  storage.
- Unrelated meshes are retained.
- Scoped HAnimSegment displacers still affect only placements within their
  Segment, even when the geometry owner is shared.
- Recognized-empty AoS geometry keeps its source dependencies and placement
  paths.
- Becoming nonempty emits `added`; becoming empty emits `removed`, and
  replaces the stored mesh with the empty result rather than retaining stale
  drawable content.
- An already allocated placement reuses its ID when it becomes nonempty again.
- Initially empty placements allocate no ID until their first nonempty result.
- Transform/material edits while dormant are reflected when a placement
  returns, and update buckets never target removed or newly added placements.
- This path visits only affected geometry owners/placements, not the entire
  scene.
- `itemCount()` counts allocated dense slots, which may include dormant
  removed items; consumers maintain the live set from `added` and `removed`.
- Empty/nonempty transitions are content changes, so they do not reset the
  baseline.
- The structural replacement contract above still applies when node-valued
  fields change.
- These statements concern native AoS geometry; packed resolver retry/liveness
  is a separate seam.
