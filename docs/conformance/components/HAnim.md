# HAnim — conformance

_Generated. Levels 1,2 · 6 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| HAnimDisplacer | 1 | ✓ | — | — | HAN-2, HANIM-DISP, ROUTE-IO-ALIAS | X3DGeometricPropertyNode |
| HAnimHumanoid | 1 | ✓ | — | — | HAN-1, ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode |
| HAnimJoint | 1 | ✓ | — | — | HAN-1, ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode |
| HAnimMotion | 2 | ✓ | — | — | HAN-3, ROUTE-IO-ALIAS | X3DChildNode |
| HAnimSegment | 1 | ✓ | — | — | HANIM-DISP, ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode, X3DGroupingNode |
| HAnimSite | 1 | ✓ | — | — | ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode, X3DGroupingNode |

## Findings

- **HAN-1** [major/FIXED] — §26.3.2, 26.3.3: HAnim skins deform with the skeleton (ADR-0055).
  - runtime/hanim/HAnimSkin.hpp compiles an immutable SkinBinding per humanoid (bind positions and normals, uncapped normalized CSR influences, inverse bind matrices from the v2 jointBinding* fields or identity for the v1 rest pose) and evaluates a humanoid-local palette each tick. SceneExtractor emits the skin once from HAnimHumanoid.skin with RenderItem::skin (binding plus per-corner source coordinate/normal indices), reports pose-only changes in RenderDelta::updatedSkinPose, and recompiles the binding only on binding-field edits. deformedMesh() runs the reference CPU skinner; the cpu_raster example draws with it. Verified on archive BoxMan2, JoeKick, Leif and Gramps. Not wired: poc_renderer does not consume the descriptor (draws the bind pose); llimit/ulimit/limitOrientation/stiffness, skeletalConfiguration, loa and the mass properties are informational and ignored; skinNormal is assumed to be indexed like skinCoord. (2026-09-27)
- **HAN-2** [major/FIXED] — §26.3.1: HAnimDisplacer morphs Joint skins and Segment meshes with live weights.
  - Joint displacers are part of the SkinBinding and apply after skinning along the owning joint's axes; their weight is SkinPose state, so animating it reports updatedSkinPose without recompiling the binding. Segment displacers apply to the Segment's own coord when the extractor builds a mesh under that Segment (MeshBuildOptions::hanimSegment); a weight or displacement edit rebuilds that mesh through updatedGeometry. Tests: hanim_skin_test, scene_extractor_hanim_test. (2026-09-27)
- **HAN-3** [major/FIXED] — §26.3.4: HAnimMotion plays frame-major channels through the event context.
  - HAnimMotionSystem drives referenced motions, honours enable gates, frame controls and channel masks, and emits frameCount/cycleTime/elapsedTime. Synthetic regression tests cover two joints, Euler order, stepping, looping, gating and ignored groups. (2026-09-27)
- **HANIM-DISP** [minor/FIXED] — §26.3.1, 26.3.5: Segment displacers deform the extracted Segment mesh; the authored Coordinate is unchanged.
  - ADR-0055 resolves the policy: displacement is applied to the mesh built for the Segment's coord, never written back into Coordinate.point, so no point_changed event is emitted (superseding the ADR-0032 mutation policy). The extractor passes the nearest enclosing HAnimSegment to the mesh builder. PickSystem builds meshes without a Segment and so picks against the undisplaced points. (2026-09-27)

