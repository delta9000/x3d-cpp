---
title: "ADR-0066: Native SAI presentation materials and frame deltas"
summary: Carry extracted materials and compare retained presentation frames by placement key.
tags: [adr, sai, provider, extraction]
updated: 2026-10-18
related:
  - ../subsystems/sai-provider.md
---

# ADR-0063: Native SAI presentation materials and frame deltas

Proposed. CAVEOS currently invents a material for every native SAI placement and
diffs snapshot placement keys, mesh identities, and matrices in its host adapter.
The provider can supply both without adding a frame queue or changing portable
SAI authority.

Append the extracted `MaterialDesc` and an opaque `material_revision` to each
`presentation_item`. Copy the descriptor by value, using its existing deep-copy
support. The four-node slice cannot author Appearance, so its published material
is the extractor's unlit-white fallback. This does not add material authoring.

Add `changes_between(previous, current)` as a native value operation. A null
previous frame produces only additions. Otherwise require matching opaque scene
identities and nondecreasing native ticks. Compare placement keys, exact world
matrices, mesh ownership/keys, and material revisions. Return added, removed,
transform, geometry, and material placement-key buckets; update buckets exclude
additions/removals and may overlap each other. Read their payloads from the
current complete frame. Bucket ordering is unspecified.

Material revisions advance on extractor `added` or `updatedMaterial` entries.
A structural rebuild can therefore invalidate an unchanged material on a
surviving placement. This conservative policy avoids a second descriptor
equality implementation and preserves extractor invalidations across skipped
ticks. Revisions are scene-local invalidation identities, not content hashes.
Geometry comparisons retain the existing mesh-identity semantics; equivalent
rebuilt meshes can require another upload. Reverted intermediate transforms
produce no net transform update.

Keep `snapshot()` and producer retention unchanged: one current frame and current
identity maps. Hosts choose how long to retain previous frames. Comparisons need
no live service, and no callback, native node authority, or frame history enters
the portable interface. Existing callers keep their source calls; the item layout
changes in this experimental package and requires a rebuild.

The existing `sai_provider_presentation` executable includes the focused
`materials_and_frame_deltas` case. Build and test execution remain pending for
this proposal.
