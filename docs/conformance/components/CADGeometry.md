# CADGeometry — conformance

_Generated. Levels 1,2 · 6 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| CADAssembly | 2 | ✓ | — | — | GRP-ADDCHILDREN, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode, X3DGroupingNode, X3DProductStructureChildNode |
| CADFace | 2 | ✓ | — | — | CAD-1, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode, X3DProductStructureChildNode |
| CADLayer | 2 | ✓ | — | — | GRP-ADDCHILDREN, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode, X3DGroupingNode |
| CADPart | 2 | ✓ | — | — | GRP-ADDCHILDREN, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode, X3DGroupingNode, X3DProductStructureChildNode |
| IndexedQuadSet | 1 | ✓ | ✓ | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DComposedGeometryNode, X3DGeometryNode |
| QuadSet | 1 | ✓ | ✓ | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DComposedGeometryNode, X3DGeometryNode |

## Findings

- **PROTO-INSTANCE-IS-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.8: All four writers retain IS mappings on nested prototype instances.
- **PROTO-EMPTY-OVERRIDE-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.10: JSON and Classic writers retain explicit NULL and empty MFNode prototype overrides.
- **CAD-1** [minor/FIXED] — §32.4.2: CADFace.shape type constraint ([Shape|LOD|Transform]) is unenforced.
  - Fixed: SceneExtractor::walk generic loop now skips a CADFace.shape child whose nodeTypeName is not Shape|LOD|Transform, mirroring the Collision.proxy guard. Tested in scene_extractor_cad1_test.cpp. (sweep 2026-06-25, fixed same day)

