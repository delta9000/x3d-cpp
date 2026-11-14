---
title: Text Extraction
summary: Text node extraction and layout via the font-metrics seam — converts X3D Text/FontStyle into renderable glyph-quad geometry.
tags: [subsystem, extract, text, font-metrics, text-layout]
updated: 2026-09-30
related:
  - ../../explanation/architecture.md
  - extract.md
  - extract-textures.md
  - generated-bindings.md
  - sdk-facade.md
---

# Text Extraction

## Purpose

- This subsystem converts X3D `Text` and `FontStyle` nodes into renderable
  geometry.
- It owns three responsibilities: (1) running the §15 layout engine to place
  per-line baselines and compute block bounds, (2) emitting one CCW glyph quad
  per codepoint into a `MeshData` with `isGlyphMesh=true`, and (3) writing the
  `Text` node's `outputOnly` fields (`textBounds`, `lineBounds`, `origin`)
  back via reflection.
- The SDK never opens font files or calls a rasterizer — the consumer supplies
  all glyph metrics and atlas UV coordinates through the `FontMetrics`
  callback seam.

## Key files

| File | Role |
|---|---|
| `runtime/extract/FontMetrics.hpp` | Defines the `FontMetrics` seam type (`std::function<GlyphResult(const FontKey&)>`), the `FontKey` lookup triple (family / style / UTF-32 codepoint), `GlyphMetrics` (advanceEm, atlas UV, glyph box and font ascent/descent in em units), `GlyphResult` / `GlyphStatus` (Ready / Pending / Failed), and `makeMonospaceStub()` (default stub: advanceEm=0.6, no atlas UV). |
| `runtime/extract/TextLayout.hpp` | Pure §15 layout engine. No node or IO dependencies. Implements `computeTextLayout(FontStyleParams, TextParams, FontMetricsCallback) -> TextLayoutResult`: baseline-step, justify, per-line `length[]` stretch/compress, `maxExtent` compress-to-fit, and `lineBounds`/`textBounds`/`origin` output computation per ISO/IEC 19775-1:2023 §15.2.2.3, §15.4.1, §15.4.2. |
| `runtime/extract/TextExtract.hpp` | Integration layer. Reuses the text readers and UTF-8 decoder in `runtime/scene/GeometryBounds.hpp`, calls `computeTextLayout`, walks each line glyph-by-glyph through the `FontMetrics` seam, and emits quads into `MeshData`. Also provides `setTextOutputs()` to write layout results back onto the `Text` node via the reflection `set` lambdas. |
| `runtime/extract/tests/text_layout_test.cpp` | 19 unit tests for `computeTextLayout` using a monospaced stub. Covers the justify modes (including a table test of every minor-axis case against Tables 15.4/15.5), vertical text, spacing, `length[]`, and `maxExtent`. |
| `runtime/extract/tests/text_extract_test.cpp` | Integration tests for `buildTextMesh` and `setTextOutputs()`, including a full `SceneExtractor` walk and atlas glyph-box placement. |
| `runtime/io/stbtt/StbttFontMetrics.hpp` | Decoder-free header for the stb_truetype `FontMetrics` backend (ADR-0025 / metrics backend). Declares `makeStbttFontMetrics(FontFaceMap)` only; does not include `stb_truetype.h`. Part of the `x3d_stbtt` target (flag-gated, default OFF). |
| `runtime/io/stbtt/StbttFontMetrics.cpp` | Single TU where `stb_truetype.h` is included PRIVATE (`STB_TRUETYPE_IMPLEMENTATION` defined here). Reads unscaled advances via `stbtt_GetGlyphHMetrics`, derives `unitsPerEm` from the raw `head` table uint16 at offset 18 (big-endian). Returns `makeFailed()` for `.notdef` (glyph 0), unmapped family, or non-PLAIN style. |
| `runtime/io/stbtt/StbttGlyphAtlas.cpp` | Optional raster helper: bakes printable Basic Latin and Latin-1 Supplement into a bottom-up coverage atlas, with exact advances and bitmap-aligned glyph boxes/UVs. |
| `runtime/io/tests/glyph_atlas_tests.cpp` | Atlas bitmap/metrics oracle over all three bundled Liberation faces, boundaries, blank glyphs, and failure cases (`x3d_text_atlas_tests`). |
| `runtime/io/tests/font_metrics_tests.cpp` | `x3d_text_tests`: stb metrics and failure contract cases. |

## Interfaces and seams

### Exposed interface

```cpp
namespace x3d::runtime::extract {

// ---- FontMetrics.hpp ----

struct FontKey {
  std::string   family;     // "SERIF" | "SANS" | "TYPEWRITER" | named
  std::string   style;      // "PLAIN" | "BOLD" | "ITALIC" | "BOLDITALIC"
  std::uint32_t codepoint;  // UTF-32
};

struct GlyphMetrics {
  float advanceEm  = 0.0f;   // advance / em; multiply by FontStyle.size for local units
  bool  hasAtlasUv = false;
  float u0, v0, u1, v1;      // atlas UV rect, bottom-left origin (GL convention)
};

enum class GlyphStatus { Ready, Pending, Failed };

struct GlyphResult {
  GlyphStatus  status;
  GlyphMetrics metrics;
  bool ready()   const;
  bool pending() const;
  bool failed()  const;
  static GlyphResult makeReady(GlyphMetrics);
  static GlyphResult makePending();
  static GlyphResult makeFailed();
};

using FontMetrics = std::function<GlyphResult(const FontKey&)>;

FontMetrics makeMonospaceStub();  // advanceEm=0.6, hasAtlasUv=false

// ---- TextLayout.hpp ----

struct FontStyleParams {
  float size = 1.0f; float spacing = 1.0f;
  bool horizontal = true; bool leftToRight = true; bool topToBottom = true;
  std::string justifyMajor = "BEGIN"; std::string justifyMinor = "FIRST";
};

struct TextParams {
  std::vector<std::string> strings;
  std::vector<float>       length;   // MFFloat; 0 = unconstrained
  float                    maxExtent = 0.0f;
};

struct LineBounds  { float width = 0.0f; float height = 0.0f; };
struct TextLayoutResult {
  std::vector<LineBounds>            lineBounds;
  float textBoundsX, textBoundsY;
  float originX, originY, originZ;     // upper-left corner of textBounds (Z=0)
  std::vector<std::array<float, 2>>  lineBaselineOrigins;  // [X, Y] per line
};

using FontMetricsCallback =
    std::function<std::tuple<float, float, float>(const std::string&, float)>;
    // returns (natural_advance, ascender, descender)

TextLayoutResult computeTextLayout(
    const FontStyleParams&, const TextParams&, FontMetricsCallback);

// Axis-aligned 2D glyph extent of a laid-out block (local Z=0 plane). Shared
// with the geometry-bounds path so an injected FontMetrics yields exact,
// culling-safe Text bounds without re-deriving the layout.
struct TextExtent2D { float minX, minY, maxX, maxY; bool empty; };
TextExtent2D textLayoutExtent(const FontStyleParams&, const TextLayoutResult&);

// ---- TextExtract.hpp ----

// Produce glyph-quad MeshData for textNode.
// outLayout (optional) receives the layout result for output-field writeback.
MeshData buildTextMesh(const X3DNode& textNode,
                       const FontMetrics& fm,
                       TextLayoutResult* outLayout = nullptr);

// Write textBounds/lineBounds/origin onto the Text node's outputOnly fields.
// Returns the count of fields successfully written (normally 3).
int setTextOutputs(X3DNode& textNode, const TextLayoutResult& layout);

} // namespace x3d::runtime::extract
```

### Seam points

- Minor-axis placement — Tables 15.4/15.5 name an edge of the first or last line (column) to sit on the origin; a line spans `[baseline + descender, baseline + ascender]` across the minor axis, as its glyph quads do, and `computeTextLayout` places the first baseline so the named edge is at 0.

- FontMetrics callback — the consumer supplies a `FontMetrics` (`std::function<GlyphResult(FontKey)>`) that maps a (family, style, UTF-32 codepoint) triple to an advance ratio and optional atlas UV rect. The SDK provides `makeMonospaceStub()` as the default (advanceEm=0.6, no atlas UV) so layout math is fully testable without any real font. A consumer wires a real callback via `MeshBuildOptions::fontMetrics` at `SceneExtractor` construction time.

- MeshBuildOptions::fontMetrics — the field in `runtime/extract/MeshBuilder.hpp`'s `MeshBuildOptions` struct that carries the `FontMetrics` callback into the extraction pipeline. Defaults to `makeMonospaceStub()`. The `SceneExtractor` stores a copy and forwards it to `buildTextMesh` when it visits a `Text` geometry node.

- Reflection / generated-bindings seam — `readFontStyleParams` and `readTextParams` are shared with bounds in `GeometryBounds.hpp` and read all `Text` and `FontStyle` node fields via `geombounds::getField` and `geombounds::getNode` rather than casting to concrete node types.
    - `family` is an MFString fallback list read directly and resolved by `resolveFontFamily` (`FontMetrics.hpp`, §15.4.1): the first family the backend resolves a probe glyph in, else SERIF; with no backend injected, the first listed family.
    - Enum fields (`justify`, `style`) are read through the node-agnostic `getEnumString` reflection thunk, keeping `TextExtract.hpp` decoupled from the generated enum-class types.
    - `setTextOutputs()` writes the three `outputOnly` output fields through the reflection `set` lambdas.

- Bounds consumption  — the geometry-bounds path (`runtime/scene/GeometryBounds.hpp`) reads the same `FontStyleParams`/`TextParams` and runs the same `computeTextLayout` through an injected `extract::FontMetrics`, then takes `textLayoutExtent` as the `Text` node's local AABB. `BoundsSystem::setFontMetrics` carries the seam; with it unset the bound is the conservative heuristic. The layout engine stays the single source of glyph placement for both rendering and bounds.

- SceneExtractor Text dispatch — inside `SceneExtractor`, the geometry dispatch hook detects a `Text` geometry node, calls `buildTextMesh` to produce the glyph-quad `MeshData`, and then calls `setTextOutputs()` to populate the node's `outputOnly` fields. The produced `MeshData` carries `isGlyphMesh=true` and `solid=false` (two-sided per §15.2.1.2). The consumer uses `isGlyphMesh` to route the render item to a text-shader path.

## FontMetrics seam

See [font metrics](system-font-metrics.md) for the callback, adapter contract,
and metrics backend configuration.

### Optional glyph atlas

- `makeStbttGlyphAtlas` is a consumer-side helper in `x3d_stbtt`; the headless
  SDK still does no rasterization.
- It bakes U+0020–U+007E and U+00A0–U+00FF from each mapped face, covering the
  printable Basic Latin and Latin-1 Supplement repertoire required by
  [Immersive Annex
  E](https://www.web3d.org/documents/specifications/19775-1/V4.0/Part01/immersive.html).
- DEL/C1 controls, codepoints outside those ranges, missing glyphs, and
  unmapped families resolve to Failed.
- This is bounded repertoire support, not general Unicode, shaping, or style
  conformance; style continues to use the supplied PLAIN face.
- The CPU rasterizer's separate built-in 8x8 font remains ASCII-only.
- The atlas packs each complete bitmap with transparent padding, including
  tall accents and overhangs.
- Bottom-up UVs and `hasGlyphBox=true` geometry describe the same pixel
  rectangle relative to the baseline.
- Box coordinates are raster-pixel aligned (resolution dependent);
  `advanceEm`, ascent, and descent retain the raw font metrics.
- Space and non-breaking space retain their advance with empty boxes.
- `emPx` selects pixels per em, clamped to at least 8.
- `x3d_text_atlas_tests` compares every supported glyph's atlas pixels against
  an isolated stbtt rasterization across SERIF/SANS/TYPEWRITER at 8, 17, and
  64 px per em.
- It verifies bottom-up orientation, unclipped ink, no overlap/stray ink,
  glyph box/UV alignment, exact advances, blank spaces, range boundaries, and
  failures.
- A UTF-8 `Text` extraction case verifies the resulting quad placement and
  UVs, including non-breaking-space advance before the next visible glyph.

### Build and run

```bash
cmake -S. -B build-font -G Ninja -DX3D_CPP_BUILD_STBTT=ON -DX3D_CPP_BUILD_TESTS=ON
cmake --build build-font --target x3d_text_tests x3d_text_atlas_tests
ctest --test-dir build-font --output-on-failure -R x3d_text
```

## Related

- [Extraction Pipeline](extract.md)
- [Texture Extraction](extract-textures.md)
- [Generated Bindings](generated-bindings.md)
- [SDK Façade](sdk-facade.md)
- [Architecture](../../explanation/architecture.md)
