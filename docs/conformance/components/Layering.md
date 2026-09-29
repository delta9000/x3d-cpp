# Layering — conformance

_Generated. Levels 1 · 3 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Layer | 1 | ✓ | — | — | LAY-2, LAY-3, ROUTE-IO-ALIAS | X3DLayerNode, X3DPickableObject |
| LayerSet | 1 | ✓ | — | — | LAY-1, ROUTE-IO-ALIAS |  |
| Viewport | 1 | ✓ | — | — | GRP-ADDCHILDREN, LAY-3, ROUTE-IO-ALIAS, VIEWPORT-CLIP | X3DBoundedObject, X3DChildNode, X3DGroupingNode, X3DViewportNode |

## Findings

- **LAY-3** [major/DEFERRED] — §35.4.3: Viewport.clipBoundary / Layer.viewport have no render-surface clipping.
  - generated_cpp_bindings/Viewport.hpp:79 stores+validates clipBoundary but no system reads it; SceneExtractor has no viewport sub-region logic. Blocked on render-surface scissor/clip support (renderer seam), so deferred rather than open. (sweep 2026-06-25)
- **VIEWPORT-CLIP** [minor/OPEN] — §35.4.3: clipBoundary remap-vs-clip semantics undefined; the runtime must define the sub-rect/aspect contract for renderers.
  - Mantis 326. Policy (ADR-0035) - adopt glViewport (remap) semantics - sub-rect x=l*W, y=b*H, w=(r-l)*W, h=(t-b)*H; viewpoint aspect = w/h (from the sub-rect, not the surface); scissor to the rect; guard l<=r, b<=t. Expose a derived ViewportRegion{x,y,w,h,aspect} so renderers consume one struct. Today Viewport.hpp:87 stores the fractions only; no consumer in runtime/. Justified by the node name and 35.2.3/35.2.4 independent-view/CAD intent. 4.1 - unresolved (Mantis 326 still drifting in 2026); engine contract ahead of spec.
- **LAY-1** [major/CLOSED] — §35.4.2: LayerSet.order render-suppression is inert — unlisted layers render anyway.
  - Closed 2026-09-26: SceneExtractor follows only LayerSet.layers ordinals listed in order (zero-based per §35.4.2; invalid ordinals are ignored). Covered by scene_extractor_layers_respect_layerset_order in scene_extractor_b2_test.cpp.
- **LAY-2** [major/CLOSED] — §35.3.1: Layer.pickable is never consulted by PickSystem.
  - Closed 2026-09-26: PickSystem filters candidates through live Layer.pickable values on each query, so cached geometry indexes honor runtime changes. Covered by pick_system_skips_non_pickable_layer_and_hits_layer_behind in pick_system_test.cpp.

