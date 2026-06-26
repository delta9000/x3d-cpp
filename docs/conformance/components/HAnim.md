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

- **HAN-1** [major/DEFERRED] — §26.3.2, 26.3.3: HAnim skin deformation (skinCoord/skinNormal/skinCoordWeight) unimplemented.
  - runtime/extract/PackedMesh.hpp:54-55 marks Joints/Weights vertex attribs 'Phase 2+' (TODO at :120); no code reads skinCoord/skinNormal/skinCoordIndex/skinCoordWeight to produce deformed geometry. Blocked on a skinning subsystem. (sweep 2026-06-25)
- **HAN-2** [major/DEFERRED] — §26.3.1: HAnimDisplacer morphing (coordIndex/displacements/weight) entirely inert.
  - generated_cpp_bindings/HAnimDisplacer.hpp has the fields; no runtime code references 'displace'/HAnimDisplacer. §26.3.1 requires coordIndex+displacements vertex-group offsetting. Blocked on the morph/deformation subsystem (HAN-1). (sweep 2026-06-25)
- **HAN-3** [major/DEFERRED] — §26.3.4: HAnimMotion animation driver entirely missing — no frame advancement.
  - generated_cpp_bindings/HAnimMotion.hpp has channels/values/frameIndex/frameDuration/loop/enabled/startFrame/endFrame; no runtime system drives frames or emits cycleTime/elapsedTime/frameCount per §26.3.4. Blocked on the HAnim animation subsystem. (sweep 2026-06-25)
- **HANIM-DISP** [minor/DEFERRED] — §26.3.1, 26.3.5: HAnimDisplacer is inert; displacement never applied to Segment coord.point and no point_changed emitted.
  - X3D defers to ISO/IEC 19774 6.6 which is also silent on whether evaluating a Displacer mutates coord.point. X_ITE applies in-shader (no mutation); Plesch's pointOutput_Tongue.x3d ROUTEs the displaced coord.point (needs mutation). Policy (ADR-0032) - a DisplacerSystem makes coord.point's current value the weighted sum (neutral pose retained internally, non-destructive) and emits point_changed on the Segment's Coordinate node. Deferred on the DisplacerSystem. Today only TransformSystem references HAnim; coordIndex/displacements/weight are loaded but never consumed. 4.1 - unresolved upstream (2025 thread left it open to browser interpretation); engine ahead of spec.

