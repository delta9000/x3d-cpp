// text_render_test.cpp — Text glyph rendering. Proves the built-in font atlas
// makes Text render as LETTERS (sparse ink within the text's bounding box), not
// the solid filled bar the metrics-only SDK stub produces.
#include "RenderItem.hpp"
#include "SceneExtractor.hpp"
#include "X3DDocument.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DParse.hpp"
#include "X3DSceneBridge.hpp"

#include "cpuraster/BuiltinFont.hpp"
#include "cpuraster/Framebuffer.hpp"
#include "cpuraster/SceneRender.hpp"

#include <cstdio>
#include <string>

namespace ex = x3d::runtime::extract;
namespace cr = x3d::cpuraster;
namespace g = x3d::cpuraster::glsl;

static int failures = 0;
#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::fprintf(stderr, "FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);     \
      ++failures;                                                              \
    }                                                                          \
  } while (0)

// Fill ratio = ink pixels / bounding-box area of the ink (1.0 == solid block).
static double fillRatio(const cr::Framebuffer &fb, int &inkOut) {
  int minx = 1 << 30, miny = 1 << 30, maxx = -1, maxy = -1, ink = 0;
  for (int y = 0; y < fb.height(); ++y)
    for (int x = 0; x < fb.width(); ++x) {
      g::vec4 c = fb.colorAt(x, y);
      if (c.x + c.y + c.z > 0.15f) { // non-black background.
        ++ink;
        if (x < minx) minx = x;
        if (x > maxx) maxx = x;
        if (y < miny) miny = y;
        if (y > maxy) maxy = y;
      }
    }
  inkOut = ink;
  if (maxx < minx) return 0.0;
  double area = double(maxx - minx + 1) * double(maxy - miny + 1);
  return area > 0 ? ink / area : 0.0;
}

int main() {
  const std::string scene = std::string(X3D_CPURASTER_ASSET_DIR) + "/text_smoke.x3d";
  x3d::runtime::X3DDocument doc;
  try {
    doc = x3d::codec::parseFile(scene);
  } catch (const std::exception &e) {
    std::fprintf(stderr, "parse failed: %s\n", e.what());
    return 1;
  }
  x3d::runtime::Scene &sc = doc.getScene();
  x3d::runtime::X3DExecutionContext ctx;
  ctx.buildSceneGraph(sc);
  ctx.buildFrom(sc);
  ctx.tick(0.0);

  // Built-in font metrics into the extractor; atlas bound at draw.
  cr::BuiltinFont fontA = cr::makeBuiltinFont();
  ex::MeshBuildOptions opts;
  opts.fontMetrics = fontA.metrics;
  ex::SceneExtractor extractor(ctx, sc, opts);
  extractor.fullSnapshot();
  CHECK(extractor.itemCount() >= 1);

  bool anyGlyph = false;
  for (ex::RenderItemId id = 0; id < extractor.itemCount(); ++id)
    if (extractor.item(id).mesh.isGlyphMesh) anyGlyph = true;
  CHECK(anyGlyph); // Text produced glyph geometry.

  // WITH the atlas: letters -> sparse ink.
  cr::RenderOptions withFont;
  withFont.width = 256; withFont.height = 96;
  withFont.clearColor = {0, 0, 0};
  withFont.glyphAtlas = &fontA.atlas;
  cr::Framebuffer fbFont = cr::renderScene(ctx, extractor, withFont);
  int inkFont = 0;
  double ratioFont = fillRatio(fbFont, inkFont);

  // WITHOUT the atlas: the SDK-stub behavior -> solid filled cells (a bar).
  cr::RenderOptions noFont = withFont;
  noFont.glyphAtlas = nullptr;
  cr::Framebuffer fbBar = cr::renderScene(ctx, extractor, noFont);
  int inkBar = 0;
  double ratioBar = fillRatio(fbBar, inkBar);

  std::fprintf(stderr,
               "text: letters ink=%d fill=%.2f | solid-cells ink=%d fill=%.2f\n",
               inkFont, ratioFont, inkBar, ratioBar);

  CHECK(inkFont > 40);          // something rendered.
  CHECK(ratioBar > 0.7);        // stub fills the bbox nearly solid.
  CHECK(ratioFont < 0.55);      // letters leave gaps...
  CHECK(ratioFont < ratioBar - 0.2); // ...markedly sparser than the solid bar.
  CHECK(inkFont < inkBar);      // letters use less ink than filled cells.

  if (failures) { std::fprintf(stderr, "text_render_test: %d failure(s)\n", failures); return 1; }
  std::printf("text_render_test: OK\n");
  return 0;
}
