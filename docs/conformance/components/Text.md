# Text — conformance

_Generated. Levels 1 · 2 nodes · profiles: Immersive, Full._

| Node | Lvl | Exists | Extract | Behaves | Findings | Interfaces |
|------|-----|--------|---------|---------|----------|------------|
| FontStyle | 1 | ✓ | — | — | TXT-1, TXT-2, TXT-4, TXT-5 | X3DFontStyleNode |
| Text | 1 | ✓ | ✓ | — | TXT-1, TXT-2, TXT-4, TXT-5 | X3DGeometryNode |

## Findings

- **TXT-2** [major/OPEN] — §15.2.2.3: justify END (minor axis, horizontal, topToBottom=FALSE) mis-places the block (top edge of last line should be Y=0).
- **TXT-4** [major/OPEN] — §15.2.2.3: justify END (minor axis, vertical, leftToRight=FALSE) mis-places columns (left edge of last column should be X=0).
- **TXT-5** [minor/OPEN] — §15.2.2.2: family[] MFString fallback not iterated — should skip unsupported families and fall back to the next entry via the FontMetrics seam.
- **TXT-1** [major/FIXED] — §15.2.2.3: justify MIDDLE (minor axis) now centres the glyph BLOCK about the origin on both axes (was centring the baselines, lifting the block by ~one ascender).
  - TextLayout MIDDLE used firstBaselineMinor = totalBlockExtent/2, which centres baselines not the ascender/descender-bounded block. Fixed for horizontal (Y, both topToBottom) and vertical (X, both leftToRight); text_layout_test Test 11 updated to the spec-correct baselines (0.2 / -0.8).

