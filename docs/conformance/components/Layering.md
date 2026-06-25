# Layering — conformance

_Generated. Levels 1 · 3 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Layer | 1 | ✓ | — | — | LAY-2, LAY-3 | X3DLayerNode, X3DPickableObject |
| LayerSet | 1 | ✓ | — | — | LAY-1 |  |
| Viewport | 1 | ✓ | — | — | LAY-3, VIEWPORT-CLIP | X3DBoundedObject, X3DChildNode, X3DGroupingNode, X3DViewportNode |

## Findings

- **LAY-1** [major/OPEN] — §35.4.2: LayerSet.order render-suppression is inert — unlisted layers render anyway.
  - runtime/extract/SceneExtractor.hpp:598-610 recurses all SFNode/MFNode children unconditionally; LayerSet.order is never read. §35.4.2: layers not listed in 'order' shall not be rendered. (sweep 2026-06-25)
- **LAY-2** [major/OPEN] — §35.3.1: Layer.pickable is never consulted by PickSystem.
  - runtime/scene/PickSystem.hpp:60-63 builds roots_ from scene.rootNodes with no Layer awareness; no path reads Layer.pickable / X3DLayerNode::getPickable(). §35.3.1: a layer with pickable=FALSE does not participate in picking. (sweep 2026-06-25)
- **LAY-3** [major/DEFERRED] — §35.4.3: Viewport.clipBoundary / Layer.viewport have no render-surface clipping.
  - generated_cpp_bindings/Viewport.hpp:79 stores+validates clipBoundary but no system reads it; SceneExtractor has no viewport sub-region logic. Blocked on render-surface scissor/clip support (renderer seam), so deferred rather than open. (sweep 2026-06-25)
- **VIEWPORT-CLIP** [minor/OPEN] — §35.4.3: clipBoundary remap-vs-clip semantics undefined; the runtime must define the sub-rect/aspect contract for renderers.
  - Mantis 326. Policy (ADR-0035) - adopt glViewport (remap) semantics - sub-rect x=l*W, y=b*H, w=(r-l)*W, h=(t-b)*H; viewpoint aspect = w/h (from the sub-rect, not the surface); scissor to the rect; guard l<=r, b<=t. Expose a derived ViewportRegion{x,y,w,h,aspect} so renderers consume one struct. Today Viewport.hpp:87 stores the fractions only; no consumer in runtime/. Justified by the node name and 35.2.3/35.2.4 independent-view/CAD intent. 4.1 - unresolved (Mantis 326 still drifting in 2026); engine contract ahead of spec.

