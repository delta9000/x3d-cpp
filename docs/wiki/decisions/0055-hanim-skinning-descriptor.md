---
title: "ADR-0055: H-Anim Skinning Through a Descriptor With a Reference CPU Skinner"
summary: A humanoid's skin compiles into an immutable SkinBinding (bind positions/normals, variable-width per-vertex joint influences, inverse bind matrices, Joint displacers); each tick a SkinPose palette is evaluated from the live skeleton. Extraction publishes the binding and a palette-only delta for GPU renderers and exposes the reference CPU skinner (the one implementation of the math) for everyone else. Bind pose follows HAnim v2 binding fields when authored and the v1 rest pose otherwise. The SDK never writes deformed coordinates back into skinCoord.
tags: [adr, hanim, skinning, extraction]
updated: 2026-09-27
related:
  - ../subsystems/extract.md
  - 0045-shared-mesh-instancing.md
---

# ADR-0055: H-Anim Skinning Through a Descriptor With a Reference CPU Skinner

## Status

Accepted

## Context

X3D §26 delegates H-Anim semantics to ISO/IEC 19774. Until now only the
skeleton worked: HAnimHumanoid and HAnimJoint are transforms, so rigid segment
geometry follows the joints. Skin deformation (HAN-1), HAnimDisplacer (HAN-2,
HANIM-DISP) and HAnimMotion (HAN-3) were not implemented.

The Web3D example archive has 96 humanoid scenes; 22 carry a skin
(`skinCoord`), from BoxMan (224 vertices, 17 joints, two influences per vertex)
to Gramps (203,880 vertices, 66 joints, up to 15 influences per vertex). No
archive scene uses the HAnim v2 binding fields (`skinBindingCoords`,
`jointBinding*`).

Three places skinning could happen:

- **A. CPU skinning in the SDK**, publishing a deformed mesh every frame. Simple
  for consumers, but each animated frame rebuilds and re-uploads the expanded
  mesh (hundreds of MB/s for the large archive skins at 60 Hz).
- **B. A descriptor only** (bind mesh, influences, palette) for GPU skinning.
  Cheap per frame, but every consumer must implement skinning and the CPU
  raster example would stay static.
- **C. Both**: the descriptor plus a reference CPU skinner.

## Decision

Option C.

**Contract.** `runtime/hanim/HAnimSkin.hpp` defines it:
- `SkinBinding` is compiled once per humanoid. It holds bind positions and
  normals, a CSR list of per-vertex influences (variable width, never capped at
  four, weights normalized per vertex), inverse bind matrices per joint in
  `HAnimHumanoid.joints` order, and the Joint displacers.
- `SkinPose` is evaluated per tick: the palette `D_j = C_j · B_j⁻¹`, the joint
  matrices `C_j`, and the current displacer weights, all humanoid-local.
- `deform()` is the reference CPU skinner and the one implementation of the
  mathematics. Positions blend by the palette, normals by its inverse
  transpose and are then normalized, and Joint displacers are added after
  skinning along the owning joint's axes. Unweighted vertices keep their bind
  position.

**Bind pose.** When `skinBindingCoords` / `skinBindingNormals` / `jointBinding*`
are authored (HAnim v2, non-BASIC), they define it. Otherwise it is the authored
`skinCoord` / `skinNormal` with every joint at rest (HAnim v1 and BASIC: "all
the joint angles shall be zero"), so `B_j` is the identity and the palette is
each joint's current humanoid-local matrix.

**No write-back.** The SDK never writes deformed positions into
`skinCoord.point`. `skinCoord` is the bind source for v1 content; writing into
it would feed each frame's output into the next and disturb every other user
of that Coordinate. This also settles the policy question recorded in
HANIM-DISP: Segment displacers likewise deform the Segment's mesh at
extraction, not its authored Coordinate.

**Extraction.**
- Skin Shapes carry a reference to their humanoid's binding plus, per emitted
  corner, the source coordinate index. That index is the remap the mesh
  builder previously discarded.
- A pose-only change is reported in a new `RenderDelta` bucket (the item ids
  whose palette changed), so GPU consumers upload a small palette instead of a
  mesh.
- Consumers without GPU skinning ask extraction for the deformed mesh, which
  runs `deform()`. The CPU raster example does this.
- The skin is emitted once, from `HAnimHumanoid.skin`. The reference lists
  (`joints`, `segments`, `sites`, `viewpoints`) are not visual placements.

**Order within a tick.** HAnimMotion (and ROUTEs) set joint TRS, then transforms
propagate, then the pose is evaluated, then skinning, then Joint displacers.
Segment displacers apply to their Segment's own mesh before it is placed.

## Consequences

- GPU renderers get a compact, spec-exact skin description and pay per frame
  only for the palette; CPU consumers get the same result from one reference
  implementation, which is also the conformance oracle.
- The descriptor is public API. Its layout (CSR influences, humanoid-local
  space, source-index remap) is fixed by this ADR.
- Content with the v2 binding fields is supported but not represented in the
  archive, so it is tested with synthetic scenes.
