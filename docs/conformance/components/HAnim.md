# HAnim — conformance

_Generated. Levels 1,2 · 6 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| HAnimDisplacer | 1 | ✓ | — | — | HAN-2, HAN-AUDIT-2, HANIM-DISP, ROUTE-IO-ALIAS | X3DGeometricPropertyNode |
| HAnimHumanoid | 1 | ✓ | — | — | HAN-1, HAN-AUDIT-1, HAN-AUDIT-4, ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode |
| HAnimJoint | 1 | ✓ | — | — | HAN-1, HAN-AUDIT-1, ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode |
| HAnimMotion | 2 | ✓ | — | — | HAN-3, HAN-AUDIT-4, ROUTE-IO-ALIAS | X3DChildNode |
| HAnimSegment | 1 | ✓ | — | — | GRP-ADDCHILDREN, HAN-AUDIT-2, HANIM-DISP, ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode, X3DGroupingNode |
| HAnimSite | 1 | ✓ | — | — | GRP-ADDCHILDREN, HAN-AUDIT-3, ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode, X3DGroupingNode |

## Findings

- **HAN-1** [major/FIXED] — §26.3.2, 26.3.3: HAnim skins deform with the skeleton (ADR-0055).
  - runtime/hanim/HAnimSkin.hpp compiles an immutable SkinBinding per humanoid (bind positions and normals, uncapped normalized CSR influences, inverse bind matrices from the v2 jointBinding* fields or identity for the v1 rest pose) and evaluates a humanoid-local palette each tick. SceneExtractor emits the skin once from HAnimHumanoid.skin with RenderItem::skin (binding plus per-corner source coordinate/normal indices), reports pose-only changes in RenderDelta::updatedSkinPose, and recompiles the binding only on binding-field edits. deformedMesh() runs the reference CPU skinner; the cpu_raster example draws with it. Verified on archive BoxMan2, JoeKick, Leif and Gramps. deformedMesh() applies each authored corner normal using its source coordinate's influences, including distinct normalIndex mappings; cpu_raster and poc_renderer reverse front-face culling under mirrored transforms, which lights Leif's ccw=false skin. Not wired: poc_renderer does not consume the descriptor (draws the bind pose); llimit/ulimit/limitOrientation/stiffness, skeletalConfiguration, loa and the mass properties are informational and ignored. (2026-09-27)
- **HAN-2** [major/FIXED] — §26.3.1: HAnimDisplacer morphs Joint skins and Segment meshes with live weights.
  - Joint displacers are part of the SkinBinding and apply after skinning along the owning joint's axes; their weight is SkinPose state, so animating it reports updatedSkinPose without recompiling the binding. Segment displacers apply to the Segment's own coord when the extractor builds a mesh under that Segment (MeshBuildOptions::hanimSegment); a weight or displacement edit rebuilds that mesh through updatedGeometry. Tests: hanim_skin_test, scene_extractor_hanim_test. (2026-09-27)
- **HAN-3** [major/FIXED] — §26.3.4: HAnimMotion plays frame-major channels through the event context.
  - HAnimMotionSystem drives referenced motions, honours enable gates, frame controls and channel masks, and emits frameCount/cycleTime/elapsedTime. Synthetic regression tests cover two joints, Euler order, stepping, looping, gating and ignored groups. (2026-09-27)
- **HAN-AUDIT-1** [major/FIXED] — §26.3.2; ISO/IEC 19774-1 6.2: jointBinding* values were treated as absolute; a child Joint's bind matrix omitted its parents'.
  - 19774-1 §6.2: applying the binding translation/rotation/scale "to the corresponding Joint objects maps a skeleton to the binding pose", so the values replace each Joint's own TRS and compose down the skeleton. compileBinding now walks the skeleton with each Joint's binding TRS (keeping its center and scaleOrientation) and inverts the accumulated matrix. Test: hanim_skin_test "child joint binding matrix composes with its parent binding". (2026-09-27)
- **HAN-AUDIT-2** [major/FIXED] — §26.3.1, 26.3.5; ISO/IEC 19774-1 6.6: One geometry under two Segments sharing a Coordinate got one Segment's displacement, and delta() disagreed with fullSnapshot().
  - The raw mesh cache was keyed by geometry alone and the Segment was recorded per geometry (last walk wins). Displaced builds are now cached per (geometry, Segment), and each placement's Segment is derived from its own path on both the full walk and the delta() rebuild. Undisplaced geometry stays shared per geometry (ADR-0045). Test: scene_extractor_hanim_test "Segment-shared geometry keeps per-Segment displacement in snapshot and delta". (2026-09-27)
- **HAN-AUDIT-3** [major/FIXED] — §26.3.6; ISO/IEC 19774-1 6.5: HAnimSite's center/rotation/scale/scaleOrientation/translation were ignored.
  - TransformSystem::isTransform omitted HAnimSite, so Site children (attachments, and cameras under Sites) were placed in the parent's frame. HAnimSite is now a transform-bearing node for propagation, dirty classification and extraction. Test: scene_extractor_hanim_test "HAnimSite transforms its child geometry". (2026-09-27)
- **HAN-AUDIT-4** [minor/FIXED] — §26.3.2; ISO/IEC 19774-2 6.4: A Motion assigned to HAnimHumanoid.motions after load never played.
  - HAnimMotionSystem captured the motions list only at attach. It now re-syncs each tick when the [in,out] motions list changes, keeping the playback state of motions still referenced. Test: hanim_motion_test "HAnimHumanoid motions accepts a newly assigned Motion". (2026-09-27)
- **HANIM-DISP** [minor/FIXED] — §26.3.1, 26.3.5: Segment displacers deform the extracted Segment mesh; the authored Coordinate is unchanged.
  - ADR-0055 resolves the policy: displacement is applied to the mesh built for the Segment's coord, never written back into Coordinate.point, so no point_changed event is emitted (superseding the ADR-0032 mutation policy). The extractor passes the nearest enclosing HAnimSegment to the mesh builder. PickSystem builds meshes without a Segment and so picks against the undisplaced points. (2026-09-27)

