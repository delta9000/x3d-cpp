// linepoint_style_test.cpp — SEAM-LINEPOINT: §12.4.6 LineProperties /
// §12.4.8 PointProperties now reach the seam (MaterialDesc::line / ::point) and
// the line/point raster honours them. Before the fix every IndexedLineSet drew
// 1px and every PointSet a single pixel regardless of the authored properties.
#include "MaterialSystem.hpp"
#include "cpuraster/Framebuffer.hpp"
#include "cpuraster/Rasterizer.hpp"
#include "cpuraster/glsl.hpp"
#include "x3d/nodes/X3DNode.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"

#include <any>
#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

using namespace x3d::cpuraster;
namespace g = x3d::cpuraster::glsl;
namespace ex = x3d::runtime::extract;
using namespace x3d::core;
using namespace x3d::nodes;

static int failures = 0;
#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::fprintf(stderr, "FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);     \
      ++failures;                                                              \
    }                                                                          \
  } while (0)

static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) {
      f.set(*n, std::move(v));
      return;
    }
}

// Count pixels that differ from a black clear.
static int covered(Framebuffer &fb) {
  int n = 0;
  for (int y = 0; y < fb.height(); ++y)
    for (int x = 0; x < fb.width(); ++x) {
      g::vec4 c = fb.colorAt(x, y);
      if (c.x + c.y + c.z > 0.05f) ++n;
    }
  return n;
}

static std::vector<Vertex> hline() {
  return {
      {{-0.5f, -0.5f, 0.0f}, {0, 0, 1}, {1, 1, 1, 1}, {0, 0}},
      {{ 0.5f, -0.5f, 0.0f}, {0, 0, 1}, {1, 1, 1, 1}, {0, 0}},
  };
}

int main() {
  const g::mat4 I = g::mat4::identity();

  // ---- Line width: a wider line covers more pixels -----------------------
  {
    Framebuffer fb1(32, 32);
    fb1.clear({0, 0, 0});
    Rasterizer r1(fb1);
    r1.drawLines(hline(), {0, 1}, I, I, I, {1, 0, 0, 1}, false, /*lineWidth=*/1.0f);
    const int thin = covered(fb1);

    Framebuffer fb5(32, 32);
    fb5.clear({0, 0, 0});
    Rasterizer r5(fb5);
    r5.drawLines(hline(), {0, 1}, I, I, I, {1, 0, 0, 1}, false, /*lineWidth=*/5.0f);
    const int thick = covered(fb5);

    std::fprintf(stderr, "line px: w=1 -> %d, w=5 -> %d\n", thin, thick);
    CHECK(thin > 0);
    CHECK(thick > thin * 3); // ~5x, allow slack.
  }

  // ---- Point size + attenuation: a larger point covers more pixels --------
  {
    std::vector<Vertex> pt = {{{0, 0, 0}, {0, 0, 1}, {1, 1, 1, 1}, {0, 0}}};
    Framebuffer fb1(32, 32);
    fb1.clear({0, 0, 0});
    Rasterizer r1(fb1);
    r1.drawPoints(pt, {0}, I, I, I, {0, 1, 0, 1}, false, /*scale=*/1.0f,
                  /*atten=*/{1, 0, 0}, /*min=*/1.0f, /*max=*/1.0f);
    const int one = covered(fb1);

    Framebuffer fb7(32, 32);
    fb7.clear({0, 0, 0});
    Rasterizer r7(fb7);
    // scale 7 with min/max wide enough to pass: 7x7 = 49 px.
    r7.drawPoints(pt, {0}, I, I, I, {0, 1, 0, 1}, false, /*scale=*/7.0f,
                  /*atten=*/{1, 0, 0}, /*min=*/1.0f, /*max=*/64.0f);
    const int big = covered(fb7);

    // Clamp: scale 7 but max=3 -> 3x3 = 9 px.
    Framebuffer fbc(32, 32);
    fbc.clear({0, 0, 0});
    Rasterizer rc(fbc);
    rc.drawPoints(pt, {0}, I, I, I, {0, 1, 0, 1}, false, /*scale=*/7.0f,
                  /*atten=*/{1, 0, 0}, /*min=*/1.0f, /*max=*/3.0f);
    const int clamped = covered(fbc);

    std::fprintf(stderr, "point px: s=1 -> %d, s=7 -> %d, s=7 clamp3 -> %d\n",
                 one, big, clamped);
    CHECK(one == 1);
    CHECK(big == 49);
    CHECK(clamped == 9);
  }

  // ---- Extraction: LineProperties / PointProperties surface on MaterialDesc
  {
    auto lp = createX3DNode("LineProperties");
    setF(lp, "linewidthScaleFactor", std::any(4.0f));
    setF(lp, "applied", std::any(true));
    auto pp = createX3DNode("PointProperties");
    setF(pp, "pointSizeScaleFactor", std::any(2.5f));
    setF(pp, "pointSizeMinValue", std::any(1.0f));
    setF(pp, "pointSizeMaxValue", std::any(9.0f));
    setF(pp, "attenuation", std::any(SFVec3f{1.0f, 0.1f, 0.0f}));
    auto app = createX3DNode("Appearance");
    setF(app, "lineProperties", std::any(std::shared_ptr<X3DNode>(lp)));
    setF(app, "pointProperties", std::any(std::shared_ptr<X3DNode>(pp)));

    ex::MaterialDesc m = ex::materialOf(app.get());
    CHECK(m.line.applied == true);
    CHECK(std::fabs(m.line.linewidthScaleFactor - 4.0f) < 1e-4f);
    CHECK(std::fabs(m.point.pointSizeScaleFactor - 2.5f) < 1e-4f);
    CHECK(std::fabs(m.point.pointSizeMinValue - 1.0f) < 1e-4f);
    CHECK(std::fabs(m.point.pointSizeMaxValue - 9.0f) < 1e-4f);
    CHECK(std::fabs(m.point.attenuation.y - 0.1f) < 1e-4f);
  }

  // ---- Extraction defaults: no LineProperties/PointProperties => identity ----
  {
    auto app = createX3DNode("Appearance");
    ex::MaterialDesc m = ex::materialOf(app.get());
    CHECK(m.line.linewidthScaleFactor == 0.0f);
    CHECK(m.point.pointSizeScaleFactor == 1.0f);
    CHECK(m.point.pointSizeMinValue == 1.0f && m.point.pointSizeMaxValue == 1.0f);
  }

  // Authored line/point normals reach the fragment shader rather than the
  // constant-color path (§11.2.2.5).
  {
    FragmentShader lit = [](const FragmentInput &f, g::vec4 &out) {
      out = f.normalEye.z > 0.5f ? g::vec4{0, 1, 0, 1}
                                 : g::vec4{1, 0, 0, 1};
      return true;
    };
    Framebuffer lineFb(32, 32);
    lineFb.clear({0, 0, 0});
    Rasterizer lineRaster(lineFb);
    lineRaster.drawLines(hline(), {0, 1}, I, I, I, {1, 0, 0, 1}, false,
                         1.0f, lit);
    CHECK(lineFb.colorAt(16, 8).y > 0.9f);

    Framebuffer pointFb(32, 32);
    pointFb.clear({0, 0, 0});
    Rasterizer pointRaster(pointFb);
    const std::vector<Vertex> point = {{{0, 0, 0}, {0, 0, 1},
                                        {1, 1, 1, 1}, {0, 0}}};
    pointRaster.drawPoints(point, {0}, I, I, I, {1, 0, 0, 1}, false,
                           1.0f, {1, 0, 0}, 1.0f, 1.0f, lit);
    CHECK(pointFb.colorAt(16, 16).y > 0.9f);
  }

  if (failures) {
    std::fprintf(stderr, "linepoint_style_test: %d failure(s)\n", failures);
    return 1;
  }
  std::printf("linepoint_style_test: OK\n");
  return 0;
}
