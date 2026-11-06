---
title: Font Metrics Seam
summary: FontMetrics supplies glyph advances and boxes through a host callback, with stb_truetype metrics and atlas implementations.
tags: [subsystem, font, metrics, seam, stb-truetype, stbtt, contract]
updated: 2026-06-24
related:
  - ../architecture.md
  - ../seam-status.md
  - ../subsystems/extract-text.md
  - ../decisions/0025-fontmetrics-second-backend-swap-test.md
  - ../decisions/0024-textureresolver-second-backend-swap-test.md
---

# Font Metrics Seam

## Purpose

The `FontMetrics` seam turns a **`FontKey{family, style, codepoint}`** lookup into
`GlyphResult{status, GlyphMetrics{advanceEm, glyph box, font vertical metrics, …}}`. It is the per-codepoint metrics path:
the layout engine calls it once per character to compute text extent and pen advance; it
never opens a font file, calls a rasterizer, or touches an atlas — all of that stays in the
embedder. The SDK **never reads a font** — the seam is a consumer-supplied callback.

The advance surface is `GlyphMetrics.advanceEm`: advance / em-size (dimensionless). The
layout engine multiplies by `FontStyle.size` to get local-coordinate advance per §15.4.1.
`bearingX`, `sizeX`, `sizeY`, and `top` describe the glyph box relative to its pen in em
units; `ascent` and `descent` carry the font's vertical metrics. Text extraction positions
the atlas quad from the glyph box rather than stretching it across the advance.
The lifecycle (`Ready` / `Pending` / `Failed`) mirrors TextureResolver: `Pending` means the
atlas is not uploaded yet (skip or substitute a space); `Failed` means the codepoint is
absent from the font.

The seam type is a single value callback:

```cpp
using FontMetrics =
    std::function<GlyphResult(const FontKey&)>;
```

`makeMonospaceStub()` is the always-`Ready` stub (`advanceEm = 0.6f`); it never reads a
font file and gives correct proportional text layout for fixed-pitch output.

The interface remains stable. The unused FreeType implementation and parity
CI job were removed; see the
[decision amendment](../decisions/0025-fontmetrics-second-backend-swap-test.md#amendment-2026-11-05).
The stb metrics and atlas implementations remain available.

## Key files

| File / directory | Role |
|---|---|
| `runtime/extract/FontMetrics.hpp` | The seam: `FontMetrics` callback type, `FontKey` / `GlyphMetrics` / `GlyphResult` / `GlyphStatus`, `makeMonospaceStub()`. std-only, leaf, font-free. |
| `runtime/io/stbtt/StbttFontMetrics.{hpp,cpp}` | **Backend A** — stb_truetype. `makeStbttFontMetrics(FontFaceMap)`. The `.cpp` is the one TU that `#include`s `stb_truetype.h` (PRIVATE). Built behind `X3D_CPP_BUILD_STBTT`. |
| `runtime/io/stbtt/vendor/stb_truetype.h` | Vendored stb_truetype single-header (MIT OR Public Domain; see `STB_TRUETYPE_LICENSE.txt`). |
| `runtime/io/tests/font_metrics_tests.cpp` | `x3d_text_tests`: stb metrics, absent codepoint, unmapped family, and unsupported style cases. |
| `third_party/fonts/` | Liberation font fixtures (`LiberationSerif-Regular.ttf`, `LiberationSans-Regular.ttf`, `LiberationMono-Regular.ttf`) + `OFL.txt` license. |

## stb_truetype backend

`x3d_stbtt` accepts a family-to-TTF `FontFaceMap`, opens each face at
construction, and caches it. The implementation reads font-table advances and
normalizes them to em units. The factory header exposes no stb types.
Enable `X3D_CPP_BUILD_STBTT` to build the metrics and atlas implementations.

`x3d_text_tests` checks ready metrics for Liberation glyphs, absent codepoints,
unmapped families, and unsupported styles. `x3d_text_atlas_tests` checks raster
and atlas behavior against the same fonts. Run them with:

```bash
ctest --test-dir build -R x3d_text --output-on-failure
```

A cmap lookup of glyph index 0 (`.notdef`) returns Failed, so the layout engine
can substitute a space or skip the character. The reference metrics map loads
PLAIN faces; synthetic bold and italic styles remain unsupported.

## Adapter contract rules

When writing a `FontMetrics` adapter:

1. **Return `Failed` for absent codepoints** — glyphIndex == 0 (`.notdef`) is not a valid
   advance; return `GlyphStatus::Failed`, not a fallback advance.
2. **No core `#ifdef`** — the backend TU includes the font library PRIVATE; the public factory
   header is font-free. This is the ABI isolation rule (ADR-0023).
3. **Compute `advanceEm` as `(float)advance / (float)unitsPerEm`** — integer operands from the
   font tables; this gives exact equality across independent backends.
4. **`Pending` is not an error** — if the atlas is not ready, return `GlyphStatus::Pending`
   so the layout engine can skip or substitute; never block.
5. **`hasAtlasUv` is optional** — set it only when the atlas UV coordinates are valid;
   `false` is the correct value when the backend provides advances only.

## Scope

The metrics callback supplies advances and optional glyph boxes and atlas UVs
for Text layout. `makeMonospaceStub()` remains available for hosts without a
font backend. The stb glyph atlas is the reference renderer's font path.
