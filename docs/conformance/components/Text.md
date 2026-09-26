# Text — conformance

_Generated. Levels 1 · 2 nodes · profiles: Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| FontStyle | 1 | ✓ | — | — | SEAM-TEXT-METRICS, TXT-1, TXT-2, TXT-4, TXT-5 | X3DFontStyleNode |
| Text | 1 | ✓ | ✓ | — | SEAM-TEXT-METRICS, TXT-1, TXT-2, TXT-4, TXT-5 | X3DGeometryNode |

## Findings

- **SEAM-TEXT-METRICS** [minor/OPEN] — §15.4.2, 15.4.1: Text glyphs are stretched to the advance cell and ascender/descender are hard-coded — GlyphMetrics exposes only advanceEm + atlas UV, so the seam cannot place a proportional-font glyph at its true box even with a real atlas.
  - buildTextMesh TextExtract.hpp:294-326 maps the glyph UV across the full advance-width quad; GlyphMetrics FontMetrics.hpp:59-64 lacks bearing/size/top; vertical metrics fixed at 0.8/-0.2*size. Monospace stub is fine; a real font is not. Fix: extend GlyphMetrics with {bearingX,sizeX,sizeY,top} and place the quad at the actual glyph box. (extraction-seam review.) Visual sweep 2026-06-27 CONFIRMED now that the poc wires a real stb_truetype atlas: set_length_comp/set_length_exp render the length-compressed strings as bars.
- **TXT-1** [major/FIXED] — §15.2.2.3: justify MIDDLE (minor axis) now centres the glyph BLOCK about the origin on both axes (was centring the baselines, lifting the block by ~one ascender).
  - TextLayout MIDDLE used firstBaselineMinor = totalBlockExtent/2, which centres baselines not the ascender/descender-bounded block. Fixed for horizontal (Y, both topToBottom) and vertical (X, both leftToRight); text_layout_test Test 11 updated to the spec-correct baselines (0.2 / -0.8).
- **TXT-2** [major/CLOSED] — §15.2.2.3: justify END (minor axis, horizontal, topToBottom=FALSE) mis-places the block (top edge of last line should be Y=0).
  - Fixed with the whole minor-axis table (Table 15.4/15.5): END with topToBottom FALSE now puts the top edge of the last line at Y=0, and the horizontal BEGIN/topToBottom TRUE and vertical FIRST/BEGIN/END placements that shared the error were corrected too (text_layout_test minor-justify table).
- **TXT-4** [major/CLOSED] — §15.2.2.3: justify END (minor axis, vertical, leftToRight=FALSE) mis-places columns (left edge of last column should be X=0).
  - Fixed: END with leftToRight FALSE puts the left edge of the last column at X=0, a column spanning [baseline + descender, baseline + ascender] as its quads do (text_layout_test minor-justify table).
- **TXT-5** [minor/CLOSED] — §15.2.2.2: family[] MFString fallback not iterated — should skip unsupported families and fall back to the next entry via the FontMetrics seam.
  - Fixed: FontStyle.family is read as the MFString it is (the enum-token read returned nothing, so every Text used SERIF) and resolved with resolveFontFamily — the first family the FontMetrics backend supports, else SERIF (text_extract_test family fallback).

