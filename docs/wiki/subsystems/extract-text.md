---
title: Text Extraction
summary: Text node extraction and layout via the font-metrics seam — converts X3D Text/FontStyle into renderable glyph-quad geometry.
tags: [subsystem, extract, text, font-metrics, text-layout]
updated: 2026-06-24
related:
  - ../architecture.md
  - ../subsystems/extract.md
  - ../subsystems/extract-textures.md
  - ../subsystems/generated-bindings.md
  - ../subsystems/sdk-facade.md
---

# Text Extraction

## Purpose

This subsystem converts X3D `Text` and `FontStyle` nodes into renderable geometry. It owns three responsibilities: (1) running the §15 layout engine to place per-line baselines and compute block bounds, (2) emitting one CCW glyph quad per codepoint into a `MeshData` with `isGlyphMesh=true`, and (3) writing the `Text` node's `outputOnly` fields (`textBounds`, `lineBounds`, `origin`) back via reflection. The SDK never opens font files or calls a rasterizer — the consumer supplies all glyph metrics and atlas UV coordinates through the `FontMetrics` callback seam.

## Key files

| File | Role |
|---|---|
| `runtime/extract/FontMetrics.hpp` | Defines the `FontMetrics` seam type (`std::function<GlyphResult(const FontKey&)>`), the `FontKey` lookup triple (family / style / UTF-32 codepoint), `GlyphMetrics` (advanceEm + atlas UV rect), `GlyphResult` / `GlyphStatus` (Ready / Pending / Failed), and `makeMonospaceStub()` (default stub: advanceEm=0.6, no atlas UV). |
| `runtime/extract/TextLayout.hpp` | Pure §15 layout engine. No node or IO dependencies. Implements `computeTextLayout(FontStyleParams, TextParams, FontMetricsCallback) -> TextLayoutResult`: baseline-step, justify (BEGIN/END/FIRST/MIDDLE for both axes), per-line `length[]` stretch/compress, `maxExtent` compress-to-fit, and `lineBounds`/`textBounds`/`origin` output computation per ISO/IEC 19775-1:2023 §15.2.2.3, §15.4.1, §15.4.2. |
| `runtime/extract/TextExtract.hpp` | Integration layer. Reads `FontStyle` fields and `Text` fields off nodes via reflection (`geombounds::getNode`, `geombounds::getField`), calls `computeTextLayout`, walks each line glyph-by-glyph through the `FontMetrics` seam, and emits quads into `MeshData`. Also provides `setTextOutputs()` to write layout results back onto the `Text` node via the reflection `set` lambdas. |
| `runtime/extract/tests/text_layout_test.cpp` | 19 unit tests for `computeTextLayout` using a monospaced stub. Covers all justify modes, vertical text, spacing, `length[]`, and `maxExtent`. |
| `runtime/extract/tests/text_extract_test.cpp` | 6 integration tests for `buildTextMesh` and `setTextOutputs`, including a full `SceneExtractor` walk and an atlas-UV seam thread-through. |
| `runtime/io/stbtt/StbttFontMetrics.hpp` | Decoder-free header for the stb_truetype `FontMetrics` backend (ADR-0025 / T-TEXT genericity proof, Backend A). Declares `makeStbttFontMetrics(FontFaceMap)` only; does not include `stb_truetype.h`. Part of the `x3d_stbtt` target (flag-gated, default OFF). |
| `runtime/io/stbtt/StbttFontMetrics.cpp` | Single TU where `stb_truetype.h` is included PRIVATE (`STB_TRUETYPE_IMPLEMENTATION` defined here). Reads unscaled advances via `stbtt_GetGlyphHMetrics`, derives `unitsPerEm` from the raw `head` table uint16 at offset 18 (big-endian). Returns `makeFailed()` for .notdef (glyph 0), unmapped family, or non-PLAIN style. |
| `runtime/io/tests/font_metrics_tests.cpp` | Per-backend doctest binary (`x3d_text_tests`). Task 2: `fontmetrics_backend_a_stbtt` verifies Ready+sane advanceEm for 5 Liberation codepoints and Failed-parity for absent emoji, unmapped family, and BOLD style. Tasks 3–4 will add the FreeType case and the cross-backend swap-test. |

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

- **FontMetrics callback** — the consumer supplies a `FontMetrics` (`std::function<GlyphResult(FontKey)>`) that maps a (family, style, UTF-32 codepoint) triple to an advance ratio and optional atlas UV rect. The SDK provides `makeMonospaceStub()` as the default (advanceEm=0.6, no atlas UV) so layout math is fully testable without any real font. A consumer wires a real callback via `MeshBuildOptions::fontMetrics` at `SceneExtractor` construction time.

- **MeshBuildOptions::fontMetrics** — the field in `runtime/extract/MeshBuilder.hpp`'s `MeshBuildOptions` struct that carries the `FontMetrics` callback into the extraction pipeline. Defaults to `makeMonospaceStub()`. The `SceneExtractor` stores a copy and forwards it to `buildTextMesh` when it visits a `Text` geometry node.

- **Reflection / generated-bindings seam** — `readFontStyleParams` and `readTextParams` read all `Text` and `FontStyle` node fields via `geombounds::getField` and `geombounds::getNode` rather than casting to concrete node types. Enum fields (`justify`, `family`, `style`) are read through the node-agnostic `getEnumString` reflection thunk, keeping `TextExtract.hpp` decoupled from the generated enum-class types. `setTextOutputs` writes the three `outputOnly` output fields through the reflection `set` lambdas.

- **SceneExtractor T-TEXT dispatch** — inside `SceneExtractor`, the geometry dispatch hook (labelled `T-TEXT` in `runtime/extract/SceneExtractor.hpp`) detects a `Text` geometry node, calls `buildTextMesh` to produce the glyph-quad `MeshData`, and then calls `setTextOutputs` to populate the node's `outputOnly` fields. The produced `MeshData` carries `isGlyphMesh=true` and `solid=false` (two-sided per §15.2.1.2). The consumer uses `isGlyphMesh` to route the render item to a text-shader path.

## How it is tested

- `ctest --preset dev -R x3d_text_layout` — 19 unit tests for `computeTextLayout` in isolation. Cover all horizontal justify modes (BEGIN/MIDDLE/END/FIRST), topToBottom, leftToRight, vertical text (`horizontal=false`), `length[]` stretch and compress, `maxExtent` compress-to-fit, multi-line spacing, and the FIRST-minor-axis special case (baseline at Y=0). Uses a monospaced stub (advance=N×size, ascender=0.8×size, descender=−0.2×size); all assertions are analytic.

- `ctest --preset dev -R x3d_text_extract` — 6 integration tests for `buildTextMesh` and `setTextOutputs`. Verifies: quad count and vertex positions for a single line and a two-line string; that `setTextOutputs` writes all three `outputOnly` fields and they round-trip correctly through the generated `Text` node's typed getters (`getTextBounds()`, `getLineBounds()`, `getOrigin()`); a full `SceneExtractor` walk over a `Shape>Text` scene emitting the glyph mesh and setting node outputs; atlas-UV threading from a custom `FontMetrics` seam; and that Failed glyphs produce no quads.

No golden files exist for this subsystem — the tests use analytic expected values.

## FontMetrics seam genericity proof (ADR-0025)

The `FontMetrics` seam is proven generic across real font backends using the same
U1–U4 discipline as `TextureResolver` (ADR-0024). Backend work lives in the
`x3d_stbtt` (and eventually `x3d_freetype`) flag-gated targets; the seam header
(`FontMetrics.hpp`) ships with the core and is never contaminated by decoder
headers.

### Adapter contract (cross-backend equality surface)

All conformant backends must:

1. Read advances **unscaled** (raw `hmtx` font units, no hinting/grid-fitting).
2. Derive `unitsPerEm` as the raw `uint16` at `head+18` (big-endian) — not from a
   pixel-scale reciprocal — and compute `advanceEm = float(advance) / float(upm)`.
3. Return `makeFailed()` when the codepoint maps to glyph index 0 (`.notdef`).
4. Set `hasAtlasUv = false` (atlas path is embedder-defined, not part of equality).
5. Honor `PLAIN` style only; all other style values → `makeFailed()`.

### Backend A — `x3d_stbtt` (stb_truetype)

- Enabled with `-DX3D_CPP_BUILD_STBTT=ON`.
- Single-TU isolation: `stb_truetype.h` is included PRIVATE in
  `StbttFontMetrics.cpp`; the factory returns a `std::function`, so no stb header
  leaks to consumers.
- Lazy per-family face load with path-keyed cache (`State::faceFor`).
- Tested by `fontmetrics_backend_a_stbtt` in `x3d_text_tests`
  (`ctest -R x3d_text`): 5 Liberation codepoints (A, i, M, space, 0) → Ready with
  `0 < advanceEm < 2`; absent emoji U+1F600, unmapped family, BOLD style → Failed.

### Build and run

```bash
cmake -S . -B build-font -G Ninja -DX3D_CPP_BUILD_STBTT=ON -DX3D_CPP_BUILD_TESTS=ON
cmake --build build-font --target x3d_text_tests
ctest --test-dir build-font --output-on-failure -R x3d_text
```

## Related specs and ADRs

- Spec: ISO/IEC 19775-1:2023 §15 (Text component) — `§15.2.1.2` (solid=false, Z=0 plane), `§15.2.2.3` (direction and justification algorithm), `§15.4.1` (FontStyle: size, spacing, family, style, justify), `§15.4.2` (Text: string, length, maxExtent, textBounds, lineBounds, origin output fields).
- Design spec: `docs/superpowers/specs/2026-06-16-v1-closure-roadmap-design.md` — T-TEXT track (§§ T-TEXT-1, T-TEXT-2, T-TEXT-3); all three tasks are CLOSED.
- BACKLOG closure rows: `docs/superpowers/BACKLOG.md` § "T-TEXT — Text rendered via font-metrics seam" (T-TEXT-1..3 CLOSED; T-TEXT-D1 bidi/complex shaping and T-TEXT-D2 exact-bounds revisit remain OPEN as named deferrals).
- [Extraction Pipeline](../subsystems/extract.md) — the parent extraction pipeline (`SceneExtractor`, `MeshBuilder`) that owns the dispatch hook wiring `Text` into this subsystem.
- [Texture Extraction](../subsystems/extract-textures.md) — the parallel seam for texture/material extraction; shares the `MeshBuildOptions` struct and the same `SceneExtractor` dispatch pattern.
- [Generated Bindings](../subsystems/generated-bindings.md) — source of the `Text` and `FontStyle` node types and their reflection descriptors consumed by the integration layer.
- [SDK Façade](../subsystems/sdk-facade.md) — the public API surface through which a consumer wires a real `FontMetrics` callback at `SceneExtractor` construction.
