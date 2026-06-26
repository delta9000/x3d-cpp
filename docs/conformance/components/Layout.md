# Layout — conformance

_Generated. Levels 1,2 · 5 nodes · profiles: Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| Layout | 1 | ✓ | — | — | — | X3DChildNode, X3DLayoutNode |
| LayoutGroup | 1 | ✓ | — | — | — | X3DBoundedObject, X3DChildNode, X3DGroupingNode |
| LayoutLayer | 1 | ✓ | — | — | — | X3DLayerNode, X3DPickableObject |
| ScreenFontStyle | 2 | ✓ | — | — | LYT-1 | X3DFontStyleNode |
| ScreenGroup | 2 | ✓ | — | — | — | X3DBoundedObject, X3DChildNode, X3DGroupingNode |

## Findings

- **LYT-1** [major/FIXED] — §36.4.4: ScreenFontStyle.pointSize ignored — text always renders at size 1.0.
  - Was: readFontStyleParams (TextExtract.hpp) read 'size' which ScreenFontStyle lacks → always 1.0. Fixed by branching on nodeTypeName()==ScreenFontStyle to read 'pointSize'. Tested in text_extract_test.cpp (test_screenfontstyle_pointsize). (sweep 2026-06-25, fixed same day)

