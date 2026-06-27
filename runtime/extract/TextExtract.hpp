// TextExtract.hpp — T-TEXT v1: Text node -> glyph render-items + Text outputs.
//
// namespace x3d::runtime::extract. Header-only, node-as-truth (reads the Text /
// FontStyle nodes via reflection; adds NO members to generated nodes). This is
// the INTEGRATION layer that binds the pure §15 layout engine (TextLayout.hpp)
// and the consumer-supplied font-metrics seam (FontMetrics.hpp) to the geometry
// extraction pipeline.
//
// WHAT IT DOES
// ============
//   1. Reads FontStyle (size/spacing/horizontal/leftToRight/topToBottom/justify/
//      family/style) and Text (string/length/maxExtent) off the nodes by
//      reflection — the FontStyle child via the Text node's `fontStyle` slot;
//      justify/family/style enums via the node-agnostic getEnumString seam.
//   2. Runs computeTextLayout() to place each line's baseline.
//   3. Walks each line glyph-by-glyph using the FontMetrics seam: every Ready
//      glyph with hasAtlasUv emits ONE textured quad (4 positions in the Text
//      local Z=0 plane + 4 atlas texcoords from GlyphMetrics{u0,v0,u1,v1} + 6
//      CCW indices). A glyph WITHOUT atlas UV (the monospace stub) still ADVANCES
//      the pen and emits a quad with degenerate (0,0)-(0,0) UVs so the layout is
//      testable with no real font (positions/advance are exact; the consumer
//      simply samples nothing). A Pending glyph is skipped (advance still steps,
//      retried next frame); a Failed glyph is skipped entirely.
//   4. Sets the Text node's outputOnly fields textBounds / lineBounds / origin
//      from the layout result, via the reflection `set` lambdas the generated
//      Text node exposes for its emit* outputs.
//
// The produced MeshData carries isGlyphMesh=true so a consumer selects the text
// shader branch; solid=false (Text is two-sided per §15.2.1.2); topology
// Triangles. With the default monospace stub (advanceEm=0.6, no atlas UV) the
// mesh has 4*G positions / 6*G indices for G non-space glyphs and the Text
// outputs are exact — so the extraction path is fully unit-testable IO-free.
//
// Spec: ISO/IEC 19775-1:2023 §15 Text/FontStyle (layout in TextLayout.hpp).
#ifndef X3D_RUNTIME_EXTRACT_TEXT_EXTRACT_HPP
#define X3D_RUNTIME_EXTRACT_TEXT_EXTRACT_HPP

#include "FontMetrics.hpp" // FontMetrics seam + makeMonospaceStub()
#include "GeometryBounds.hpp" // geombounds::getField/getNode/hasField
#include "RenderItem.hpp"     // MeshData
#include "TextLayout.hpp"     // computeTextLayout + FontStyleParams/TextParams
#include "x3d/nodes/X3DNode.hpp"
#include "x3d/core/X3Dtypes.hpp"

#include <algorithm>
#include <any>
#include <array>
#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

namespace x3d::runtime::extract {

using namespace x3d::core;

namespace text_detail {

// Read an SFEnum/MFEnum field's X3D token string via the node-agnostic
// getEnumString reflection seam (empty if absent / not an enum). This keeps
// TextExtract decoupled from the concrete FontFamilyValues/JustifyChoices/
// FontStyleChoices enum-class types in the generated bindings.
inline std::string enumTokens(const X3DNode &n, const char *name) {
  for (const auto &f : n.fields())
    if (f.x3dName == name && f.getEnumString)
      return f.getEnumString(n);
  return {};
}

// Split a token string on whitespace (justify is "MAJOR" or "MAJOR MINOR";
// to_string emits the slots separated by `" "` so quotes are also stripped).
inline std::vector<std::string> splitTokens(const std::string &s) {
  std::vector<std::string> out;
  std::string cur;
  for (char c : s) {
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == ',' ||
        c == '"') {
      if (!cur.empty()) { out.push_back(cur); cur.clear(); }
    } else {
      cur.push_back(c);
    }
  }
  if (!cur.empty()) out.push_back(cur);
  return out;
}

// Decode a UTF-8 byte string to UTF-32 codepoints. Malformed bytes degrade to
// the replacement (0xFFFD) so a glyph is still requested (the seam decides).
inline std::vector<std::uint32_t> utf8ToCodepoints(const std::string &s) {
  std::vector<std::uint32_t> cps;
  std::size_t i = 0;
  const std::size_t n = s.size();
  while (i < n) {
    const auto b0 = static_cast<unsigned char>(s[i]);
    std::uint32_t cp;
    std::size_t len;
    if (b0 < 0x80) { cp = b0; len = 1; }
    else if ((b0 >> 5) == 0x6) { cp = b0 & 0x1F; len = 2; }
    else if ((b0 >> 4) == 0xE) { cp = b0 & 0x0F; len = 3; }
    else if ((b0 >> 3) == 0x1E) { cp = b0 & 0x07; len = 4; }
    else { cps.push_back(0xFFFD); ++i; continue; }
    if (i + len > n) { cps.push_back(0xFFFD); break; }
    bool ok = true;
    for (std::size_t k = 1; k < len; ++k) {
      const auto bk = static_cast<unsigned char>(s[i + k]);
      if ((bk >> 6) != 0x2) { ok = false; break; }
      cp = (cp << 6) | (bk & 0x3F);
    }
    if (!ok) { cps.push_back(0xFFFD); ++i; continue; }
    cps.push_back(cp);
    i += len;
  }
  return cps;
}

} // namespace text_detail

// ---------------------------------------------------------------------------
// readFontStyleParams — pull FontStyleParams + the resolved (family,style) off
// the Text node's fontStyle child. A NULL fontStyle uses the spec defaults
// (§15.4.1: SERIF/PLAIN, size 1, spacing 1, horizontal/leftToRight/topToBottom
// all TRUE, justify {BEGIN, FIRST}). `family`/`style` out-params resolve the
// FontMetrics FontKey lookup (first listed family wins; empty style => PLAIN).
// ---------------------------------------------------------------------------
inline FontStyleParams readFontStyleParams(const X3DNode &textNode,
                                           std::string &familyOut,
                                           std::string &styleOut) {
  FontStyleParams fs; // spec defaults
  familyOut = "SERIF";
  styleOut = "PLAIN";

  auto fsNode = geombounds::getNode(textNode, "fontStyle");
  if (!fsNode) return fs;

  // ScreenFontStyle (§36.4.4) sizes text in points via `pointSize`, not `size`.
  fs.size = (fsNode->nodeTypeName() == "ScreenFontStyle")
                ? geombounds::getField<float>(*fsNode, "pointSize", 1.0f)
                : geombounds::getField<float>(*fsNode, "size", 1.0f);
  fs.spacing = geombounds::getField<float>(*fsNode, "spacing", 1.0f);
  fs.horizontal = geombounds::getField<bool>(*fsNode, "horizontal", true);
  fs.leftToRight = geombounds::getField<bool>(*fsNode, "leftToRight", true);
  fs.topToBottom = geombounds::getField<bool>(*fsNode, "topToBottom", true);

  // justify is the combined MAJOR[ MINOR] enum token (§15.2.2.3 justify[2]).
  const auto jt = text_detail::splitTokens(
      text_detail::enumTokens(*fsNode, "justify"));
  if (!jt.empty()) fs.justifyMajor = jt[0];
  if (jt.size() >= 2) fs.justifyMinor = jt[1];
  // ("" slots fall back to the slot default inside computeTextLayout.)

  // family is an MFString of preferences; the first listed wins.
  const auto fam = text_detail::splitTokens(
      text_detail::enumTokens(*fsNode, "family"));
  if (!fam.empty()) familyOut = fam[0];

  const auto sty = text_detail::splitTokens(
      text_detail::enumTokens(*fsNode, "style"));
  if (!sty.empty()) styleOut = sty[0];

  return fs;
}

// ---------------------------------------------------------------------------
// readTextParams — pull TextParams (string/length/maxExtent) off the Text node.
// ---------------------------------------------------------------------------
inline TextParams readTextParams(const X3DNode &textNode) {
  TextParams t;
  t.strings =
      geombounds::getField<std::vector<std::string>>(textNode, "string", {});
  t.length = geombounds::getField<std::vector<float>>(textNode, "length", {});
  t.maxExtent = geombounds::getField<float>(textNode, "maxExtent", 0.0f);
  return t;
}

// ---------------------------------------------------------------------------
// makeLayoutMetricsAdapter — bridge the per-codepoint FontMetrics seam to the
// per-LINE (string,size)->(advance,ascender,descender) callback TextLayout
// needs. The natural advance of a line is the sum of each codepoint's
// advanceEm*size (Pending/Failed glyphs contribute 0 advance — they do not
// occupy major-axis space). ascender/descender are fixed proportions of size
// (0.8 / -0.2): the seam exposes no per-line vertical metrics in v1, and these
// match the monospace-stub test contract.
// ---------------------------------------------------------------------------
inline FontMetricsCallback makeLayoutMetricsAdapter(const FontMetrics &fm,
                                                    const std::string &family,
                                                    const std::string &style) {
  return [&fm, family, style](const std::string &s,
                              float size) -> std::tuple<float, float, float> {
    float advance = 0.0f;
    for (std::uint32_t cp : text_detail::utf8ToCodepoints(s)) {
      const GlyphResult g = fm(FontKey{family, style, cp});
      if (g.ready()) advance += g.metrics.advanceEm * size;
    }
    return {advance, 0.8f * size, -0.2f * size};
  };
}

// ---------------------------------------------------------------------------
// buildTextMesh — produce a glyph-quad MeshData for `textNode`.
//
// Layout (line baselines, bounds, origin) comes from computeTextLayout(); the
// per-line baseline origin gives the pen start, and each glyph advances the pen
// along the major axis (X for horizontal, Y for vertical) by advanceEm*size.
// length[]/maxExtent compression is applied by scaling the per-glyph advance so
// the rendered line fits its effective length (lineBounds[i].width for
// horizontal, .height for vertical).
//
// Every NON-Pending/NON-Failed glyph emits one quad of height = size and width =
// scaled advance. The quad spans the baseline-to-ascender box: for horizontal
// text [penMajor, penMajor+adv] x [baseline-descender? ..]. v1 uses the simple
// glyph cell [pen, pen+adv] on the major axis and [baseline+descender,
// baseline+ascender]≈[baseline, baseline+size] on the minor — exact for the
// stub and a faithful textured-quad cell for a real atlas.
//
// `outLayout` (optional) receives the TextLayoutResult so the caller can set the
// Text node outputs without recomputing.
// ---------------------------------------------------------------------------
inline MeshData buildTextMesh(const X3DNode &textNode, const FontMetrics &fm,
                              TextLayoutResult *outLayout = nullptr) {
  MeshData mesh;
  mesh.isGlyphMesh = true;
  mesh.topology = Topology::Triangles;
  mesh.solid = false; // Text is two-sided (§15.2.1.2, solid default FALSE).
  mesh.ccw = true;

  std::string family, style;
  const FontStyleParams fs = readFontStyleParams(textNode, family, style);
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
        // minor (Y) cell: baseline+descender .. baseline+ascender ≈ size tall.
        const float y0 = baseY - 0.2f * size; // descender
        const float y1 = baseY + 0.8f * size; // ascender
        emitQuad(m0, y0, m1, y1, u0, v0, u1, v1);
      } else {
        // vertical: major axis is Y, the cell spans the column width on X.
        const float x0 = baseX - 0.2f * size;
        const float x1 = baseX + 0.8f * size;
        emitQuad(x0, m0, x1, m1, u0, v0, u1, v1);
      }
      pen += dir * a;
    }
  }

  mesh.hasNormals = !mesh.normals.empty();
  mesh.hasColors = false;
  return mesh;
}

// ---------------------------------------------------------------------------
// setTextOutputs — write the layout result back onto the Text node's outputOnly
// fields (textBounds: SFVec2f, lineBounds: MFVec2f, origin: SFVec3f) via the
// reflection `set` lambdas the generated node exposes for its emit* outputs.
// §15.4.2: these outputs are generated when length[] and maxExtent are default
// and on redraws; we always populate them (a superset — harmless when set).
// Returns the number of output fields successfully written.
// ---------------------------------------------------------------------------
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
