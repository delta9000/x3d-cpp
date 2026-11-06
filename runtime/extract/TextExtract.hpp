// Bind the §15 layout engine to glyph meshes and Text output fields.
// Text meshes are two-sided (§15.2.1.2). Pending glyphs are retried each frame;
// failed glyphs are skipped.
#ifndef X3D_RUNTIME_EXTRACT_TEXT_EXTRACT_HPP
#define X3D_RUNTIME_EXTRACT_TEXT_EXTRACT_HPP

#include "FontMetrics.hpp"
#include "GeometryBounds.hpp"
#include "RenderItem.hpp"
#include "TextLayout.hpp"
#include "FieldRead.hpp"
#include "x3d/nodes/X3DNode.hpp"
#include "x3d/core/X3Dtypes.hpp"

#include <algorithm>
#include <any>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace x3d::runtime::extract {

using namespace x3d::core;

namespace text_detail {
using geombounds::utf8ToCodepoints;
} // namespace text_detail

using geombounds::readFontStyleParams;
using geombounds::readTextParams;
using geombounds::makeLayoutMetricsAdapter;

// buildTextMesh — produce a glyph-quad MeshData for `textNode`.
//
// Layout (line baselines, bounds, origin) comes from computeTextLayout(); the
// per-line baseline origin gives the pen start, and each glyph advances the pen
// along the major axis (X for horizontal, Y for vertical) by advanceEm*size.
// length[]/maxExtent compression is applied by scaling the per-glyph advance so
// the rendered line fits its effective length (lineBounds[i].width for
// horizontal, .height for vertical).
//
// Every NON-Pending/non-Failed glyph emits one quad of height = size and width =
// scaled advance. The quad spans the baseline-to-ascender box: for horizontal
// text uses the simple
// glyph cell [pen, pen+adv] on the major axis and [baseline+descender,
// baseline+ascender]≈[baseline, baseline+size] on the minor — exact for the
// stub and a faithful textured-quad cell for a real atlas.
//
// `outLayout` (optional) receives the TextLayoutResult so the caller can set the
// Text node outputs without recomputing.
inline MeshData buildTextMesh(const X3DNode &textNode, const FontMetrics &fm,
                              TextLayoutResult *outLayout = nullptr) {
  MeshData mesh;
  mesh.isGlyphMesh = true;
  mesh.topology = Topology::Triangles;
  mesh.solid = false; // Text is two-sided (§15.2.1.2, solid default FALSE).
  mesh.ccw = true;

  std::string family, style;
  std::vector<std::string> families;
  const FontStyleParams fs = readFontStyleParams(textNode, family, style, &families);
  family = resolveFontFamily(families, style, fm); // §15.4.1 fallback list
  const TextParams t = readTextParams(textNode);

  const FontMetricsCallback lineMetrics =
      makeLayoutMetricsAdapter(fm, family, style);
  const TextLayoutResult layout = computeTextLayout(fs, t, lineMetrics);
  if (outLayout) *outLayout = layout;

  const float size = (fs.size > 0.0f) ? fs.size : 1.0f;

  // Emit one CCW quad (two triangles) in the Z=0 plane.
  //   corners listed (x0,y0)=bottom-left, (x1,y0)=bottom-right,
  //                   (x1,y1)=top-right,  (x0,y1)=top-left.
  //   UVs: (u0,v0) bottom-left .. (u1,v1) top-right (GL convention).
  const auto emitQuad = [&](float x0, float y0, float x1, float y1, float u0,
                            float v0, float u1, float v1) {
    const auto base = static_cast<std::uint32_t>(mesh.positions.size());
    mesh.positions.push_back(SFVec3f{x0, y0, 0.0f});
    mesh.positions.push_back(SFVec3f{x1, y0, 0.0f});
    mesh.positions.push_back(SFVec3f{x1, y1, 0.0f});
    mesh.positions.push_back(SFVec3f{x0, y1, 0.0f});
    mesh.texcoords.push_back(SFVec2f{u0, v0});
    mesh.texcoords.push_back(SFVec2f{u1, v0});
    mesh.texcoords.push_back(SFVec2f{u1, v1});
    mesh.texcoords.push_back(SFVec2f{u0, v1});
    // Two CCW triangles: (0,1,2) + (0,2,3).
    mesh.indices.push_back(base);
    mesh.indices.push_back(base + 1);
    mesh.indices.push_back(base + 2);
    mesh.indices.push_back(base);
    mesh.indices.push_back(base + 2);
    mesh.indices.push_back(base + 3);
    // Text faces +Z (front of the Z=0 plane); two-sided so normal is +Z.
    for (int k = 0; k < 4; ++k) mesh.normals.push_back(SFVec3f{0, 0, 1});
  };

  const std::size_t N = t.strings.size();
  for (std::size_t i = 0; i < N; ++i) {
    if (i >= layout.lineBaselineOrigins.size()) break;
    const float baseX = layout.lineBaselineOrigins[i][0];
    const float baseY = layout.lineBaselineOrigins[i][1];

    // Effective (post-length/maxExtent) major extent for this line.
    const float effective = fs.horizontal ? layout.lineBounds[i].width
                                          : layout.lineBounds[i].height;

    // Natural (unscaled) major extent = sum of glyph advances at `size`.
    const auto cps = text_detail::utf8ToCodepoints(t.strings[i]);
    float natural = 0.0f;
    std::vector<float> adv(cps.size(), 0.0f);
    std::vector<GlyphResult> gr;
    gr.reserve(cps.size());
    for (std::size_t c = 0; c < cps.size(); ++c) {
      GlyphResult g = fm(FontKey{family, style, cps[c]});
      adv[c] = g.ready() ? g.metrics.advanceEm * size : 0.0f;
      natural += adv[c];
      gr.push_back(g);
    }
    // Per-line glyph scale so the rendered advances sum to `effective`.
    const float glyphScale =
        (natural > 1e-9f) ? (effective / natural) : 1.0f;

    // Pen starts at the line baseline origin and advances along the major axis.
    float pen = fs.horizontal ? baseX : baseY;
    // Direction of advance along the major axis.
    const float dir =
        fs.horizontal ? (fs.leftToRight ? +1.0f : -1.0f)
                      : (fs.topToBottom ? -1.0f : +1.0f);

    for (std::size_t c = 0; c < cps.size(); ++c) {
      const GlyphResult &g = gr[c];
      const float a = adv[c] * glyphScale;
      if (g.failed() || g.pending()) {
        // Failed: no advance, no quad. Pending: keep the slot (advance) but no
        // quad (atlas not ready) so the line length is stable across frames.
        if (g.pending()) pen += dir * a;
        continue;
      }
      // Major-axis span of this glyph cell.
      float m0 = pen;
      float m1 = pen + dir * a;
      if (m1 < m0) std::swap(m0, m1); // keep x0<x1 / y0<y1 ordering.

      float u0 = 0.0f, v0 = 0.0f, u1 = 0.0f, v1 = 0.0f;
      if (g.metrics.hasAtlasUv) {
        u0 = g.metrics.u0; v0 = g.metrics.v0;
        u1 = g.metrics.u1; v1 = g.metrics.v1;
      }

      if (fs.horizontal) {
        if (g.metrics.hasGlyphBox) {
          const float x0 = pen + dir * g.metrics.bearingX * size * glyphScale;
          const float x1 = x0 + dir * g.metrics.sizeX * size * glyphScale;
          const float y0 = baseY + (g.metrics.top - g.metrics.sizeY) * size;
          const float y1 = baseY + g.metrics.top * size;
          emitQuad(std::min(x0, x1), y0, std::max(x0, x1), y1, u0, v0, u1, v1);
        } else {
          emitQuad(m0, baseY - 0.2f * size, m1, baseY + 0.8f * size,
                   u0, v0, u1, v1);
        }
      } else {
        if (g.metrics.hasGlyphBox) {
          const float x0 = baseX + g.metrics.bearingX * size;
          const float x1 = x0 + g.metrics.sizeX * size;
          const float y0 = pen + dir * g.metrics.top * size * glyphScale;
          const float y1 = y0 - dir * g.metrics.sizeY * size * glyphScale;
          emitQuad(std::min(x0, x1), std::min(y0, y1), std::max(x0, x1),
                   std::max(y0, y1), u0, v0, u1, v1);
        } else {
          emitQuad(baseX - 0.2f * size, m0, baseX + 0.8f * size, m1,
                   u0, v0, u1, v1);
        }
      }
      pen += dir * a;
    }
  }

  mesh.hasNormals = !mesh.normals.empty();
  mesh.hasColors = false;
  return mesh;
}

// setTextOutputs — write the layout result back onto the Text node's outputOnly
// fields (textBounds: SFVec2f, lineBounds: MFVec2f, origin: SFVec3f) via the
// reflection `set` lambdas the generated node exposes for its emit* outputs.
// §15.4.2: these outputs are generated when length[] and maxExtent are default
// and on redraws; we always populate them (a superset — harmless when set).
// Returns the number of output fields successfully written.
inline int setTextOutputs(X3DNode &textNode, const TextLayoutResult &layout) {
  int written = 0;
  const auto setField = [&](const char *name, const std::any &v) -> bool {
    for (auto &f : textNode.fields())
      if (f.x3dName == name && f.set) { f.set(textNode, v); return true; }
    return false;
  };

  if (setField("textBounds",
               std::any(SFVec2f{layout.textBoundsX, layout.textBoundsY})))
    ++written;

  MFVec2f lb;
  lb.reserve(layout.lineBounds.size());
  for (const auto &b : layout.lineBounds) lb.push_back(SFVec2f{b.width, b.height});
  if (setField("lineBounds", std::any(lb))) ++written;

  if (setField("origin", std::any(SFVec3f{layout.originX, layout.originY,
                                          layout.originZ})))
    ++written;

  return written;
}

} // namespace x3d::runtime::extract
#endif // X3D_RUNTIME_EXTRACT_TEXT_EXTRACT_HPP
