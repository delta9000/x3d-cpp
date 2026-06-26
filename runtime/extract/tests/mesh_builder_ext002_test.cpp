// mesh_builder_ext002_test.cpp — EXT-002: with colorPerVertex=FALSE, fan/strip
// sets apply ONE color per FAN/STRIP (§11.3.2/§11.4.13/§11.4.15), not per triangle.
#include "MeshBuilder.hpp"
#include "X3DNodeFactory.hpp"
#include <any>
#include "doctest/doctest.h"
#include <iostream>
#include <memory>
#include <vector>

using namespace x3d::runtime;
using namespace x3d::runtime::extract;

static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields()) if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}
static bool isRed(const SFColorRGBA &c)   { return c.r > 0.5f && c.g < 0.5f; }
static bool isGreen(const SFColorRGBA &c) { return c.g > 0.5f && c.r < 0.5f; }

static std::shared_ptr<X3DNode> color2() {
  auto col = createX3DNode("Color");
  setF(col, "color", std::any(std::vector<SFColor>{{1, 0, 0}, {0, 1, 0}}));
  return col;
}
static std::shared_ptr<X3DNode> coord(int n) {
  auto c = createX3DNode("Coordinate");
  std::vector<SFVec3f> p;
  for (int i = 0; i < n; ++i) p.push_back(SFVec3f{float(i), float(i % 2), 0});
  setF(c, "point", std::any(p));
  return c;
}

TEST_CASE("mesh_builder_ext002_test") {
  // TriangleFanSet: fanCount [3,4] -> fan0 = 1 tri (3 verts), fan1 = 2 tris (6 verts).
  {
    auto g = createX3DNode("TriangleFanSet");
    setF(g, "coord", std::any(std::shared_ptr<X3DNode>(coord(7))));
    setF(g, "color", std::any(std::shared_ptr<X3DNode>(color2())));
    setF(g, "colorPerVertex", std::any(SFBool{false}));
    setF(g, "fanCount", std::any(std::vector<int>{3, 4}));
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.colors.size() == 9));
    for (int i = 0; i < 3; ++i) CHECK((isRed(m.colors[i])));   // fan0 -> color[0]
    for (int i = 3; i < 9; ++i) CHECK((isGreen(m.colors[i]))); // fan1 -> color[1] (both tris)
    std::cout << "ok: TriangleFanSet colorPerVertex=FALSE -> one color per fan\n";
  }
  // IndexedTriangleStripSet: index [0..3,-1,4..6] -> strip0 = 2 tris, strip1 = 1 tri.
  {
    auto g = createX3DNode("IndexedTriangleStripSet");
    setF(g, "coord", std::any(std::shared_ptr<X3DNode>(coord(7))));
    setF(g, "color", std::any(std::shared_ptr<X3DNode>(color2())));
    setF(g, "colorPerVertex", std::any(SFBool{false}));
    setF(g, "index", std::any(std::vector<int>{0, 1, 2, 3, -1, 4, 5, 6}));
    MeshData m = buildLocalMesh(g.get());
    CHECK((m.colors.size() == 9));
    for (int i = 0; i < 6; ++i) CHECK((isRed(m.colors[i])));   // strip0 (2 tris) -> color[0]
    for (int i = 6; i < 9; ++i) CHECK((isGreen(m.colors[i]))); // strip1 (1 tri) -> color[1]
    std::cout << "ok: IndexedTriangleStripSet colorPerVertex=FALSE -> one color per strip\n";
  }
  std::cout << "all EXT-002 tests passed\n";
  return;
}
