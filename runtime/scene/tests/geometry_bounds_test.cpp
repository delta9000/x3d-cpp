// geometry_bounds_test.cpp
#include "GeometryBounds.hpp"
#include "BoundsSystem.hpp"
#include "TransformSystem.hpp"
#include "TextExtract.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DScene.hpp"
#include "X3DDocument.hpp"
#include <any>
#include "doctest/doctest.h"
#include <cmath>
#include <memory>
#include <vector>
using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;
using namespace x3d::runtime::extract;
static bool feq(float a, float b) { return std::fabs(a - b) < 1e-4f; }
static void setF(const std::shared_ptr<X3DNode>& n, const char* name, std::any v) {
  for (auto& f : n->fields()) if (f.x3dName == name && f.set) { f.set(*n, std::move(v)); return; }
}

TEST_CASE("geometry_bounds_test") {
  auto box = createX3DNode("Box");
  setF(box, "size", std::any(SFVec3f{2,4,6}));
  Aabb b = localGeometryBounds(box.get());
  CHECK((feq(b.min.x,-1) && feq(b.max.y,2) && feq(b.max.z,3)));

  auto sph = createX3DNode("Sphere");
  setF(sph, "radius", std::any(2.5f));
  Aabb s = localGeometryBounds(sph.get());
  CHECK((feq(s.min.x,-2.5f) && feq(s.max.z,2.5f)));

  auto cyl = createX3DNode("Cylinder");
  setF(cyl, "radius", std::any(1.0f)); setF(cyl, "height", std::any(10.0f));
  Aabb c = localGeometryBounds(cyl.get());
  CHECK((feq(c.min.x,-1) && feq(c.max.y,5) && feq(c.min.y,-5)));

  // IndexedFaceSet with a Coordinate -> AABB over points
  auto ifs = createX3DNode("IndexedFaceSet");
  auto coord = createX3DNode("Coordinate");
  setF(coord, "point", std::any(std::vector<SFVec3f>{{0,0,0},{1,2,3},{-1,-1,0}}));
  setF(ifs, "coord", std::any(std::shared_ptr<X3DNode>(coord)));
  Aabb m = localGeometryBounds(ifs.get());
  CHECK((feq(m.min.x,-1) && feq(m.min.y,-1) && feq(m.max.y,2) && feq(m.max.z,3)));

  // unknown / empty geometry -> empty
  auto txt = createX3DNode("WorldInfo"); // not geometry
  CHECK((localGeometryBounds(txt.get()).empty));

  // ElevationGrid: x in [0,(xDim-1)*xSp], z in [0,(zDim-1)*zSp], y over height.
  auto eg = createX3DNode("ElevationGrid");
  setF(eg, "xDimension", std::any(3)); setF(eg, "zDimension", std::any(2));
  setF(eg, "xSpacing", std::any(2.0f)); setF(eg, "zSpacing", std::any(5.0f));
  setF(eg, "height", std::any(std::vector<float>{0,1,2,3,4,5}));
  Aabb g = localGeometryBounds(eg.get());
  CHECK((feq(g.min.x,0) && feq(g.max.x,4) && feq(g.max.z,5) && feq(g.min.y,0) && feq(g.max.y,5)));

  // Extrusion: conservative spine±maxSectionRadius. Square section radius ~1.414,
  // spine along y from 0..10.
  auto ex = createX3DNode("Extrusion");
  setF(ex, "crossSection", std::any(std::vector<SFVec2f>{{1,1},{-1,1},{-1,-1},{1,-1},{1,1}}));
  setF(ex, "spine", std::any(std::vector<SFVec3f>{{0,0,0},{0,10,0}}));
  Aabb x = localGeometryBounds(ex.get());
  CHECK((!x.empty && x.max.y >= 10.0f && x.max.x >= 1.0f && x.min.x <= -1.0f));

  // Text: non-empty, and shrinks to maxExtent when set.
  auto te = createX3DNode("Text");
  setF(te, "string", std::any(std::vector<std::string>{"hello","world"}));
  Aabb tb = localGeometryBounds(te.get());
  CHECK((!tb.empty));
  setF(te, "maxExtent", std::any(3.0f));
  Aabb tb2 = localGeometryBounds(te.get());
  CHECK((tb2.size().x <= 6.01f)); // width capped by 2*maxExtent (symmetric box)

  // GEO-2 sibling: a GeoCoordinate coord (MFVec3d) must yield real bounds, not
  // an empty AABB (it was read as MFVec3f and silently dropped).
  auto gifs = createX3DNode("IndexedFaceSet");
  auto gcoord = createX3DNode("GeoCoordinate");
  setF(gcoord, "point",
       std::any(std::vector<SFVec3d>{{0, 0, 0}, {2, 4, 6}, {-1, -1, 0}}));
  setF(gifs, "coord", std::any(std::shared_ptr<X3DNode>(gcoord)));
  Aabb gm = localGeometryBounds(gifs.get());
  CHECK((feq(gm.min.x, -1) && feq(gm.max.y, 4) && feq(gm.max.z, 6)));

  // GeoElevationGrid (SFDouble spacing, MFDouble height): bounds from the
  // double-precision grid, not the float default.
  auto geg = createX3DNode("GeoElevationGrid");
  setF(geg, "xDimension", std::any(3));
  setF(geg, "zDimension", std::any(2));
  setF(geg, "xSpacing", std::any(SFDouble{2.0}));
  setF(geg, "zSpacing", std::any(SFDouble{5.0}));
  setF(geg, "height", std::any(MFDouble{0, 1, 2, 3, 4, 5}));
  Aabb gg = localGeometryBounds(geg.get());
  CHECK((feq(gg.max.x, 4) && feq(gg.max.z, 5) && feq(gg.max.y, 5)));

  // NurbsCurve / NurbsPatchSurface: AABB over control points (convex hull).
  {
    auto cc = createX3DNode("Coordinate");
    setF(cc, "point", std::any(std::vector<SFVec3f>{{-2,0,0},{0,5,0},{3,0,1}}));
    auto nc = createX3DNode("NurbsCurve");
    setF(nc, "controlPoint", std::any(std::shared_ptr<X3DNode>(cc)));
    Aabb b = localGeometryBounds(nc.get());
    CHECK((feq(b.min.x,-2) && feq(b.max.x,3) && feq(b.max.y,5) && feq(b.max.z,1)));

    auto cs = createX3DNode("Coordinate");
    setF(cs, "point", std::any(std::vector<SFVec3f>{{0,0,0},{1,0,0},{0,1,4}}));
    auto np = createX3DNode("NurbsPatchSurface");
    setF(np, "controlPoint", std::any(std::shared_ptr<X3DNode>(cs)));
    Aabb p = localGeometryBounds(np.get());
    CHECK((feq(p.min.x,0) && feq(p.max.x,1) && feq(p.max.y,1) && feq(p.max.z,4)));
  }
  return;
}

// ===========================================================================
// Exact Text bounds via the FontMetrics seam (T-TEXT-D2 / M2B-1).
// A fake FontMetrics with known, non-uniform advances: 'W' -> 1.0 em, else 0.5.
// ===========================================================================
static FontMetrics fakeMetrics() {
  return [](const FontKey &k) -> GlyphResult {
    const float adv = (k.codepoint == 'W') ? 1.0f : 0.5f;
    return GlyphResult::makeReady(GlyphMetrics{adv, false, 0.f, 0.f, 0.f, 0.f});
  };
}
static Aabb aabbOfPositions(const MeshData &m) {
  Aabb a;
  for (const auto &p : m.positions) a.expand(p);
  return a;
}
static bool contains(const Aabb &outer, const Aabb &inner) {
  return !outer.empty && !inner.empty &&
         outer.min.x <= inner.min.x + 1e-4f && outer.min.y <= inner.min.y + 1e-4f &&
         outer.max.x >= inner.max.x - 1e-4f && outer.max.y >= inner.max.y - 1e-4f;
}

TEST_CASE("text_bounds_fontmetrics") {
  // Two-line Text, size=1 spacing=1, justify MIDDLE MIDDLE. Known advances:
  // line "W0" = 1.5, line "WW" = 2.0.
  auto text = createX3DNode("Text");
  setF(text, "string", std::any(std::vector<std::string>{"W0", "WW"}));
  auto fstyle = createX3DNode("FontStyle");
  setF(fstyle, "justify",
       std::any(std::vector<JustifyChoices>{JustifyChoices::MIDDLE_MIDDLE}));
  setF(text, "fontStyle", std::any(std::shared_ptr<X3DNode>(fstyle)));

  const FontMetrics fm = fakeMetrics();

  // Without metrics: the conservative heuristic is unchanged (pinned).
  Aabb h = localGeometryBounds(text.get());
  CHECK((!h.empty));
  CHECK((feq(h.min.x, -1.2f) && feq(h.max.x, 1.2f))); // longest=2 * size * 0.6
  CHECK((feq(h.min.y, -2.0f) && feq(h.max.y, 2.0f))); // 2 lines * size * spacing

  // With metrics: exact glyph extents (the layout's own extent).
  Aabb b = localGeometryBounds(text.get(), fm);
  CHECK((feq(b.min.x, -1.0f) && feq(b.max.x, 1.0f)));
  CHECK((feq(b.min.y, -1.0f) && feq(b.max.y, 1.0f)));

  // Culling safety: the bound must CONTAIN every rendered glyph.
  const MeshData mesh = buildTextMesh(*text, fm);
  CHECK((contains(b, aabbOfPositions(mesh))));

  // And it must equal the layout's glyph extent (not over-bound either).
  std::string family, style;
  const FontStyleParams fsp = readFontStyleParams(*text, family, style);
  const TextParams tp = readTextParams(*text);
  const TextLayoutResult layout =
      computeTextLayout(fsp, tp, makeLayoutMetricsAdapter(fm, family, style));
  const TextExtent2D ext = textLayoutExtent(fsp, layout);
  CHECK((feq(b.min.x, ext.minX) && feq(b.max.x, ext.maxX)));
  CHECK((feq(b.min.y, ext.minY) && feq(b.max.y, ext.maxY)));

  // BoundsSystem threads its injected FontMetrics into the Text bound.
  auto shape = createX3DNode("Shape");
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(text)));
  Scene scene;
  scene.addRootNode(shape);
  TransformSystem ts;
  ts.buildIndex(scene);
  BoundsSystem bs;
  bs.setFontMetrics(fakeMetrics());
  bs.buildBounds(scene, ts);
  Aabb s = bs.localBounds(shape.get());
  CHECK((feq(s.min.x, -1.0f) && feq(s.max.x, 1.0f)));
  CHECK((feq(s.min.y, -1.0f) && feq(s.max.y, 1.0f)));
  return;
}
