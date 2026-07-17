---
title: Font Metrics Seam
summary: The FontMetrics seam (font-table advance → advanceEm) proven generic by two independent backends — stb_truetype + FreeType — and a CI-gated exact-equal advanceEm swap-test over Liberation font fixtures (PLAIN style).
tags: [subsystem, font, metrics, seam, stb-truetype, freetype, swap-test, genericity, thesis]
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
`GlyphResult{status, GlyphMetrics{advanceEm, …}}`. It is the per-codepoint advance path:
the layout engine calls it once per character to compute text extent and pen advance; it
never opens a font file, calls a rasterizer, or touches an atlas — all of that stays in the
embedder. The SDK **never reads a font** — the seam is a consumer-supplied callback.

The advance surface is `GlyphMetrics.advanceEm`: advance / em-size (dimensionless). The
layout engine multiplies by `FontStyle.size` to get local-coordinate advance per §15.4.1.
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

!!! note "Seam proven generic — two independent backends (stb_truetype + FreeType)"
    The font-metrics seam is **GREEN** ([ADR-0025](../decisions/0025-fontmetrics-second-backend-swap-test.md)):
    two fully independent backends implement the *unchanged* `FontMetrics` interface, and a
    CI-gated swap-test asserts they return **exact bit-identical `advanceEm`** for every
    PLAIN-style glyph in the Liberation font fixtures. Because the interface carried two
    backends with **no signature change**, it was promoted `[EXPERIMENTAL]` → `[STABLE]` in
    `include/x3d/sdk.hpp`. See the [Seam-Status Matrix](../seam-status.md).

## Key files

| File / directory | Role |
|---|---|
| `runtime/extract/FontMetrics.hpp` | The seam: `FontMetrics` callback type, `FontKey` / `GlyphMetrics` / `GlyphResult` / `GlyphStatus`, `makeMonospaceStub()`. std-only, leaf, font-free. |
| `runtime/io/stbtt/StbttFontMetrics.{hpp,cpp}` | **Backend A** — stb_truetype. `makeStbttFontMetrics(FontFaceMap)`. The `.cpp` is the one TU that `#include`s `stb_truetype.h` (PRIVATE). Built behind `X3D_CPP_BUILD_STBTT`. |
| `runtime/io/stbtt/vendor/stb_truetype.h` | Vendored stb_truetype single-header (MIT OR Public Domain; see `STB_TRUETYPE_LICENSE.txt`). |
| `runtime/io/freetype/FreetypeFontMetrics.{hpp,cpp}` | **Backend B** — FreeType. `makeFreetypeFontMetrics(FontFaceMap)`. The `.cpp` is the one TU that calls the FreeType C API (PRIVATE). Built behind `X3D_CPP_BUILD_FREETYPE` (via vcpkg). |
| `runtime/io/tests/font_metrics_tests.cpp` | The grouped doctest binary `x3d_text_tests`: per-backend cases, the exact-equal advanceEm swap-test, Failed-parity on `.notdef`, and BOLD/ITALIC out-of-scope cases. Talks only through the seam factories — no backend headers. |
| `third_party/fonts/` | Liberation font fixtures (`LiberationSerif-Regular.ttf`, `LiberationSans-Regular.ttf`, `LiberationMono-Regular.ttf`) + `OFL.txt` license. |

## The two backends

Both backends accept a `FontFaceMap` (`std::map<std::string, std::string>`, family → TTF
path), open each face once at construction, cache it, and return `advanceEm` on demand.

- **Backend A — stb_truetype** (`x3d_stbtt` static lib, `X3D_CPP_BUILD_STBTT`, OFF default).
  The hand-written C truetype parser adjacent to stb_image in the PoC renderer's tree.
  `stb_truetype.h` is included **PRIVATE** in the single TU, so consumers linking `x3d_stbtt`
  inherit no stb headers — the public `StbttFontMetrics.hpp` is font-free. Reads `unitsPerEm`
  from the `head` table and the `hmtx` advance for each codepoint; computes
  `advanceEm = (float)advance / (float)unitsPerEm`. **No core `#ifdef`.**
- **Backend B — FreeType** (`x3d_freetype` static lib, `X3D_CPP_BUILD_FREETYPE`, OFF default,
  via vcpkg). The reference-quality font library, **fully independent** of stb_truetype (shares
  zero implementation). FreeType is initialized once per construction; faces are opened with
  `FT_New_Face` and cached. Advance is read via `FT_Get_Advance` (hinting disabled,
  `FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING`); `unitsPerEm` from `FT_Face::units_per_EM`.
  Same `advanceEm = (float)advance / (float)unitsPerEm` formula. **No core `#ifdef`.**

## The genericity proof — exact-equal advanceEm swap-test

`x3d_text_tests` (`runtime/io/tests/font_metrics_tests.cpp`) queries the **same** Liberation
font fixtures through both backends and asserts **exact bit-identical `advanceEm`** (strict
`==`, no epsilon) for every PLAIN-style glyph, plus equal `Failed` status on absent
codepoints. The test TU talks **only through the seam factories** and includes no backend
header — the ABI-isolation discipline from the
[AssetResolver seam](system-asset-io.md) (whose swap-test once segfaulted on AWS SDK
header-vs-lib skew). Gated by the `fontmetrics-swap` CI job
(`-DX3D_CPP_BUILD_STBTT=ON -DX3D_CPP_BUILD_FREETYPE=ON`, scoped build,
`ctest -R x3d_text`) — no network, no service container, Liberation TTFs are vendored.

### The exact-equality boundary

Exact bit-identical `advanceEm` holds because both backends read **the same integer inputs**
from the same font tables:

- `unitsPerEm` is the raw `uint16` from the `head` table (same value in both libraries for
  the same TTF).
- `hmtxAdvance` is the integer advance from the `hmtx` table for the codepoint's glyph index.

Both compute `(float)hmtxAdvance / (float)unitsPerEm` — the same IEEE-754 `float / float`
divide over identical integer operands gives a bit-identical result on any conformant
hardware. This is structurally guaranteed, not empirically tuned.

### Swap matrix

| Family | Style | Codepoints tested | Equality criterion | Result |
|---|---|---|---|---|
| SERIF | PLAIN | A, i, M, (space), 0 | exact `==` advanceEm | Pass |
| SANS | PLAIN | A, i, M, (space), 0 | exact `==` advanceEm | Pass |
| TYPEWRITER | PLAIN | A, i, M, (space), 0 | exact `==` advanceEm | Pass |
| SERIF/SANS/TYPEWRITER | BOLD | — | out of scope (stb has no embolden API) | Failed (correct) |
| SERIF/SANS/TYPEWRITER | ITALIC | — | out of scope (stb has no oblique API) | Failed (correct) |
| Any | PLAIN | U+1F600 (absent) | equal `Failed` | Pass |
| Any | PLAIN | atlas UV, outline | out of scope (not font-table-derived) | Pending/N/A |

99/99 assertions passed. The 3 family × 5 codepoint × exact-equal cells are the swap matrix;
BOLD/ITALIC are dropped cells (not out of parity — both backends correctly return Failed for
faces that were never loaded).

### The `.notdef` contract rule

When a codepoint's cmap lookup yields glyph index 0 (the `.notdef` sentinel), both backends
return `GlyphStatus::Failed`. This is the contract: `.notdef` is not a valid glyph — the
layout engine must substitute a space or skip the character, not render a box. Both backends
enforce this identically; the swap-test verifies parity on `U+1F600`.

### Dropped cells — synthetic BOLD/ITALIC

stb_truetype has no embolden or oblique API. The `FontFaceMap` passed to both backends in
the swap-test contains PLAIN faces only. When either backend receives a request with
`style != "PLAIN"`, it cannot find the face in the map and returns `GlyphStatus::Failed`.
This is the correct contract for the current fixture set. Synthetic style variants (applying
font-side embolden or oblique transforms) are a separate future card.

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

## Scope / what this seam does and doesn't unblock

Like the TextureResolver seam, the font-metrics seam unblocks **no specific conformance
findings** — its value is **thesis-completion** (the full font-metrics path is now proven
generic) and providing a real advance source for `Text` node layout. The PoC's monospace stub
remains available via `makeMonospaceStub()` and is unchanged. Out of scope: synthetic
BOLD/ITALIC variants, atlas-UV / outline integration, vertical text advance (advance height
per §15.2.2.3 — a separate card), and replacing the monospace stub with a full layout engine
for the PoC renderer.
