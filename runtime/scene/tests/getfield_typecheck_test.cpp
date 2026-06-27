// getfield_typecheck_test.cpp — readField() distinguishes a field that is
// absent (default is correct) from one that is present but of the wrong type
// (a caller-side contract violation that getField must surface, not hide).
#include "GeometryBounds.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "doctest/doctest.h"
#include <any>
#include <string>

using namespace x3d::runtime;
using namespace x3d::runtime::geombounds;
using namespace x3d::core;
using namespace x3d::nodes;

TEST_CASE("readField classifies absent / ok / type-mismatch") {
  auto box = createX3DNode("Box"); // Box.size is SFVec3f with a default present

  // Present, requested type matches -> Ok + value populated.
  SFVec3f sz{0, 0, 0};
  CHECK(readField(*box, "size", sz) == FieldRead::Ok);
  CHECK(sz.x == doctest::Approx(2.0f)); // default Box size is 2,2,2

  // Present, but requested type is wrong (SFVec3f field read as float).
  // This is the silent-default bug class (GEO-1/LYT-1) — must be detected.
  float wrong = -1.0f;
  CHECK(readField(*box, "size", wrong) == FieldRead::TypeMismatch);

  // Absent field -> Absent (so getField legitimately returns its default).
  std::string missing;
  CHECK(readField(*box, "noSuchField", missing) == FieldRead::Absent);
}

TEST_CASE("getVec3fLenient reads both SFVec3f and SFVec3d position fields") {
  // Regular Viewpoint.position is SFVec3f — read directly.
  auto vp = createX3DNode("Viewpoint");
  for (auto &f : vp->fields())
    if (f.x3dName == std::string("position") && f.set)
      f.set(*vp, std::any(SFVec3f{1, 2, 3}));
  SFVec3f a = getVec3fLenient(*vp, "position", {0, 0, 0});
  CHECK(a.x == doctest::Approx(1.0f));
  CHECK(a.z == doctest::Approx(3.0f));

  // GeoViewpoint.position is SFVec3d — must narrow, not silently read zero (GEO-1).
  auto gvp = createX3DNode("GeoViewpoint");
  for (auto &f : gvp->fields())
    if (f.x3dName == std::string("position") && f.set)
      f.set(*gvp, std::any(SFVec3d{10, 20, 30}));
  SFVec3f g = getVec3fLenient(*gvp, "position", {0, 0, 0});
  CHECK(g.x == doctest::Approx(10.0f));
  CHECK(g.y == doctest::Approx(20.0f));
  CHECK(g.z == doctest::Approx(30.0f));

  // GeoViewpoint.centerOfRotation is also SFVec3d (nav EXAMINE pivot, GEO-1 sibling).
  for (auto &f : gvp->fields())
    if (f.x3dName == std::string("centerOfRotation") && f.set)
      f.set(*gvp, std::any(SFVec3d{-1, -2, -3}));
  SFVec3f c = getVec3fLenient(*gvp, "centerOfRotation", {0, 0, 0});
  CHECK(c.x == doctest::Approx(-1.0f));
  CHECK(c.z == doctest::Approx(-3.0f));
}

TEST_CASE("getPointsLenient reads MFVec3f and MFVec3d coord points") {
  // GeoCoordinate.point is MFVec3d — must narrow to MFVec3f (GEO-2).
  auto gc = createX3DNode("GeoCoordinate");
  for (auto &f : gc->fields())
    if (f.x3dName == std::string("point") && f.set)
      f.set(*gc, std::any(std::vector<SFVec3d>{{1, 2, 3}, {-4, -5, -6}}));
  std::vector<SFVec3f> pts = getPointsLenient(*gc, "point");
  REQUIRE(pts.size() == 2);
  CHECK(pts[0].x == doctest::Approx(1.0f));
  CHECK(pts[1].z == doctest::Approx(-6.0f));
}

TEST_CASE("getFloatLenient / getFloatsLenient read SFFloat+SFDouble, MFFloat+MFDouble") {
  // ElevationGrid uses SFFloat/MFFloat; GeoElevationGrid uses SFDouble/MFDouble.
  auto eg = createX3DNode("ElevationGrid");
  for (auto &f : eg->fields())
    if (f.x3dName == std::string("xSpacing") && f.set)
      f.set(*eg, std::any(SFFloat{2.5f}));
  CHECK(getFloatLenient(*eg, "xSpacing", 1.0f) == doctest::Approx(2.5f));

  auto geg = createX3DNode("GeoElevationGrid");
  for (auto &f : geg->fields()) {
    if (f.x3dName == std::string("xSpacing") && f.set)
      f.set(*geg, std::any(SFDouble{4.0}));          // SFDouble field
    if (f.x3dName == std::string("height") && f.set)
      f.set(*geg, std::any(MFDouble{1.0, 3.0, -2.0})); // MFDouble field
  }
  CHECK(getFloatLenient(*geg, "xSpacing", 1.0f) == doctest::Approx(4.0f));
  std::vector<float> h = getFloatsLenient(*geg, "height");
  REQUIRE(h.size() == 3);
  CHECK(h[1] == doctest::Approx(3.0f));
  CHECK(h[2] == doctest::Approx(-2.0f));
}
