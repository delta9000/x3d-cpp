#include "doctest/doctest.h"
// interpolator_test.cpp
// Per-interpolator cascade tests for the interpolator-System family. Each test
// builds an interpolator node, registers its System (wiring the set_fraction
// handler), routes value_changed to a sink, then posts a fraction at 0, 0.5, 1,
// and one off-key value and asserts the emitted value reaches the sink. A final
// scene test drives several interpolators from one source fraction (fan-out) to
// prove they coexist in a single cascade.
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "InterpolatorSystem.hpp"
#include "Interpolation.hpp"
#include "X3DExecutionContext.hpp"

#include "x3d/nodes/ColorInterpolator.hpp"
#include "x3d/nodes/CoordinateInterpolator.hpp"
#include "x3d/nodes/CoordinateInterpolator2D.hpp"
#include "x3d/nodes/NormalInterpolator.hpp"
#include "x3d/nodes/OrientationInterpolator.hpp"
#include "x3d/nodes/PositionInterpolator.hpp"
#include "x3d/nodes/PositionInterpolator2D.hpp"
#include "x3d/nodes/ScalarInterpolator.hpp"

#include <any>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>

using namespace x3d;
using namespace x3d::runtime;
using namespace x3d::core;
using namespace x3d::nodes;

namespace {

int failures = 0;

void check(bool cond, const std::string &what) {
  if (!cond) {
    std::cerr << "FAIL: " << what << "\n";
    ++failures;
  } else {
    std::cout << "ok: " << what << "\n";
  }
}

constexpr float kEps = 1e-4f;
bool feq(float a, float b) { return std::fabs(a - b) <= kEps; }

// Drive one fraction through the interpolator and run the cascade. The
// interpolator's value_changed is read back off the node after the cascade
// settles (postEvent on value_changed updates the outputOnly field via the
// generated reflection set-thunk).
void post(X3DExecutionContext &ctx, X3DNode *interp, float fraction) {
  ctx.postEvent(interp, "set_fraction", std::any(SFFloat{fraction}));
  ctx.process();
}

// -- ScalarInterpolator -----------------------------------------------------
void test_scalar() {
  auto interp = std::make_shared<ScalarInterpolator>();
  interp->setKey(MFFloat{0.0f, 1.0f});
  interp->setKeyValue(MFFloat{0.0f, 10.0f});

  X3DExecutionContext ctx;
  InterpolatorSystem<ScalarInterpolator, float> sys(
      [](const float &a, const float &b, float t) { return lerpf(a, b, t); });
  sys.attach(interp.get(), ctx);
  // Read value_changed back off the node after the cascade settles.

  post(ctx, interp.get(), 0.0f);
  check(feq(interp->getValue_changed(), 0.0f), "scalar f=0 -> 0");
  post(ctx, interp.get(), 0.5f);
  check(feq(interp->getValue_changed(), 5.0f), "scalar f=0.5 -> 5");
  post(ctx, interp.get(), 1.0f);
  check(feq(interp->getValue_changed(), 10.0f), "scalar f=1 -> 10");
  post(ctx, interp.get(), 2.0f);
  check(feq(interp->getValue_changed(), 10.0f),
        "scalar f=2 (off-key) clamps to 10");
}

// -- PositionInterpolator ---------------------------------------------------
void test_position() {
  auto interp = std::make_shared<PositionInterpolator>();
  interp->setKey(MFFloat{0.0f, 1.0f});
  interp->setKeyValue(MFVec3f{SFVec3f{0, 0, 0}, SFVec3f{10, 20, 30}});

  X3DExecutionContext ctx;
  InterpolatorSystem<PositionInterpolator, SFVec3f> sys(
      [](const SFVec3f &a, const SFVec3f &b, float t) { return lerpVec3(a, b, t); });
  sys.attach(interp.get(), ctx);

  post(ctx, interp.get(), 0.5f);
  SFVec3f v = interp->getValue_changed();
  check(feq(v.x, 5) && feq(v.y, 10) && feq(v.z, 15),
        "position f=0.5 -> (5,10,15)");
  post(ctx, interp.get(), -1.0f);
  v = interp->getValue_changed();
  check(feq(v.x, 0) && feq(v.y, 0) && feq(v.z, 0),
        "position f=-1 (off-key) clamps to front");
}

// -- PositionInterpolator2D -------------------------------------------------
void test_position2d() {
  auto interp = std::make_shared<PositionInterpolator2D>();
  interp->setKey(MFFloat{0.0f, 1.0f});
  interp->setKeyValue(MFVec2f{SFVec2f{0, 0}, SFVec2f{4, 8}});

  X3DExecutionContext ctx;
  InterpolatorSystem<PositionInterpolator2D, SFVec2f> sys(
      [](const SFVec2f &a, const SFVec2f &b, float t) { return lerpVec2(a, b, t); });
  sys.attach(interp.get(), ctx);

  post(ctx, interp.get(), 0.25f);
  SFVec2f v = interp->getValue_changed();
  check(feq(v.x, 1) && feq(v.y, 2), "position2D f=0.25 -> (1,2)");
  post(ctx, interp.get(), 1.0f);
  v = interp->getValue_changed();
  check(feq(v.x, 4) && feq(v.y, 8), "position2D f=1 -> (4,8)");
}

// -- ColorInterpolator (HSV) ------------------------------------------------
void test_color() {
  auto interp = std::make_shared<ColorInterpolator>();
  // Red (hue 0) -> Green (hue 120). HSV midpoint is hue 60 = yellow (1,1,0).
  interp->setKey(MFFloat{0.0f, 1.0f});
  interp->setKeyValue(MFColor{SFColor{1, 0, 0}, SFColor{0, 1, 0}});

  X3DExecutionContext ctx;
  InterpolatorSystem<ColorInterpolator, SFColor> sys(
      [](const SFColor &a, const SFColor &b, float t) { return lerpColorHsv(a, b, t); });
  sys.attach(interp.get(), ctx);

  post(ctx, interp.get(), 0.0f);
  SFColor c = interp->getValue_changed();
  check(feq(c.r, 1) && feq(c.g, 0) && feq(c.b, 0), "color f=0 -> red");
  post(ctx, interp.get(), 0.5f);
  c = interp->getValue_changed();
  check(feq(c.r, 1) && feq(c.g, 1) && feq(c.b, 0),
        "color f=0.5 HSV midpoint red->green is yellow (1,1,0)");
  post(ctx, interp.get(), 1.0f);
  c = interp->getValue_changed();
  check(feq(c.r, 0) && feq(c.g, 1) && feq(c.b, 0), "color f=1 -> green");
}

// -- OrientationInterpolator (SLERP) ----------------------------------------
void test_orientation() {
  auto interp = std::make_shared<OrientationInterpolator>();
  // 0 rad -> pi/2 rad about +Z. Midpoint SLERP should be pi/4 about +Z.
  interp->setKey(MFFloat{0.0f, 1.0f});
  interp->setKeyValue(
      MFRotation{SFRotation{0, 0, 1, 0.0f}, SFRotation{0, 0, 1, 1.5707963f}});

  X3DExecutionContext ctx;
  InterpolatorSystem<OrientationInterpolator, SFRotation> sys(
      [](const SFRotation &a, const SFRotation &b, float t) { return slerpRotation(a, b, t); });
  sys.attach(interp.get(), ctx);

  post(ctx, interp.get(), 0.5f);
  SFRotation r = interp->getValue_changed();
  check(feq(std::fabs(r.z), 1.0f), "orientation f=0.5 axis is +/-Z");
  // Angle should be ~pi/4 (0.7853982). Axis sign may flip; compare magnitudes.
  float effective = (r.z >= 0) ? r.angle : -r.angle;
  check(feq(effective, 0.7853982f) || feq(r.angle, 0.7853982f),
        "orientation f=0.5 SLERP angle ~ pi/4");
  post(ctx, interp.get(), 1.0f);
  r = interp->getValue_changed();
  check(feq(r.angle, 1.5707963f) && feq(std::fabs(r.z), 1.0f),
        "orientation f=1 -> pi/2 about Z");
}

// -- CoordinateInterpolator (per-point linear) ------------------------------
void test_coordinate() {
  auto interp = std::make_shared<CoordinateInterpolator>();
  // 2 keys, 2 points each: row0=[(0,0,0),(1,1,1)] row1=[(10,0,0),(11,1,1)].
  interp->setKey(MFFloat{0.0f, 1.0f});
  interp->setKeyValue(MFVec3f{SFVec3f{0, 0, 0}, SFVec3f{1, 1, 1},
                              SFVec3f{10, 0, 0}, SFVec3f{11, 1, 1}});

  X3DExecutionContext ctx;
  MultiInterpolatorSystem<CoordinateInterpolator, SFVec3f> sys(
      [](const SFVec3f &a, const SFVec3f &b, float t) { return lerpVec3(a, b, t); });
  sys.attach(interp.get(), ctx);

  post(ctx, interp.get(), 0.5f);
  MFVec3f v = interp->getValue_changed();
  check(v.size() == 2, "coordinate emits 2 points");
  check(feq(v[0].x, 5) && feq(v[0].y, 0) && feq(v[0].z, 0),
        "coordinate point0 f=0.5 -> (5,0,0)");
  check(feq(v[1].x, 6) && feq(v[1].y, 1) && feq(v[1].z, 1),
        "coordinate point1 f=0.5 -> (6,1,1)");
  post(ctx, interp.get(), 1.0f);
  v = interp->getValue_changed();
  check(v.size() == 2 && feq(v[0].x, 10) && feq(v[1].x, 11),
        "coordinate f=1 -> row1");
}

// -- CoordinateInterpolator2D -----------------------------------------------
void test_coordinate2d() {
  auto interp = std::make_shared<CoordinateInterpolator2D>();
  interp->setKey(MFFloat{0.0f, 1.0f});
  interp->setKeyValue(
      MFVec2f{SFVec2f{0, 0}, SFVec2f{2, 2}, SFVec2f{4, 0}, SFVec2f{6, 2}});

  X3DExecutionContext ctx;
  MultiInterpolatorSystem<CoordinateInterpolator2D, SFVec2f> sys(
      [](const SFVec2f &a, const SFVec2f &b, float t) { return lerpVec2(a, b, t); });
  sys.attach(interp.get(), ctx);

  post(ctx, interp.get(), 0.5f);
  MFVec2f v = interp->getValue_changed();
  check(v.size() == 2, "coordinate2D emits 2 points");
  check(feq(v[0].x, 2) && feq(v[0].y, 0), "coordinate2D point0 f=0.5 -> (2,0)");
  check(feq(v[1].x, 4) && feq(v[1].y, 2), "coordinate2D point1 f=0.5 -> (4,2)");
}

// -- NormalInterpolator (SLERP on sphere) -----------------------------------
void test_normal() {
  auto interp = std::make_shared<NormalInterpolator>();
  // 1 point: +X -> +Y. Midpoint SLERP is the unit bisector (~0.707,0.707,0).
  interp->setKey(MFFloat{0.0f, 1.0f});
  interp->setKeyValue(MFVec3f{SFVec3f{1, 0, 0}, SFVec3f{0, 1, 0}});

  X3DExecutionContext ctx;
  MultiInterpolatorSystem<NormalInterpolator, SFVec3f> sys(
      [](const SFVec3f &a, const SFVec3f &b, float t) { return slerpNormal(a, b, t); });
  sys.attach(interp.get(), ctx);

  post(ctx, interp.get(), 0.5f);
  MFVec3f v = interp->getValue_changed();
  check(v.size() == 1, "normal emits 1 point");
  float len = std::sqrt(v[0].x * v[0].x + v[0].y * v[0].y + v[0].z * v[0].z);
  check(feq(len, 1.0f), "normal f=0.5 stays unit length");
  check(feq(v[0].x, 0.70710678f) && feq(v[0].y, 0.70710678f) && feq(v[0].z, 0),
        "normal f=0.5 -> unit bisector (0.707,0.707,0)");
  post(ctx, interp.get(), 1.0f);
  v = interp->getValue_changed();
  check(feq(v[0].x, 0) && feq(v[0].y, 1) && feq(v[0].z, 0), "normal f=1 -> +Y");
}

// -- Multi-interpolator scene: one source fraction fans out -----------------
// Proves several interpolator systems coexist in one context, each driven by
// the same posted fraction through the cascade (here posted directly to each
// set_fraction sink; in a real scene a TimeSensor would ROUTE to all of them).
void test_multi_interpolator_scene() {
  auto pos = std::make_shared<PositionInterpolator>();
  pos->setKey(MFFloat{0.0f, 1.0f});
  pos->setKeyValue(MFVec3f{SFVec3f{0, 0, 0}, SFVec3f{8, 0, 0}});

  auto sca = std::make_shared<ScalarInterpolator>();
  sca->setKey(MFFloat{0.0f, 1.0f});
  sca->setKeyValue(MFFloat{0.0f, 100.0f});

  auto col = std::make_shared<ColorInterpolator>();
  col->setKey(MFFloat{0.0f, 1.0f});
  col->setKeyValue(MFColor{SFColor{1, 0, 0}, SFColor{0, 1, 0}});

  X3DExecutionContext ctx;
  InterpolatorSystem<PositionInterpolator, SFVec3f> psys(
      [](const SFVec3f &a, const SFVec3f &b, float t) { return lerpVec3(a, b, t); });
  InterpolatorSystem<ScalarInterpolator, float> ssys(
      [](const float &a, const float &b, float t) { return lerpf(a, b, t); });
  InterpolatorSystem<ColorInterpolator, SFColor> csys(
      [](const SFColor &a, const SFColor &b, float t) { return lerpColorHsv(a, b, t); });
  psys.attach(pos.get(), ctx);
  ssys.attach(sca.get(), ctx);
  csys.attach(col.get(), ctx);

  // One fraction, three interpolators.
  float f = 0.25f;
  ctx.postEvent(pos.get(), "set_fraction", std::any(SFFloat{f}));
  ctx.postEvent(sca.get(), "set_fraction", std::any(SFFloat{f}));
  ctx.postEvent(col.get(), "set_fraction", std::any(SFFloat{f}));
  ctx.process();

  check(feq(pos->getValue_changed().x, 2.0f),
        "scene: position at f=0.25 -> x=2");
  check(feq(sca->getValue_changed(), 25.0f), "scene: scalar at f=0.25 -> 25");
  SFColor c = col->getValue_changed();
  check(feq(c.r, 1.0f) && c.g > 0.0f && feq(c.b, 0.0f),
        "scene: color at f=0.25 stays on red->green HSV arc");
}

} // namespace

TEST_CASE("interpolator_test") {
  test_scalar();
  test_position();
  test_position2d();
  test_color();
  test_orientation();
  test_coordinate();
  test_coordinate2d();
  test_normal();
  test_multi_interpolator_scene();

  if (failures) {
    std::cerr << failures << " check(s) failed\n";
    CHECK(false); return;
  }
  std::cout << "all interpolator tests passed\n";
  return;
}
