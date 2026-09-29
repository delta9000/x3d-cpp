---
title: H-Anim
summary: Humanoid skin bindings and live joint palettes, the reference CPU skinner, Joint and Segment displacers, HAnimMotion playback, and how extraction publishes skins.
tags: [subsystem, hanim, skinning, motion, extraction]
updated: 2026-09-27
related:
  - ../decisions/0055-hanim-skinning-descriptor.md
  - extract.md
---

# H-Anim

X3D §26 delegates H-Anim semantics to ISO/IEC 19774. HAnimHumanoid and
HAnimJoint are transforms, so rigid Segment geometry follows the skeleton
through ordinary transform propagation. This page covers what is added on top
of that: skin deformation, displacers and motion playback. The design is
[ADR-0055](../decisions/0055-hanim-skinning-descriptor.md).

## Order within a tick

1. `HAnimMotionSystem` and ROUTEs set joint `rotation` / `translation`.
2. Transforms propagate.
3. Extraction evaluates the skin pose (on `delta()` / `fullSnapshot()`).
4. A consumer skins: on the GPU from the descriptor, or on the CPU with
   `deformedMesh()`. Joint displacers apply after skinning.

Segment displacers apply to their Segment's own mesh when it is built, before
placement.

## Binding and pose

`runtime/hanim/HAnimSkin.hpp` is the contract; `HAnimSkinImpl.hpp` implements it.

`compileBinding()` builds an immutable `SkinBinding` for one HAnimHumanoid:

- **Joint order.** Palette order is `HAnimHumanoid.joints` with repeats removed,
  followed by any skeleton-reachable joint with skin weights that the list
  omits.
- **Influences.** All valid vertex influences are kept in CSR form (never
  capped at four) and normalized per vertex. Malformed list lengths, indices
  and weight sums are recorded in `diagnostics`.
- **Bind pose.** Authored `skinBindingCoords` / `skinBindingNormals` replace the
  skin source arrays; `Coordinate` and `CoordinateDouble` are both accepted.
  `jointBindingPositions` / `Rotations` / `Scales` replace each joint's own
  translation, rotation and scale in the binding pose (19774-1 §6.2), associated
  by position in the `joints` list; a single value applies to every joint. The
  joint keeps its `center` and `scaleOrientation`, and its bind matrix composes
  down the skeleton from its parents' bind matrices. Without these fields every
  inverse bind matrix is the identity: the v1 and BASIC rest pose.

`evaluatePose()` walks the skeleton and composes the same local matrices as
`TransformSystem`, excluding the humanoid's own transform, so everything stays
humanoid-local. The palette is `C_j · B_j⁻¹`. Displacer weights are read at
evaluation time.

## Deformation

`deform()` is the reference CPU skinner and the single implementation of the
math:

- Every weighted source position is blended through the palette. An
  unweighted position keeps its bind value.
- Authored normals are blended by each palette matrix's inverse-transpose 3×3,
  then normalized.
- Joint displacements are added after skinning, rotated by the owning joint's
  3×3.

The SDK never writes deformed values into `skinCoord.point`.

## Displacers

A **Joint** displacer is part of the binding. Its `weight` is pose state: a
weight change reports a pose update and does not recompile the binding.

A **Segment** displacer offsets points of its Segment's own `coord`, in Segment
coordinates. When the extractor builds a mesh under an HAnimSegment, it passes
the nearest enclosing Segment in `MeshBuildOptions::hanimSegment`. The mesh
builder then calls `displaceSegmentPoints(segment, coord, points)`, which
applies only when the geometry's `coord` is that Segment's `coord`. The authored
Coordinate is not modified and no `point_changed` is emitted.

A weight or displacement edit rebuilds the Segment mesh through
`updatedGeometry`. Displaced meshes are cached per (geometry, Segment), and
each placement finds its Segment from its own path. A geometry under two
Segments that share one Coordinate therefore gets each Segment's displacement,
and `delta()` and `fullSnapshot()` agree. Undisplaced geometry stays shared.
`PickSystem` builds meshes without a Segment and picks against the undisplaced
points.

## Sites

HAnimSite is a transform: its `translation`, `rotation`, `scale`,
`scaleOrientation` and `center` place its children, like HAnimJoint.

## Extraction

The skin is emitted once, from `HAnimHumanoid.skin`: Shapes, or geometry nodes
placed there directly (§26.3.2). The `joints`, `segments`, `sites` and
`viewpoints` fields are references and add no visual placements.

Each skin item carries `RenderItem::SkinDesc`:

- the shared `SkinBinding`;
- the source coordinate index of every expanded mesh corner (and source normal
  indices when authored);
- a pose version.

A joint transform or Joint-displacer weight change advances the version and
lists the item in `RenderDelta::updatedSkinPose`. The geometry ID and mesh stay
the same, so a GPU consumer uploads only the palette. The binding is recompiled
only when a binding field changes: the humanoid's skin or binding fields, a
joint's `skinCoordIndex` / `skinCoordWeight`, or the skeleton structure.

CPU consumers call `SceneExtractor::deformedMesh(id)`. It evaluates the pose,
deforms the source coordinates and maps them to the expanded corners. Each
corner's normal is its bind normal (the authored `skinNormal` /
`skinBindingNormals` entry through the corner's normal index, else the normal
the mesh builder generated for the bind pose) skinned through the palette's
inverse transpose with the influences of the corner's source *coordinate*, so
`normalIndex` may differ from `coordIndex`. The `cpu_raster` example draws
skins this way.

`poc_renderer` skins on the GPU with the same math. It uploads each skin item's
per-corner (offset, count) ranges and its uncapped (joint, weight) influences
once, as a vertex attribute and a buffer texture. On `updatedSkinPose` it
evaluates the pose and replaces only the palette buffer; the vertex shaders
blend positions and inverse-transpose normals. Only skins with Joint displacers
use `deformedMesh()` instead, refreshing their mesh on pose updates.

Both renderers flip the front face under a negative-determinant world
transform (`ccw` XOR reflection). Leif and Lily use `ccw=false` under the
humanoid scale `1 1 -1`.

### Visual verification

Posed archive humanoids (ROUTEs stripped, fixed joint rotations, one fixed
camera) were rendered in `cpu_raster`, `poc_renderer` and an independent X3D
browser. Poses agree for JoeKick, Leif, Lily and BoxMan2. The archive files are
not committed, so this is a manual check; the committed GL regressions are
`x3d_poc_skin_gl` (GPU skin against precomputed deformed geometry, including a
five-influence vertex) and `x3d_poc_positional_light_gl`.

## Motion

`HAnimMotionSystem` (`runtime/hanim/HAnimMotionSystem.hpp`, registered by
`attachStandardRuntime`) plays each motion in `HAnimHumanoid.motions`.

**Motions list.** `HAnimHumanoid.motions` is `[in,out]`. When it changes, the
system re-syncs on the next tick: new motions start, removed ones stop, and
motions still referenced keep their playback state.

**Gating.** Playback needs both the humanoid's `motionsEnabled` entry and the
motion's `enabled` field; an absent `motionsEnabled` entry means enabled.

**Channels.** Joint names resolve against the humanoid's joints, including
those in the skeleton hierarchy.

- `channels` gives, for each name in `joints`, a count followed by that many
  transform names. Commas and spaces both separate tokens.
- An `IGNORED` group still consumes its values.
- `channelsEnabled` indexes the flattened channel list; absent entries mean
  enabled.
- `values` is indexed by frame, then group, then channel.
- Euler channels are in degrees. They compose in listed order into one
  axis-angle joint rotation, in radians.
- Position channels set the named translation components.

Joint changes are posted through the event context, so ROUTEs and dirty
tracking see them before transform propagation.

**Frame control.**

- `frameDuration` sets the frame interval.
- `frameIncrement` sets direction and stride; zero pauses automatic advance.
- `frameIndex` is clamped to the available frames.
- `startFrame` / `endFrame` bound playback; an `endFrame` of zero means the
  last frame.
- A true `next` / `previous` steps once and wraps.
- Automatic playback stops at the boundary unless `loop` is true.

The motion emits `frameCount`, `cycleTime` (at activation and at each wrap) and
cumulative `elapsedTime`.

The channel and control rules follow the
[ISO/IEC 19774-2 v2.1 working draft §6.3–6.4](https://www.web3d.org/specifications/X3Dv4Draft/ISO-IEC19774/ISO-IEC19774-2/ISO-IEC19774-2v2.1/ISO-IEC19774-2v2.1-WD/MotionDataAnimation/MotionNodes.html),
because the published 2.0 motion-node page could not be retrieved. Units follow
[published 19774-2 §5.2.3–5.2.4](https://www.web3d.org/documents/specifications/19774-2/V2.0/MotionDataAnimation/AnimationUsingInterpolators.html).

## Not implemented

- `llimit`, `ulimit`, `limitOrientation`, `stiffness`: inverse-kinematics hints.
- `skeletalConfiguration`, `loa`.
- The mass properties.


## Tests

- `runtime/hanim/tests/hanim_skin_test.cpp`: constructed scenes for arm
  rotation, blended normals, more than four influences, binding fields, Joint
  and Segment displacers, and bad input.
- `runtime/extract/tests/scene_extractor_hanim_test.cpp`: skin placement,
  corner remap, pose-only deltas, displacer weights and binding recompiles.
- `runtime/events/tests/hanim_motion_test.cpp`: motion playback.
- `examples/poc_renderer/tests/skin_gl_test.py`: llvmpipe image comparisons
  for a rotated skin and a five-influence skin against ordinary geometry.

Setting `X3D_ARCHIVE_DIR` to the Web3D archive's `examples` directory enables
the archive smoke cases. These cover BoxMan2, Leif and Gramps skinning (with
timings) and KoreanCharacterMotionAnnexD01Jin motion.
