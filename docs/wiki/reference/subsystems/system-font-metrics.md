---
title: Font metrics
summary: FontMetrics supplies glyph advances and boxes through a host callback, with stb_truetype metrics and atlas implementations.
tags: [subsystem, font, metrics, seam, stb-truetype, stbtt, contract]
updated: 2026-06-24
related:
  - ../../explanation/architecture.md
  - ../seam-matrix.md
  - extract-text.md
  - ../../explanation/decisions/0025-fontmetrics-second-backend-swap-test.md
  - ../../explanation/decisions/0024-textureresolver-second-backend-swap-test.md
---

# Font metrics

`runtime/extract/FontMetrics.hpp` defines the host callback:

```cpp
using FontMetrics = std::function<GlyphResult(const FontKey&)>;
```

`FontKey` selects a family, style, and codepoint. `GlyphResult` carries `Ready`,
`Pending`, or `Failed` and `GlyphMetrics`. Layout uses these metrics without
opening font files. See [Text extraction](extract-text.md) for layout rules.

## Metrics contract

`advanceEm` is the raw advance divided by units per em. Layout multiplies it
by `FontStyle.size`. Read the integer font metrics before converting to float;
do not derive units per em from a pixel-scale reciprocal.

An absent codepoint, including glyph index zero, returns `Failed`.
`Pending` is permitted for a render-time atlas that is not ready yet.
`hasAtlasUv` indicates valid atlas coordinates; metrics-only adapters leave it
false. `bearingX`, `sizeX`, `sizeY`, and `top` describe the glyph box relative
to the pen. `ascent` and `descent` describe the font's vertical metrics.

`makeMonospaceStub()` returns ready metrics with `advanceEm=0.6` and no atlas.
It is a fixed-pitch approximation, not a font or a conformance claim.

## stb implementation

Enable `X3D_CPP_BUILD_STBTT` for `x3d_stbtt`.
`runtime/io/stbtt/StbttFontMetrics.hpp` exposes
`makeStbttFontMetrics(FontFaceMap)`. It loads mapped font faces lazily and
caches them by path. Missing families, missing glyphs, and unsupported styles
fail. The metrics implementation accepts PLAIN faces.

`makeStbttGlyphAtlas` supplies the atlas path described in
[Text extraction](extract-text.md#optional-glyph-atlas). Font buffers outlive
the stb font structures that refer to them.

`runtime/io/tests/font_metrics_tests.cpp` covers metrics and failure cases;
`glyph_atlas_tests.cpp` covers raster boxes, advances, and UVs.

## Related

- [Architecture](../../explanation/architecture.md)
- [Seam matrix](../seam-matrix.md)
- [ADR-0025: Second FontMetrics Backend (FreeType) + Advance Swap-Test as the Genericity Proof](../../explanation/decisions/0025-fontmetrics-second-backend-swap-test.md)
- [ADR-0024: Second TextureResolver Backend (wuffs) + Decode Swap-Test as the Genericity Proof](../../explanation/decisions/0024-textureresolver-second-backend-swap-test.md)
