# Layout — conformance

_Generated. Levels 1,2 · 5 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Layout | 1 | ✓ | — | — | REQ-LAYOUT, ROUTE-IO-ALIAS | X3DChildNode, X3DLayoutNode |
| LayoutGroup | 1 | ✓ | — | — | GRP-ADDCHILDREN, REQ-LAYOUT, ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode, X3DGroupingNode |
| LayoutLayer | 1 | ✓ | — | — | REQ-LAYOUT, ROUTE-IO-ALIAS | X3DLayerNode, X3DPickableObject |
| ScreenFontStyle | 2 | ✓ | — | — | LYT-1, REQ-LAYOUT, ROUTE-IO-ALIAS | X3DFontStyleNode |
| ScreenGroup | 2 | ✓ | — | — | GRP-ADDCHILDREN, REQ-LAYOUT, ROUTE-IO-ALIAS | X3DBoundedObject, X3DChildNode, X3DGroupingNode |

## Findings

- **REQ-LAYOUT** [major/OPEN] — §36.2; 36.4.1-36.4.5: Layout rectangles and screen-relative transforms are absent; ScreenFontStyle.pointSize is treated as a world-space size.
  - No production layout-node dispatch exists in TransformSystem or SceneExtractor. TextExtract::readFontStyleParams copies pointSize into FontStyleParams::size without viewport/DPI conversion. Acceptance: nested align/offset/size/scaleMode in WORLD/FRACTION/PIXEL units; ScreenGroup retains pixel scale while the camera moves; screen text and LayoutLayer work with viewport changes.
- **LYT-1** [major/FIXED] — §36.4.4: ScreenFontStyle.pointSize ignored — text always renders at size 1.0.
  - Was: readFontStyleParams (TextExtract.hpp) read 'size' which ScreenFontStyle lacks → always 1.0. Fixed by branching on nodeTypeName()==ScreenFontStyle to read 'pointSize'. Tested in text_extract_test.cpp (test_screenfontstyle_pointsize). (sweep 2026-06-25, fixed same day)

