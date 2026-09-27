---
title: H-Anim extraction
summary: Skin descriptors, pose deltas, CPU mesh access, and Segment placement.
---

# H-Anim extraction

The extractor follows `HAnimHumanoid.skeleton` for rigid geometry and emits
`HAnimHumanoid.skin` once in the humanoid's frame. The `joints`, `segments`,
`sites`, and `viewpoints` fields are references, so they add no visual placements.
This follows X3D v4 §26.3.2 and ISO/IEC 19774-1 §6.2.

Each skin item has an optional `RenderItem::SkinDesc`. It holds a shared
`hanim::SkinBinding`, a source coordinate index for every expanded mesh corner,
source normal indices when authored, and a pose version. A joint transform or
displacer weight change advances that version and places the item ID in
`RenderDelta.updatedSkinPose`. The geometry ID and mesh stay stable for a pose
change. Binding fields and skeleton structure cause the binding to be compiled
again.

CPU consumers call `SceneExtractor::deformedMesh(id)`. It evaluates the current
pose, deforms source coordinates, then maps them to expanded corners. Authored
binding normals are mapped by their normal indices; otherwise triangle normals
are generated using the geometry's winding and smoothing rules. The CPU raster
example calls this method when drawing a skin item. Segment meshes pass their
source points through `hanim::displaceSegmentPoints` before placement.

The deformation implementation in `HAnimSkinImpl.hpp` is currently a stub. It
returns the bind pose, so pose changes are observable through the descriptor and
delta while the visible mesh remains at rest. See [ADR-0055](../decisions/0055-hanim-skinning-descriptor.md)
for the core contract and space convention.
