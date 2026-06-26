# CADGeometry — conformance

_Generated. Levels 1,2 · 6 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| CADAssembly | 2 | ✓ | — | — | — | X3DBoundedObject, X3DChildNode, X3DGroupingNode, X3DProductStructureChildNode |
| CADFace | 2 | ✓ | — | — | CAD-1 | X3DBoundedObject, X3DChildNode, X3DProductStructureChildNode |
| CADLayer | 2 | ✓ | — | — | — | X3DBoundedObject, X3DChildNode, X3DGroupingNode |
| CADPart | 2 | ✓ | — | — | — | X3DBoundedObject, X3DChildNode, X3DGroupingNode, X3DProductStructureChildNode |
| IndexedQuadSet | 1 | ✓ | ✓ | — | — | X3DComposedGeometryNode, X3DGeometryNode |
| QuadSet | 1 | ✓ | ✓ | — | — | X3DComposedGeometryNode, X3DGeometryNode |

## Findings

- **CAD-1** [minor/FIXED] — §32.4.2: CADFace.shape type constraint ([Shape|LOD|Transform]) is unenforced.
  - Fixed: SceneExtractor::walk generic loop now skips a CADFace.shape child whose nodeTypeName is not Shape|LOD|Transform, mirroring the Collision.proxy guard. Tested in scene_extractor_cad1_test.cpp. (sweep 2026-06-25, fixed same day)

