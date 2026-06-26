// FontMetrics.hpp — T-TEXT font-metrics seam (consumer-supplied glyph metrics).
// namespace x3d::runtime::extract. Header-only, std-only, leaf.
//
// IO contract: the SDK NEVER opens font files or calls a rasterizer. The consumer
// supplies this callback; the layout engine calls it once per codepoint. Same
// Pending semantics as TextureResolver: Pending = atlas not uploaded yet, skip
// glyph or substitute a space; the layout engine never blocks.
//
// Units: advanceEm is in EM units (advance / em-size). The layout engine scales by
// the FontStyle.size field to get local-coordinate advance. For horizontal text
// this is advance width; for vertical text this is advance height (§15.2.2.3).
// atlas UV (u0,v0)-(u1,v1) is normalized [0,1] texture coordinates into an
// embedder-managed atlas texture, bottom-left origin (GL convention).
//
// Key formulas (for the layout engine consuming FontMetrics):
//
//   // Line pitch (baseline-to-baseline distance) per §15.4.1:
//   float linePitch = fontStyle.spacing * fontStyle.size;
//
//   // Per-character pen advance (horizontal text, leftToRight=TRUE):
//   GlyphResult g = fontMetrics({family, style, cp});
//   if (g.ready())
//       penX += g.metrics.advanceEm * fontStyle.size;   // local coords
//
// Placement: included by the forthcoming TextLayout.hpp / TextExtract.hpp — NOT
// by SceneExtractor.hpp directly. This keeps the shared-file edit surface minimal
// per the §3 fan-out structure in the design spec.
#ifndef X3D_RUNTIME_EXTRACT_FONT_METRICS_HPP
#define X3D_RUNTIME_EXTRACT_FONT_METRICS_HPP

#include <cstdint>
#include <functional>
#include <string>

namespace x3d::runtime::extract {

// ---------------------------------------------------------------------------
// FontKey — the lookup triple (family, style, codepoint).
//
// family    — resolved font family string: one of FontStyle.family[] after
//             browser-mapping; the layout engine passes the first matched
//             family. Well-known values: "SERIF", "SANS", "TYPEWRITER".
// style     — one of "PLAIN", "BOLD", "ITALIC", "BOLDITALIC"
//             (§15.4.1 FontStyle.style values; empty string → "PLAIN").
// codepoint — UTF-32 codepoint. The layout engine iterates the UTF-8
//             Text.string[] array and converts to UTF-32 before each call.
// ---------------------------------------------------------------------------
struct FontKey {
  std::string   family;     // e.g. "SERIF", "SANS", "TYPEWRITER", or a named family.
  std::string   style;      // "PLAIN" | "BOLD" | "ITALIC" | "BOLDITALIC"
  std::uint32_t codepoint;  // UTF-32
};

// ---------------------------------------------------------------------------
// GlyphMetrics — per-glyph result payload. advanceEm is always meaningful on
// Ready; hasAtlasUv gates the textured-quad glyph path (when false the consumer
// uses outline/procedural rendering, or skips the glyph in stub mode).
// ---------------------------------------------------------------------------
struct GlyphMetrics {
  float advanceEm = 0.0f;  // advance / em — multiply by FontStyle.size for local units.
  bool  hasAtlasUv = false;
  float u0 = 0.0f, v0 = 0.0f; // atlas UV bottom-left corner (GL convention).
  float u1 = 0.0f, v1 = 0.0f; // atlas UV top-right corner.
};

// ---------------------------------------------------------------------------
// GlyphStatus — lifecycle state of a glyph resolve.
// Pending is ONLY legal on the render-time atlas path: atlas not yet uploaded;
// the layout engine skips or substitutes a space and retries next frame.
// ---------------------------------------------------------------------------
enum class GlyphStatus { Ready, Pending, Failed };

// ---------------------------------------------------------------------------
// GlyphResult — status + metrics. Named factory helpers mirror AssetResult
// and TexturePixelResult conventions.
// ---------------------------------------------------------------------------
struct GlyphResult {
  GlyphStatus  status  = GlyphStatus::Failed;
  GlyphMetrics metrics;

  bool ready()   const { return status == GlyphStatus::Ready;   }
  bool pending() const { return status == GlyphStatus::Pending; }
  bool failed()  const { return status == GlyphStatus::Failed;  }

  static GlyphResult makeReady(GlyphMetrics m) {
    return GlyphResult{GlyphStatus::Ready, m};
  }
  static GlyphResult makePending() {
    return GlyphResult{GlyphStatus::Pending, {}};
  }
  static GlyphResult makeFailed() {
    return GlyphResult{GlyphStatus::Failed, {}};
  }
};

// ---------------------------------------------------------------------------
// FontMetrics — THE callback type.
//
// Copyable value type (std::function) — same threading contract as
// AssetResolver and TextureResolver: owned by the consumer; the SDK never
// opens files or calls a font API; zero-overhead default (null) is illegal
// to call, so use makeMonospaceStub() as the SDK-internal default.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Backend adapter contract (T-TEXT genericity proof, ADR-0025).
// A FontMetrics backend that wants to be swap-test-interchangeable MUST:
//   1. Read metrics UNSCALED (no hinting/grid-fitting): the advance is the raw
//      `hmtx` advance in font units.
//   2. Read `unitsPerEm` as the raw uint16 from the `head` table and return
//      advanceEm = float(advance) / float(unitsPerEm). (Do NOT derive em-size
//      from a float pixel-scale reciprocal — that inserts a rounding step and
//      breaks bit-exact parity across backends.)
//   3. Return makeFailed() when the codepoint maps to glyph index 0 (.notdef):
//      glyph 0 carries a valid advance, so without this rule a missing glyph
//      would read as Ready instead of Failed.
//   4. Set hasAtlasUv = false: the atlas/raster path is embedder-defined and
//      is not part of the cross-backend equality surface.
// v1 backends honor PLAIN style only; other styles return Failed.
// ---------------------------------------------------------------------------
using FontMetrics = std::function<GlyphResult(const FontKey&)>;

// ---------------------------------------------------------------------------
// makeMonospaceStub() — default FontMetrics: always Ready, advanceEm = 0.6f,
// no atlas UV.
//
// 0.6 is a reasonable fixed-pitch approximation for a typical sans-serif
// em-advance ratio.  It is NOT a spec value — it is an explicitly documented
// stub constant, chosen so layout math (line pitch, maxExtent compression,
// per-line length) is numerically testable without any real font data.
//
// The layout engine uses this as the default when no FontMetrics is wired.
// ---------------------------------------------------------------------------
inline FontMetrics makeMonospaceStub() {
  return [](const FontKey&) -> GlyphResult {
    return GlyphResult::makeReady(GlyphMetrics{0.6f, false, 0.f, 0.f, 0.f, 0.f});
  };
}

} // namespace x3d::runtime::extract
#endif // X3D_RUNTIME_EXTRACT_FONT_METRICS_HPP
