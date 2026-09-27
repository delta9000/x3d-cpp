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

- **HAN-1** [major/DEFERRED] — §26.3.2, 26.3.3: HAnim skin deformation core exists but is not connected to extraction.
  - runtime/hanim/HAnimSkinImpl.hpp now compiles source coordinates, variable-width normalized influences and bind matrices, evaluates joint palettes, and deforms positions/normals. Extraction does not yet consume the binding or publish deformed geometry, so authored skins remain static in rendered scenes. (2026-09-27)
- **HAN-2** [major/DEFERRED] — §26.3.1: HAnimDisplacer math exists but is not connected to extraction.
  - runtime/hanim/HAnimSkinImpl.hpp applies Joint displacers after skinning and Segment displacers to caller-owned points. Extraction and event scheduling do not yet call these functions, so morphing is not visible in rendered scenes. (2026-09-27)
- **HAN-3** [major/DEFERRED] — §26.3.4: HAnimMotion animation driver entirely missing — no frame advancement.
  - generated_cpp_bindings/HAnimMotion.hpp has channels/values/frameIndex/frameDuration/loop/enabled/startFrame/endFrame; no runtime system drives frames or emits cycleTime/elapsedTime/frameCount per §26.3.4. Blocked on the HAnim animation subsystem. (sweep 2026-06-25)
- **HANIM-DISP** [minor/DEFERRED] — §26.3.1, 26.3.5: Segment displacer output is not connected to extracted geometry.
  - ADR-0055 resolves Segment displacement as a caller-owned mesh deformation without changing the authored Coordinate. runtime/hanim/HAnimSkinImpl.hpp implements that calculation and a reverse lookup for Segment geometry coordinates. Extraction does not yet call it, and no point_changed event is emitted; the earlier ADR-0032 mutation policy is superseded for this core. (2026-09-27)

