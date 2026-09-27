# HAnim — conformance

_Generated. Levels 1,2 · 6 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| HAnimDisplacer | 1 | ✓ | — | — | HAN-2, HANIM-DISP | X3DGeometricPropertyNode |
| HAnimHumanoid | 1 | ✓ | — | — | HAN-1 | X3DBoundedObject, X3DChildNode |
| HAnimJoint | 1 | ✓ | — | — | HAN-1 | X3DBoundedObject, X3DChildNode |
| HAnimMotion | 2 | ✓ | — | — | HAN-3 | X3DChildNode |
| HAnimSegment | 1 | ✓ | — | — | HANIM-DISP | X3DBoundedObject, X3DChildNode, X3DGroupingNode |
| HAnimSite | 1 | ✓ | — | — | — | X3DBoundedObject, X3DChildNode, X3DGroupingNode |

## Findings

- **HAN-1** [major/DEFERRED] — §26.3.2, 26.3.3: HAnim skin extraction and pose deltas implemented; deformation core pending.
  - ADR-0055 presentation path emits skin once, retains source-coordinate indices per corner, shares a SkinBinding, reports updatedSkinPose, and exposes SceneExtractor::deformedMesh. HAnimSkinImpl.hpp is still a stub, so joint weights do not yet deform the output. (2026-09-27)
- **HAN-2** [major/DEFERRED] — §26.3.1: HAnimDisplacer extraction hooks exist; deformation core pending.
  - MeshBuilder passes Segment coordinates through hanim::displaceSegmentPoints and skin pose deltas watch Joint displacers. HAnimSkinImpl.hpp is still a stub, so displacement math remains pending. (2026-09-27)
- **HAN-3** [major/DEFERRED] — §26.3.4: HAnimMotion animation driver entirely missing — no frame advancement.
  - generated_cpp_bindings/HAnimMotion.hpp has channels/values/frameIndex/frameDuration/loop/enabled/startFrame/endFrame; no runtime system drives frames or emits cycleTime/elapsedTime/frameCount per §26.3.4. Blocked on the HAnim animation subsystem. (sweep 2026-06-25)
- **HANIM-DISP** [minor/DEFERRED] — §26.3.1, 26.3.5: Segment displacement extraction hook exists; deformation core pending.
  - ADR-0055 supersedes the earlier write-back proposal: Segment mesh extraction calls hanim::displaceSegmentPoints without changing authored Coordinate.point. The implementation in HAnimSkinImpl.hpp is still a stub. The X3D/ISO texts do not require a point_changed event from this evaluation, so no such event is emitted. (2026-09-27)

