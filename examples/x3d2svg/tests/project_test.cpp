// examples/x3d2svg/tests/project_test.cpp
//
// Unit tests for the x3d2svg projection core. Pure CPU, no display, no assets
// beyond the bundled smoke scene. Uses plain asserts (no doctest dependency) so
// the example stays as dependency-light as the tool it tests. Registered as
// ctest `x3d_x3d2svg_project`.
#include "x3d2svg.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <string>

namespace sdk = x3d::sdk;
using namespace x3d2svg;

#ifndef X3D2SVG_ASSET_DIR
#error "X3D2SVG_ASSET_DIR must be defined by the build"
#endif

static int failures = 0;
#define CHECK(cond, msg)                                                       \
  do {                                                                         \
    if (!(cond)) { std::fprintf(stderr, "FAIL: %s\n", msg); ++failures; }      \
  } while (0)

// A point dead ahead of a fit camera projects to the image centre.
static void test_fit_centres_target() {
  sdk::Aabb b;
  b.expand({-1, -1, -1});
  b.expand({1, 1, 1});
  Camera cam = fitCamera(b, 0.7854);
  Vec3 c = cam.toView({0, 0, 0});             // bounds centre
  CHECK(c.z < 0, "fit: target must be in front of camera (view z < 0)");
  // The centre of the bounds maps to (0,0) in view space's x/y (dead ahead).
  CHECK(std::fabs(c.x) < 1e-6 && std::fabs(c.y) < 1e-6,
        "fit: bounds centre must be on the view axis");
}

// A camera's basis is orthonormal, so view-space distances match world-space.
static void test_fit_preserves_distance() {
  sdk::Aabb b;
  b.expand({0, 0, 0});
  b.expand({4, 2, 6});
  Camera cam = fitCamera(b, 0.7854);
  Vec3 w0{1, 1, 1}, w1{2, 3, 1};
  double dw = std::sqrt(dot(sub(w1, w0), sub(w1, w0)));
  Vec3 v0 = cam.toView(w0), v1 = cam.toView(w1);
  double dv = std::sqrt(dot(sub(v1, v0), sub(v1, v0)));
  CHECK(std::fabs(dw - dv) < 1e-6, "fit: rigid transform must preserve distance");
}

// End-to-end on the bundled scene: three primitives extract, project, and land
// inside the viewport with plausible facet counts.
static void test_smoke_scene_projects() {
  const std::string scene = std::string(X3D2SVG_ASSET_DIR) + "/smoke.x3d";
  auto session = sdk::RuntimeSession::create(sdk::parseFile(scene));
  auto &ex = session->extractor();
  session->fullSnapshot();
  session->tick(0.0);
  session->delta();

  CHECK(session->context().boundViewpoint() != nullptr,
        "smoke: scene should bind its Viewpoint");
  long items = session->fullSnapshot().added.size();
  CHECK(items == 3, "smoke: expected 3 render items (box, sphere, cone)");

  Camera cam;
  cam.useMatrix = true;
  cam.view = ex.camera().viewMatrix;
  cam.fov = ex.camera().fieldOfView;

  const int W = 400, H = 300;
  auto facets = collect(ex, cam, W, H);
  CHECK(facets.size() > 50, "smoke: bound-viewpoint render should produce facets");

  int inView = 0;
  for (const auto &f : facets)
    for (int j = 0; j < 3; ++j)
      if (f.sx[j] >= 0 && f.sx[j] <= W && f.sy[j] >= 0 && f.sy[j] <= H) ++inView;
  CHECK(inView > 0, "smoke: some projected vertices must land inside the viewport");

  // Painter's order: facets sorted far-to-near (non-increasing depth).
  for (size_t i = 1; i < facets.size(); ++i)
    CHECK(facets[i - 1].depth >= facets[i].depth - 1e-9,
          "smoke: facets must be sorted far-to-near");

  // A forced fit also renders (and exercises the fit path on real geometry).
  Camera fit = fitCamera(ex.sceneWorldBounds(), cam.fov);
  CHECK(!collect(ex, fit, W, H).empty(), "smoke: view-all fit should also render");
}

// The SVG writer emits well-formed, non-empty output.
static void test_svg_emission() {
  std::vector<Facet> facets(1);
  facets[0] = Facet{{10, 20, 30}, {10, 30, 20}, 5.0, 0.8, 200, 100, 50, 1.0};
  std::ostringstream os;
  emitSvgHeader(os, 100, 100);
  emitGroup(os, facets, 0, 1, 30.0);
  os << "</svg>\n";
  std::string s = os.str();
  CHECK(s.find("<svg") != std::string::npos, "svg: header present");
  CHECK(s.find("<polygon") != std::string::npos, "svg: polygon emitted");
  CHECK(s.find("#a05028") != std::string::npos, "svg: shaded fill colour present");
}

int main() {
  test_fit_centres_target();
  test_fit_preserves_distance();
  test_smoke_scene_projects();
  test_svg_emission();
  if (failures == 0) std::printf("x3d2svg project_test: ALL PASS\n");
  return failures == 0 ? 0 : 1;
}
