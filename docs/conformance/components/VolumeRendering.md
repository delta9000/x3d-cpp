# VolumeRendering — conformance

_Generated. Levels 1,2,3 · 13 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| BlendedVolumeStyle | 3 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DComposableVolumeRenderStyleNode, X3DVolumeRenderStyleNode |
| BoundaryEnhancementVolumeStyle | 2 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DComposableVolumeRenderStyleNode, X3DVolumeRenderStyleNode |
| CartoonVolumeStyle | 3 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DComposableVolumeRenderStyleNode, X3DVolumeRenderStyleNode |
| ComposedVolumeStyle | 3 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DComposableVolumeRenderStyleNode, X3DVolumeRenderStyleNode |
| EdgeEnhancementVolumeStyle | 2 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DComposableVolumeRenderStyleNode, X3DVolumeRenderStyleNode |
| IsoSurfaceVolumeData | 2 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS, VOL-1 | X3DBoundedObject, X3DChildNode, X3DVolumeDataNode |
| OpacityMapVolumeStyle | 1 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DComposableVolumeRenderStyleNode, X3DVolumeRenderStyleNode |
| ProjectionVolumeStyle | 2 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DVolumeRenderStyleNode |
| SegmentedVolumeData | 2 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS, VOL-1 | X3DBoundedObject, X3DChildNode, X3DVolumeDataNode |
| ShadedVolumeStyle | 3 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DComposableVolumeRenderStyleNode, X3DVolumeRenderStyleNode |
| SilhouetteEnhancementVolumeStyle | 2 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DComposableVolumeRenderStyleNode, X3DVolumeRenderStyleNode |
| ToneMappedVolumeStyle | 2 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS | X3DComposableVolumeRenderStyleNode, X3DVolumeRenderStyleNode |
| VolumeData | 1 | ✓ | — | — | META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS, VOL-1 | X3DBoundedObject, X3DChildNode, X3DVolumeDataNode |

## Findings

- **VOL-1** [major/DEFERRED] — §41.4.1-41.4.13: VolumeRendering entirely inert — all 13 nodes are parse-only stubs.
  - grep of runtime/ for VolumeData/VolumeStyle returns 0 hits; X3DVolumeDataNode has no 'geometry' field so SceneExtractor.hpp:519 produces no RenderItem, and recognizedGeometryType() omits all volume types. No VolumeSystem/voxel path. Blocked on a volume-rendering subsystem. (sweep 2026-06-25)
- **PROTO-INSTANCE-IS-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.8: All four writers retain IS mappings on nested prototype instances.
- **PROTO-EMPTY-OVERRIDE-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.10: JSON and Classic writers retain explicit NULL and empty MFNode prototype overrides.

