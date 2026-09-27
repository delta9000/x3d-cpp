# Text — conformance

_Generated. Levels 1 · 2 nodes · profiles: Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| FontStyle | 1 | ✓ | — | — | SEAM-TEXT-METRICS, TXT-1, TXT-2, TXT-4, TXT-5 | X3DFontStyleNode |
| Text | 1 | ✓ | ✓ | — | SEAM-TEXT-METRICS, TXT-1, TXT-2, TXT-4, TXT-5 | X3DGeometryNode |

## Findings

- **TXT-1** [major/FIXED] — §15.2.2.3: justify MIDDLE (minor axis) now centres the glyph BLOCK about the origin on both axes (was centring the baselines, lifting the block by ~one ascender).
  - TextLayout MIDDLE used firstBaselineMinor = totalBlockExtent/2, which centres baselines not the ascender/descender-bounded block. Fixed for horizontal (Y, both topToBottom) and vertical (X, both leftToRight); text_layout_test Test 11 updated to the spec-correct baselines (0.2 / -0.8).
- **TXT-2** [major/CLOSED] — §15.2.2.3: justify END (minor axis, horizontal, topToBottom=FALSE) mis-places the block (top edge of last line should be Y=0).
  - Fixed with the whole minor-axis table (Table 15.4/15.5): END with topToBottom FALSE now puts the top edge of the last line at Y=0, and the horizontal BEGIN/topToBottom TRUE and vertical FIRST/BEGIN/END placements that shared the error were corrected too (text_layout_test minor-justify table).
- **TXT-4** [major/CLOSED] — §15.2.2.3: justify END (minor axis, vertical, leftToRight=FALSE) mis-places columns (left edge of last column should be X=0).
  - Fixed: END with leftToRight FALSE puts the left edge of the last column at X=0, a column spanning [baseline + descender, baseline + ascender] as its quads do (text_layout_test minor-justify table).
- **TXT-5** [minor/CLOSED] — §15.2.2.2: family[] MFString fallback not iterated — should skip unsupported families and fall back to the next entry via the FontMetrics seam.
  - Fixed: FontStyle.family is read as the MFString it is (the enum-token read returned nothing, so every Text used SERIF) and resolved with resolveFontFamily — the first family the FontMetrics backend supports, else SERIF (text_extract_test family fallback).
- **SEAM-TEXT-METRICS** [minor/CLOSED] — §15.4.2, 15.4.1: Text glyphs are stretched to the advance cell and ascender/descender are hard-coded — GlyphMetrics exposes only advanceEm + atlas UV, so the seam cannot place a proportional-font glyph at its true box even with a real atlas.
  - Closed: GlyphMetrics now carries the normalized glyph box and font ascent/descent, populated by stb_truetype and FreeType; buildTextMesh places the quad at that box within the advance cell. The text_extract_test atlas-UV case checks the offset, narrower glyph box; vertical extraction tests verify legacy stub coordinates, Y-axis length compression, and real glyph-box placement. font_metrics_tests extends the backend swap proof across all added fields. Metrics without hasGlyphBox retain advance-cell geometry.

