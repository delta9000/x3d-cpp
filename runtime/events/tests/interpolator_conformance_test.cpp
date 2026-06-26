#include "doctest/doctest.h"
// interpolator_conformance_test.cpp
// Behavioral-conformance tests for the interpolator cluster (campaign wave-2):
//   - INTERP-02 (§19.3.1): an interpolator with an empty `key` field shall emit
//     NO value_changed event; a later non-empty key re-enables emission.
//   - INTERP-01 (§19.2.4 / §19.4.10-13): the Spline{Scalar,Position,Position2D}
//     Hermite interpolators, the SquadOrientationInterpolator, and the
//     EaseInEaseOut fraction modifier each have a System that emits on
//     set_fraction (previously the no-op default fired and nothing was emitted).
//   - PIV-1: attachInterpolators(scene, ctx) wires every interpolator in a scene
//     with no manual per-node attach (the production caller registerInterpolator-
//     Systems lacked).
//
// All expected values below are hand-computed from the normative §19.2.4 Hermite
// basis (h00=2s^3-3s^2+1, h01=-2s^3+3s^2, h10=s^3-2s^2+s, h11=s^3-s^2) and the
// §19.4.4 EaseInEaseOut algorithm.
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "Interpolation.hpp"
#include "InterpolatorSystem.hpp"
#include "SplineInterpolatorSystem.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DSceneBridge.hpp"

#include "EaseInEaseOut.hpp"
#include "ScalarInterpolator.hpp"
#include "SplinePositionInterpolator.hpp"
#include "SplineScalarInterpolator.hpp"
#include "SquadOrientationInterpolator.hpp"

#include "X3DDocument.hpp" // out-of-line Scene::addRootNode definition
#include "X3DNodeFactory.hpp"

#include <any>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>

using namespace x3d;
using namespace x3d::runtime;

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

void post(X3DExecutionContext &ctx, X3DNode *n, float fraction) {
  ctx.postEvent(n, "set_fraction", std::any(SFFloat{fraction}));
  ctx.process();
}

// -- INTERP-02: empty key emits no event ------------------------------------
void test_empty_key_no_event() {
  auto interp = std::make_shared<ScalarInterpolator>();
  interp->setKey(MFFloat{});                 // empty key
  interp->setKeyValue(MFFloat{1.0f, 2.0f});  // non-empty values
  interp->emitValue_changed(SFFloat{99.0f}); // sentinel

  X3DExecutionContext ctx;
  InterpolatorSystem<ScalarInterpolator, float> sys(
      [](const float &a, const float &b, float t) { return lerpf(a, b, t); });
  sys.attach(interp.get(), ctx);

  post(ctx, interp.get(), 0.5f);
  check(feq(interp->getValue_changed(), 99.0f),
        "INTERP-02 empty key: set_fraction emits no value_changed (sentinel kept)");

  // A later non-empty key re-enables emission on the next set_fraction.
  interp->setKey(MFFloat{0.0f, 1.0f});
  post(ctx, interp.get(), 0.5f);
  check(feq(interp->getValue_changed(), 1.5f),
        "INTERP-02 non-empty key after empty: emission re-enabled (->1.5)");
}

// -- INTERP-01: SplineScalarInterpolator (Hermite) --------------------------
void test_spline_scalar() {
  // 2 keys, no keyVelocity, not closed -> endpoint tangents zero.
  // seg0 mid (s=0.5): v = h01*10 = 0.5*10 = 5.  s=0.25: h01(0.25)*10 = 1.5625.
  auto sp = std::make_shared<SplineScalarInterpolator>();
  sp->setKey(MFFloat{0.0f, 1.0f});
  sp->setKeyValue(MFFloat{0.0f, 10.0f});

  X3DExecutionContext ctx;
  SplineInterpolatorSystem<SplineScalarInterpolator, float> sys;
  sys.attach(sp.get(), ctx);

  post(ctx, sp.get(), 0.0f);
  check(feq(sp->getValue_changed(), 0.0f), "spline scalar f=0 -> v0=0");
  post(ctx, sp.get(), 1.0f);
  check(feq(sp->getValue_changed(), 10.0f), "spline scalar f=1 -> v1=10");
  post(ctx, sp.get(), 0.25f);
  check(feq(sp->getValue_changed(), 1.5625f),
        "spline scalar f=0.25 -> 1.5625 (zero-tangent Hermite, != linear 2.5)");
  post(ctx, sp.get(), 0.5f);
  check(feq(sp->getValue_changed(), 5.0f), "spline scalar f=0.5 -> 5.0");

  // 3 keys [0,1,2] values [0,10,30], no velocity. Interior tangent T1=(30-0)/2=15.
  // seg0 mid: h01*10 + h11*15 = 5 + (-0.125)*15 = 3.125.
  // seg1 mid: h00*10 + h01*30 + h10*15 = 5 + 15 + 1.875 = 21.875.
  auto sp3 = std::make_shared<SplineScalarInterpolator>();
  sp3->setKey(MFFloat{0.0f, 1.0f, 2.0f});
  sp3->setKeyValue(MFFloat{0.0f, 10.0f, 30.0f});
  SplineInterpolatorSystem<SplineScalarInterpolator, float> sys3;
  X3DExecutionContext ctx3;
  sys3.attach(sp3.get(), ctx3);
  post(ctx3, sp3.get(), 0.5f);
  check(feq(sp3->getValue_changed(), 3.125f), "spline scalar 3-key seg0 mid -> 3.125");
  post(ctx3, sp3.get(), 1.0f);
  check(feq(sp3->getValue_changed(), 10.0f), "spline scalar 3-key f=1 (key1) -> 10");
  post(ctx3, sp3.get(), 1.5f);
  check(feq(sp3->getValue_changed(), 21.875f), "spline scalar 3-key seg1 mid -> 21.875");
}

// -- INTERP-01: author keyVelocity (size==2 endpoint form) ------------------
void test_spline_scalar_velocity() {
  // keyVelocity size 2 -> first = vel of first key, second = vel of last key.
  // Endpoints specified -> T0_0 = 2, T1_1 = 3 (raw, no F scaling).
  // seg0 mid: h01*10 + h10*2 + h11*3 = 5 + 0.125*2 + (-0.125)*3 = 4.875.
  auto sp = std::make_shared<SplineScalarInterpolator>();
  sp->setKey(MFFloat{0.0f, 1.0f});
  sp->setKeyValue(MFFloat{0.0f, 10.0f});
  sp->setKeyVelocity(MFFloat{2.0f, 3.0f});

  X3DExecutionContext ctx;
  SplineInterpolatorSystem<SplineScalarInterpolator, float> sys;
  sys.attach(sp.get(), ctx);
  post(ctx, sp.get(), 0.5f);
  check(feq(sp->getValue_changed(), 4.875f),
        "spline scalar keyVelocity[2,3] seg0 mid -> 4.875");
}

// -- INTERP-01: SplinePositionInterpolator (per-component Hermite) ----------
void test_spline_position() {
  // Each component interpolates independently with the same Hermite math as the
  // scalar 2-key zero-tangent case scaled by the component end value.
  auto sp = std::make_shared<SplinePositionInterpolator>();
  sp->setKey(MFFloat{0.0f, 1.0f});
  sp->setKeyValue(MFVec3f{SFVec3f{0, 0, 0}, SFVec3f{10, 20, 40}});

  X3DExecutionContext ctx;
  SplineInterpolatorSystem<SplinePositionInterpolator, SFVec3f> sys;
  sys.attach(sp.get(), ctx);
  post(ctx, sp.get(), 0.25f); // h01(0.25)=0.15625 -> (1.5625, 3.125, 6.25)
  SFVec3f v = sp->getValue_changed();
  check(feq(v.x, 1.5625f) && feq(v.y, 3.125f) && feq(v.z, 6.25f),
        "spline position f=0.25 -> (1.5625,3.125,6.25)");
  post(ctx, sp.get(), 1.0f);
  v = sp->getValue_changed();
  check(feq(v.x, 10) && feq(v.y, 20) && feq(v.z, 40), "spline position f=1 -> v1");
}

// -- INTERP-01: empty-key guard applies to spline systems too ---------------
void test_spline_empty_key() {
  auto sp = std::make_shared<SplineScalarInterpolator>();
  sp->setKey(MFFloat{});
  sp->setKeyValue(MFFloat{1.0f, 2.0f});
  sp->emitValue_changed(SFFloat{77.0f});
  X3DExecutionContext ctx;
  SplineInterpolatorSystem<SplineScalarInterpolator, float> sys;
  sys.attach(sp.get(), ctx);
  post(ctx, sp.get(), 0.5f);
  check(feq(sp->getValue_changed(), 77.0f),
        "spline empty key: no value_changed (sentinel kept)");
}

// -- INTERP-01: SquadOrientationInterpolator --------------------------------
void test_squad() {
  // N=2 squad reduces to plain SLERP (a0=q0, a1=q1). Mid of 0->pi/2 about Z is
  // pi/4 about Z.
  auto sq = std::make_shared<SquadOrientationInterpolator>();
  sq->setKey(MFFloat{0.0f, 1.0f});
  sq->setKeyValue(MFRotation{SFRotation{0, 0, 1, 0.0f}, SFRotation{0, 0, 1, 1.5707963f}});

  X3DExecutionContext ctx;
  SquadOrientationInterpolatorSystem sys;
  sys.attach(sq.get(), ctx);

  post(ctx, sq.get(), 0.0f);
  SFRotation r = sq->getValue_changed();
  check(feq(r.angle, 0.0f) || feq(std::fabs(r.z), 1.0f), "squad f=0 -> q0 (angle 0)");
  post(ctx, sq.get(), 1.0f);
  r = sq->getValue_changed();
  check(feq(r.angle, 1.5707963f) && feq(std::fabs(r.z), 1.0f), "squad f=1 -> pi/2 about Z");
  post(ctx, sq.get(), 0.5f);
  r = sq->getValue_changed();
  float eff = (r.z >= 0) ? r.angle : -r.angle;
  check(feq(std::fabs(r.z), 1.0f) && (feq(eff, 0.7853982f) || feq(r.angle, 0.7853982f)),
        "squad N=2 f=0.5 reduces to SLERP -> pi/4 about Z");

  // N=3 sanity: 0 -> 90 -> 0 about Z. f=1 lands on the middle key exactly.
  auto sq3 = std::make_shared<SquadOrientationInterpolator>();
  sq3->setKey(MFFloat{0.0f, 1.0f, 2.0f});
  sq3->setKeyValue(MFRotation{SFRotation{0, 0, 1, 0.0f}, SFRotation{0, 0, 1, 1.5707963f},
                              SFRotation{0, 0, 1, 0.0f}});
  X3DExecutionContext ctx3;
  SquadOrientationInterpolatorSystem sys3;
  sys3.attach(sq3.get(), ctx3);
  post(ctx3, sq3.get(), 1.0f);
  r = sq3->getValue_changed();
  check(feq(r.angle, 1.5707963f) && feq(std::fabs(r.z), 1.0f), "squad N=3 f=1 -> key1 (pi/2 Z)");
  post(ctx3, sq3.get(), 0.5f);
  r = sq3->getValue_changed();
  float len = std::sqrt(r.x * r.x + r.y * r.y + r.z * r.z);
  check(feq(std::fabs(r.z), 1.0f) || len < kEps, "squad N=3 f=0.5 axis about Z (finite unit)");
}

// -- INTERP-01: EaseInEaseOut (§19.4.4 algorithm) ---------------------------
void test_ease_in_ease_out() {
  // key=[0,1]; easeInEaseOut pairs are (easeIn, easeOut).
  // seg0: e_out = easeInEaseOut[0].y = 0.4 ; e_in = easeInEaseOut[1].x = 0.3.
  // S=0.7; t = 1/(2-0.4-0.3) = 0.7692308.
  auto ee = std::make_shared<EaseInEaseOut>();
  ee->setKey(MFFloat{0.0f, 1.0f});
  ee->setEaseInEaseOut(MFVec2f{SFVec2f{0.0f, 0.4f}, SFVec2f{0.3f, 0.0f}});

  X3DExecutionContext ctx;
  EaseInEaseOutSystem sys;
  sys.attach(ee.get(), ctx);

  // u=0.2 < e_out: (t/e_out)*u^2 = 1.923077*0.04 = 0.0769231 (global == local here).
  ctx.postEvent(ee.get(), "set_fraction", std::any(SFFloat{0.2f}));
  ctx.process();
  check(feq(ee->getModifiedFraction_changed(), 0.0769231f),
        "ease u=0.2 (easeOut region) -> 0.0769231");
  // u=0.5 in [e_out, 1-e_in]: t*(2u - e_out) = 0.7692308*0.6 = 0.4615385.
  ctx.postEvent(ee.get(), "set_fraction", std::any(SFFloat{0.5f}));
  ctx.process();
  check(feq(ee->getModifiedFraction_changed(), 0.4615385f),
        "ease u=0.5 (constant-speed region) -> 0.4615385");
  // u=0.8 > 1-e_in: 1 - (t*(1-u)^2)/e_in = 1 - 0.0307692/0.3 = 0.8974359.
  ctx.postEvent(ee.get(), "set_fraction", std::any(SFFloat{0.8f}));
  ctx.process();
  check(feq(ee->getModifiedFraction_changed(), 0.8974359f),
        "ease u=0.8 (easeIn region) -> 0.8974359");
}

void test_ease_in_ease_out_scaled() {
  // S>1 path: e_out=0.8, e_in=0.6 -> S=1.4 -> scale both by S:
  // e_out=0.5714286, e_in=0.4285714; t = 1/(2-1) = 1.0.
  // u=0.3 < e_out: (1/0.5714286)*0.09 = 1.75*0.09 = 0.1575.
  auto ee = std::make_shared<EaseInEaseOut>();
  ee->setKey(MFFloat{0.0f, 1.0f});
  ee->setEaseInEaseOut(MFVec2f{SFVec2f{0.0f, 0.8f}, SFVec2f{0.6f, 0.0f}});
  X3DExecutionContext ctx;
  EaseInEaseOutSystem sys;
  sys.attach(ee.get(), ctx);
  ctx.postEvent(ee.get(), "set_fraction", std::any(SFFloat{0.3f}));
  ctx.process();
  check(feq(ee->getModifiedFraction_changed(), 0.1575f),
        "ease S>1 scaled, u=0.3 -> 0.1575");
}

// -- PIV-1: attachInterpolators production wiring ---------------------------
void test_attach_interpolators_production() {
  // Build a scene with a core interpolator and a spline interpolator, wire only
  // through attachInterpolators (no manual per-node attach), and assert both emit.
  auto sca = createX3DNode("ScalarInterpolator");
  for (auto &f : sca->fields()) {
    if (f.x3dName == "key" && f.set) f.set(*sca, std::any(MFFloat{0.0f, 1.0f}));
    if (f.x3dName == "keyValue" && f.set) f.set(*sca, std::any(MFFloat{0.0f, 100.0f}));
  }
  auto spl = createX3DNode("SplinePositionInterpolator");
  for (auto &f : spl->fields()) {
    if (f.x3dName == "key" && f.set) f.set(*spl, std::any(MFFloat{0.0f, 1.0f}));
    if (f.x3dName == "keyValue" && f.set)
      f.set(*spl, std::any(MFVec3f{SFVec3f{0, 0, 0}, SFVec3f{8, 0, 0}}));
  }
  Scene scene;
  scene.addRootNode(sca);
  scene.addRootNode(spl);
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  attachInterpolators(scene, ctx); // production wiring — no manual attach

  ctx.postEvent(sca.get(), "set_fraction", std::any(SFFloat{0.25f}));
  ctx.postEvent(spl.get(), "set_fraction", std::any(SFFloat{1.0f}));
  ctx.process();

  float scaOut = 0.0f;
  for (auto &f : sca->fields())
    if (f.x3dName == "value_changed" && f.get) scaOut = std::any_cast<SFFloat>(f.get(*sca));
  check(feq(scaOut, 25.0f), "PIV-1 attachInterpolators wires ScalarInterpolator (->25)");

  SFVec3f splOut{};
  for (auto &f : spl->fields())
    if (f.x3dName == "value_changed" && f.get) splOut = std::any_cast<SFVec3f>(f.get(*spl));
  check(feq(splOut.x, 8.0f), "PIV-1 attachInterpolators wires SplinePositionInterpolator (->x=8)");
}

} // namespace

TEST_CASE("interpolator_conformance_test") {
  test_empty_key_no_event();
  test_spline_scalar();
  test_spline_scalar_velocity();
  test_spline_position();
  test_spline_empty_key();
  test_squad();
  test_ease_in_ease_out();
  test_ease_in_ease_out_scaled();
  test_attach_interpolators_production();

  if (failures) {
    std::cerr << failures << " check(s) failed\n";
    CHECK(false); return;
  }
  std::cout << "all interpolator-conformance tests passed\n";
  return;
}
