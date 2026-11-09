# Text — conformance

_Generated. Levels 1 · 2 nodes · profiles: Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| FontStyle | 1 | ✓ | — | — | DELTA-TEXT-OUTPUTS, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS, SEAM-TEXT-METRICS, TXT-1, TXT-2, TXT-4, TXT-5, TXT-ATLAS-LATIN1 | X3DFontStyleNode |
| Text | 1 | ✓ | ✓ | — | DELTA-PENDING-RESOURCES, DELTA-TEXT-OUTPUTS, META-EXTENDED-ATTRIBUTES-WRITE, PROTO-EMPTY-OVERRIDE-WRITE, PROTO-INSTANCE-IS-WRITE, ROUTE-IO-ALIAS, SEAM-TEXT-METRICS, TXT-1, TXT-2, TXT-4, TXT-5, TXT-ATLAS-LATIN1 | X3DGeometryNode |

## Findings

- **TXT-1** [major/FIXED] — §15.2.2.3: justify MIDDLE (minor axis) now centres the glyph BLOCK about the origin on both axes (was centring the baselines, lifting the block by ~one ascender).
  - TextLayout MIDDLE used firstBaselineMinor = totalBlockExtent/2, which centres baselines not the ascender/descender-bounded block. Fixed for horizontal (Y, both topToBottom) and vertical (X, both leftToRight); text_layout_test Test 11 updated to the spec-correct baselines (0.2 / -0.8).
- **TXT-2** [major/CLOSED] — §15.2.2.3: justify END (minor axis, horizontal, topToBottom=FALSE) mis-places the block (top edge of last line should be Y=0).
  - Fixed with the whole minor-axis table (Table 15.4/15.5): END with topToBottom FALSE now puts the top edge of the last line at Y=0, and the horizontal BEGIN/topToBottom TRUE and vertical FIRST/BEGIN/END placements that shared the error were corrected too (text_layout_test minor-justify table).
- **TXT-4** [major/CLOSED] — §15.2.2.3: justify END (minor axis, vertical, leftToRight=FALSE) mis-places columns (left edge of last column should be X=0).
  - Fixed: END with leftToRight FALSE puts the left edge of the last column at X=0, a column spanning [baseline + descender, baseline + ascender] as its quads do (text_layout_test minor-justify table).
- **TXT-ATLAS-LATIN1** [major/FIXED] — §15.4.1, 15.4.2; Annex E FontStyle support: The optional stbtt atlas drops Latin-1 Supplement glyphs present in its supplied font.
  - glyph_atlas_tests.cpp: The atlas covers printable Basic Latin and Latin-1 Supplement with complete glyph boxes and raw advances.
- **PROTO-INSTANCE-IS-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.8: All four writers retain IS mappings on nested prototype instances.
- **PROTO-EMPTY-OVERRIDE-WRITE** [major/FIXED] — §ISO/IEC 19775-1, 4.8; ISO/IEC 19776-1, 4.3.10: JSON and Classic writers retain explicit NULL and empty MFNode prototype overrides.
- **DELTA-PENDING-RESOURCES** [major/FIXED] — §ISO/IEC 19775-1 15.4.2, 18.4.6: Pending textures and glyphs retry on later ticks and publish incremental completion.
- **TXT-5** [minor/CLOSED] — §15.2.2.2: family[] MFString fallback not iterated — should skip unsupported families and fall back to the next entry via the FontMetrics seam.
  - Fixed: FontStyle.family is read as the MFString it is (the enum-token read returned nothing, so every Text used SERIF) and resolved with resolveFontFamily — the first family the FontMetrics backend supports, else SERIF (text_extract_test family fallback).
- **SEAM-TEXT-METRICS** [minor/CLOSED] — §15.4.2, 15.4.1: Text glyphs are stretched to the advance cell and ascender/descender are hard-coded — GlyphMetrics exposes only advanceEm + atlas UV, so the seam cannot place a proportional-font glyph at its true box even with a real atlas.
  - text_extract_test.cpp: Glyph boxes position text quads independently of pen advance.
- **DELTA-TEXT-OUTPUTS** [minor/FIXED] — §ISO/IEC 19775-1 15.4.1, 15.4.2: Incremental Text rebuilds refresh textBounds, lineBounds, and origin outputs.

