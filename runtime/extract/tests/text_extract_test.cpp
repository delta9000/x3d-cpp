// text_extract_test.cpp — T-TEXT v1: Text node -> glyph render-items + outputs.
//
// Drives the integration layer (TextExtract.hpp) end-to-end:
//   * buildTextMesh() with the default monospace stub (advanceEm=0.6, no atlas
//     UV) produces one quad per glyph (4 positions / 6 indices each) at the
//     positions the §15 layout engine computes.
//   * setTextOutputs() writes textBounds / lineBounds / origin onto the Text
//     node's outputOnly fields (read back via the typed getters).
//   * A SceneExtractor full walk over a Shape>Text emits a glyph mesh and sets
//     the node outputs (the wired dispatch hook).
//   * An atlas-UV FontMetrics seam threads u0,v0,u1,v1 onto the glyph quads.
#include "TextExtract.hpp"

#include "SceneExtractor.hpp"
#include "X3DDocument.hpp"
#include "X3DExecutionContext.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DScene.hpp"

#include "x3d/nodes/Text.hpp"

#include <any>
#include "doctest/doctest.h"
#include <cmath>
#include <memory>
#include <string>
#include <vector>

using namespace x3d::runtime;
using namespace x3d::runtime::extract;

static bool feq(float a, float b, float eps = 1e-4f) {
  return std::fabs(a - b) < eps;
}
static void check(bool c, const char *m) {
  if (!c) { CHECK((c && m)); (void)m; }
}
static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}

// Build a Text node with the given strings and an optional FontStyle child.
static std::shared_ptr<X3DNode>
makeText(const std::vector<std::string> &strs,
         std::shared_ptr<X3DNode> fontStyle = nullptr) {
  auto text = createX3DNode("Text");
  setF(text, "string", std::any(std::vector<std::string>(strs)));
  if (fontStyle) setF(text, "fontStyle", std::any(std::shared_ptr<X3DNode>(fontStyle)));
  return text;
}

// ===========================================================================
// Test 1: single line "abc", default FontStyle, monospace stub.
//   advanceEm = 0.6, size = 1 => per-glyph advance = 0.6; natural = 1.8.
//   3 glyphs => 3 quads => 12 positions, 18 indices.
//   Default major BEGIN/leftToRight => pen starts at X=0, advances +X.
//   glyph 0 spans X[0,0.6], glyph 1 X[0.6,1.2], glyph 2 X[1.2,1.8].
//   Minor FIRST => baseline Y=0; cell Y[-0.2, 0.8].
// ===========================================================================
static void test_single_line_quads() {
  auto text = makeText({"abc"});
  TextLayoutResult layout;
  MeshData m = buildTextMesh(*text, makeMonospaceStub(), &layout);

  check(m.isGlyphMesh, "single: isGlyphMesh");
  check(!m.solid, "single: two-sided (solid=false)");
  check(m.topology == Topology::Triangles, "single: triangles");
  check(m.positions.size() == 12, "single: 12 positions (3 quads)");
  check(m.indices.size() == 18, "single: 18 indices (3 quads)");
  check(m.texcoords.size() == 12, "single: 12 texcoords");
  check(m.normals.size() == 12, "single: 12 normals");

  // glyph 0 bottom-left at (0, -0.2); top-right at (0.6, 0.8).
  check(feq(m.positions[0].x, 0.0f) && feq(m.positions[0].y, -0.2f),
        "single: glyph0 bottom-left");
  check(feq(m.positions[2].x, 0.6f) && feq(m.positions[2].y, 0.8f),
        "single: glyph0 top-right");
  // glyph 2 spans X[1.2, 1.8].
  check(feq(m.positions[8].x, 1.2f), "single: glyph2 left X");
  check(feq(m.positions[10].x, 1.8f), "single: glyph2 right X");
  // Z is always 0 (Text in the Z=0 plane).
  for (const auto &p : m.positions) check(feq(p.z, 0.0f), "single: Z=0 plane");

  // Layout outputs: width = natural = 1.8, height = size = 1.
  check(feq(layout.textBoundsX, 1.8f), "single: textBoundsX=1.8");
  check(feq(layout.textBoundsY, 1.0f), "single: textBoundsY=1.0");
  check(layout.lineBounds.size() == 1, "single: 1 lineBound");
  check(feq(layout.lineBounds[0].width, 1.8f), "single: lineBounds width");
}

// ===========================================================================
// Test 2: setTextOutputs writes the outputOnly fields onto the node.
// ===========================================================================
static void test_set_outputs() {
  auto text = makeText({"abc"});
  TextLayoutResult layout;
  (void)buildTextMesh(*text, makeMonospaceStub(), &layout);
  int written = setTextOutputs(*text, layout);
  check(written == 3, "outputs: textBounds+lineBounds+origin set");

  auto *tn = dynamic_cast<Text *>(text.get());
  check(tn != nullptr, "outputs: dynamic_cast Text");
  check(feq(tn->getTextBounds().x, 1.8f), "outputs: textBounds.x");
  check(feq(tn->getTextBounds().y, 1.0f), "outputs: textBounds.y");
  check(tn->getLineBounds().size() == 1, "outputs: lineBounds count");
  check(feq(tn->getLineBounds()[0].x, 1.8f), "outputs: lineBounds[0].x");
  // origin = upper-left of textBounds: X=0 (BEGIN/ltr), Y=ascender=0.8.
  check(feq(tn->getOrigin().x, 0.0f), "outputs: origin.x");
  check(feq(tn->getOrigin().y, 0.8f), "outputs: origin.y");
  check(feq(tn->getOrigin().z, 0.0f), "outputs: origin.z");
}

// ===========================================================================
// Test 3: two lines, FontStyle spacing=2 => line1 baseline at Y=-2.
//   "ab" (2 glyphs) + "cd" (2 glyphs) => 4 quads => 16 positions.
//   line0 baseline Y=0, line1 baseline Y=-2 (topToBottom default, FIRST minor).
// ===========================================================================
static void test_two_lines_spacing() {
  auto fontStyle = createX3DNode("FontStyle");
  setF(fontStyle, "spacing", std::any(2.0f));
  auto text = makeText({"ab", "cd"}, fontStyle);

  TextLayoutResult layout;
  MeshData m = buildTextMesh(*text, makeMonospaceStub(), &layout);

  check(m.positions.size() == 16, "two: 16 positions (4 quads)");
  // line0 glyph0 cell Y around baseline 0: bottom-left Y=-0.2.
  check(feq(m.positions[0].y, -0.2f), "two: line0 cell Y at baseline 0");
  // line1 is the 3rd quad (index 8): baseline Y=-2 => cell Y bottom -2.2.
  check(feq(m.positions[8].y, -2.2f), "two: line1 cell Y at baseline -2");
  // textBoundsY = (N-1)*spacing*size + size = 1*2 + 1 = 3.
  check(feq(layout.textBoundsY, 3.0f), "two: textBoundsY=3");
}

// ===========================================================================
// Test 4: SceneExtractor full walk over Shape>Text emits a glyph mesh and sets
// the Text node outputs (the wired dispatch hook).
// ===========================================================================
static void test_scene_extractor_text() {
  auto text = makeText({"hi"}); // 2 glyphs
  auto shape = createX3DNode("Shape");
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(text)));

  Scene scene;
  scene.addRootNode(shape);
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  SceneExtractor ex(ctx, scene); // default MeshBuildOptions => monospace stub.
  auto snap = ex.fullSnapshot();

  check(snap.added.size() == 1, "scene: one render item emitted");
  const auto &item = ex.item(snap.added[0]);
  check(item.mesh.isGlyphMesh, "scene: glyph mesh flagged");
  check(item.mesh.positions.size() == 8, "scene: 2 glyphs => 8 positions");
  check(item.mesh.indices.size() == 12, "scene: 2 glyphs => 12 indices");

  // The dispatch hook set the Text node outputs.
  auto *tn = dynamic_cast<Text *>(text.get());
  check(tn != nullptr, "scene: Text node");
  // "hi" = 2 glyphs * 0.6 = 1.2 natural width.
  check(feq(tn->getTextBounds().x, 1.2f), "scene: textBounds.x set on node");
  check(tn->getLineBounds().size() == 1, "scene: lineBounds set on node");
}

// ===========================================================================
// Test 5: atlas-UV FontMetrics seam threads u0,v0,u1,v1 onto the glyph quads.
// ===========================================================================
static void test_atlas_uv_seam() {
  // A seam that returns a fixed atlas rect for every glyph.
  FontMetrics atlas = [](const FontKey &) -> GlyphResult {
    GlyphMetrics gm;
    gm.advanceEm = 0.5f;
    gm.hasAtlasUv = true;
    gm.u0 = 0.1f; gm.v0 = 0.2f; gm.u1 = 0.3f; gm.v1 = 0.4f;
    return GlyphResult::makeReady(gm);
  };
  auto text = makeText({"a"}); // 1 glyph
  MeshData m = buildTextMesh(*text, atlas);

  check(m.positions.size() == 4, "atlas: 1 quad");
  // texcoords: bottom-left (u0,v0), bottom-right (u1,v0), top-right (u1,v1),
  //            top-left (u0,v1).
  check(feq(m.texcoords[0].x, 0.1f) && feq(m.texcoords[0].y, 0.2f),
        "atlas: BL uv");
  check(feq(m.texcoords[2].x, 0.3f) && feq(m.texcoords[2].y, 0.4f),
        "atlas: TR uv");
  // advance 0.5 => glyph spans X[0, 0.5].
  check(feq(m.positions[0].x, 0.0f), "atlas: glyph left X");
  check(feq(m.positions[1].x, 0.5f), "atlas: glyph right X");
}

// ===========================================================================
// Test 6: a Failed glyph is skipped (no quad, no advance).
// ===========================================================================
static void test_failed_glyph_skipped() {
  FontMetrics failing = [](const FontKey &) -> GlyphResult {
    return GlyphResult::makeFailed();
  };
  auto text = makeText({"xyz"});
  MeshData m = buildTextMesh(*text, failing);
  check(m.positions.empty(), "failed: no quads emitted");
  check(m.indices.empty(), "failed: no indices");
}

// LYT-1: a Text whose fontStyle is a ScreenFontStyle must honour pointSize.
// ScreenFontStyle has no 'size' field (only 'pointSize'), so the generic
// getField<float>("size") read silently defaulted to 1.0.
static void test_screenfontstyle_pointsize() {
  auto text = createX3DNode("Text");
  auto sfs = createX3DNode("ScreenFontStyle");
  setF(sfs, "pointSize", std::any(12.0f));
  setF(text, "fontStyle", std::any(std::shared_ptr<X3DNode>(sfs)));
  std::string fam, sty;
  FontStyleParams fs = readFontStyleParams(*text, fam, sty);
  check(feq(fs.size, 12.0f), "ScreenFontStyle.pointSize honoured as size");
}

TEST_CASE("text_extract_test") {
  test_single_line_quads();
  test_set_outputs();
  test_two_lines_spacing();
  test_scene_extractor_text();
  test_atlas_uv_seam();
  test_failed_glyph_skipped();
  test_screenfontstyle_pointsize();
  return;
}
