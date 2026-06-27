#include "doctest/doctest.h"
// follower_conformance_test.cpp
// Behavioral-conformance tests for the Followers component runtime (§39):
// FollowerArith ops, DamperSystem (IIR), ChaserSystem (re-basing ramp), wiring.
#include "FollowerArith.hpp"
#include "FollowerRegistration.hpp"
#include "FollowerSystem.hpp"
#include "x3d/nodes/PositionChaser.hpp"
#include "x3d/nodes/PositionDamper.hpp"
#include "x3d/nodes/ScalarChaser.hpp"
#include "x3d/nodes/ScalarDamper.hpp"
#include "X3DExecutionContext.hpp"
#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>
using namespace x3d::runtime;
using namespace x3d;
static int g_fail = 0;
#define XCHECK(c, m) do{ if(!(c)){ std::fprintf(stderr,"FAIL: %s (%s:%d)\n",m,__FILE__,__LINE__); ++g_fail; } }while(0)
static bool feq(float a, float b, float e=1e-4f){ return std::fabs(a-b) < e; }

static void test_arith() {
  // float
  XCHECK(feq(FollowerArith<float>::lerp(0.f,10.f,0.25f), 2.5f), "float lerp");
  XCHECK(feq(FollowerArith<float>::dist(1.f,4.f), 3.f), "float dist");
  // SFVec3f
  SFVec3f a{0,0,0}, b{2,4,6};
  auto m = FollowerArith<SFVec3f>::lerp(a,b,0.5f);
  XCHECK(feq(m.x,1)&&feq(m.y,2)&&feq(m.z,3), "vec3 lerp");
  XCHECK(feq(FollowerArith<SFVec3f>::dist(a,b), std::sqrt(56.f), 1e-3f), "vec3 dist");
  // SFColor
  SFColor c0{0,0,0}, c1{1,1,1};
  auto cm = FollowerArith<SFColor>::lerp(c0,c1,0.5f);
  XCHECK(feq(cm.r,0.5f)&&feq(cm.g,0.5f)&&feq(cm.b,0.5f), "color lerp");
  // SFVec2f
  SFVec2f v0{0,0}, v1{4,8}; auto vm=FollowerArith<SFVec2f>::lerp(v0,v1,0.25f);
  XCHECK(feq(vm.x,1)&&feq(vm.y,2), "vec2 lerp");
  // SFRotation: slerp end-points + stays unit, dist=angle
  SFRotation r0{0,1,0,0.f}, r1{0,1,0,1.5707963f};
  auto re = FollowerArith<SFRotation>::lerp(r0,r1,1.0f);
  XCHECK(feq(re.angle,1.5707963f,1e-3f), "rotation slerp reaches end angle");
  XCHECK(FollowerArith<SFRotation>::dist(r0,r1) > 1.4f, "rotation dist ~ angle");
  XCHECK(FollowerArith<SFRotation>::dist(r0,r0) < 1e-4f, "rotation dist zero when equal");
  // MFVec3f element-wise; output adopts b's length
  MFVec3f ma{{0,0,0},{0,0,0}}, mb{{2,2,2},{4,4,4}};
  auto mm = FollowerArith<MFVec3f>::lerp(ma,mb,0.5f);
  XCHECK(mm.size()==2 && feq(mm[0].x,1)&&feq(mm[1].x,2), "MFVec3f element-wise lerp");
  XCHECK(FollowerArith<MFVec3f>::dist(ma,mb) > 0.f, "MFVec3f dist nonzero");
  // MFVec2f
  MFVec2f na{{0,0}}, nb{{4,4}};
  auto nm = FollowerArith<MFVec2f>::lerp(na,nb,0.5f);
  XCHECK(nm.size()==1 && feq(nm[0].x,2), "MFVec2f element-wise lerp");
}

static void test_damper() {
  X3DExecutionContext ctx;
  // order-1, tau=0.3 scalar step 0 -> 1; sample at t=tau => ~0.6321.
  // tolerance defaults to -1 (no finite tolerance), so omit the setter call.
  auto sd = std::make_shared<ScalarDamper>();
  sd->setOrderUnchecked(1); sd->setTauUnchecked(0.3);
  sd->setInitialValueUnchecked(0.0f); sd->setInitialDestinationUnchecked(0.0f);
  DamperSystem<ScalarDamper, float> dsys;
  dsys.attach(sd.get(), ctx);
  sd->onSet_destination(1.0f);            // begin transition to 1
  // tick at dt=0.3 once: out ~ 0.6321
  dsys.update(0.3, ctx);
  XCHECK(feq(sd->getValue_changed(), 0.6321f, 5e-3f), "damper order1 reaches ~63.2% at t=tau");
  XCHECK(sd->getIsActive()==true, "damper active during transition");
  // continue: asymptotes upward toward 1
  for(double t=0.6;t<=3.0;t+=0.3) dsys.update(t, ctx);
  XCHECK(sd->getValue_changed() > 0.95f, "damper asymptotes toward destination");

  // order=0 OR tau=0 => passthrough (immediate).
  auto sp = std::make_shared<ScalarDamper>();
  sp->setOrderUnchecked(0); sp->setTauUnchecked(0.3);
  sp->setInitialValueUnchecked(0.0f); sp->setInitialDestinationUnchecked(0.0f);
  DamperSystem<ScalarDamper, float> psys; psys.attach(sp.get(), ctx);
  sp->onSet_destination(5.0f); psys.update(0.016, ctx);
  XCHECK(feq(sp->getValue_changed(), 5.0f), "damper order0 forwards destination immediately");

  // set_value jumps + stops.
  auto sv = std::make_shared<ScalarDamper>();
  sv->setOrderUnchecked(3); sv->setTauUnchecked(0.3);
  DamperSystem<ScalarDamper, float> vsys; vsys.attach(sv.get(), ctx);
  sv->onSet_destination(1.0f); vsys.update(0.1, ctx);
  sv->onSet_value(9.0f);
  XCHECK(feq(sv->getValue_changed(), 9.0f), "damper set_value jumps to value");
  XCHECK(sv->getIsActive()==false, "damper set_value stops transition");

  // determinism: two identical runs match.
  auto run=[&](){ auto n=std::make_shared<ScalarDamper>(); n->setOrderUnchecked(3); n->setTauUnchecked(0.3);
    DamperSystem<ScalarDamper,float> s; s.attach(n.get(),ctx); n->onSet_destination(1.0f);
    for(double t=0.1;t<=2.0;t+=0.1) { s.update(t,ctx); } return n->getValue_changed(); };
  XCHECK(feq(run(), run(), 1e-6f), "damper deterministic");

  // initial-transition path: initialValue != initialDestination => isActive TRUE on attach.
  {
    X3DExecutionContext ctx2;
    auto si = std::make_shared<ScalarDamper>();
    si->setOrderUnchecked(1); si->setTauUnchecked(0.3);
    si->setInitialValueUnchecked(0.0f); si->setInitialDestinationUnchecked(5.0f);
    DamperSystem<ScalarDamper, float> isys; isys.attach(si.get(), ctx2);
    XCHECK(si->getIsActive()==true, "damper initial-transition emits isActive TRUE on attach");
  }

  // Vec3 type works through the same template.
  auto pd = std::make_shared<PositionDamper>();
  pd->setOrderUnchecked(2); pd->setTauUnchecked(0.2);
  DamperSystem<PositionDamper, SFVec3f> p3; p3.attach(pd.get(), ctx);
  pd->onSet_destination(SFVec3f{10,0,0});
  for(double t=0.1;t<=3.0;t+=0.1) p3.update(t, ctx);
  XCHECK(pd->getValue_changed().x > 9.5f, "PositionDamper converges in x");
}

static void test_chaser() {
  using namespace x3d::runtime;
  X3DExecutionContext ctx;
  // D=1 step 0->1: reaches exactly 1.0 at t=1.0, ~0.5 at t=0.5.
  auto sc = std::make_shared<ScalarChaser>();
  sc->setDurationUnchecked(1.0);
  sc->setInitialValueUnchecked(0.0f); sc->setInitialDestinationUnchecked(0.0f);
  ChaserSystem<ScalarChaser, float> cs; cs.attach(sc.get(), ctx);
  sc->onSet_destination(1.0f);            // event at t=0 (first update establishes start time)
  cs.update(0.0, ctx);
  cs.update(0.5, ctx);
  XCHECK(feq(sc->getValue_changed(), 0.5f, 1e-2f), "chaser ~0.5 mid-transition");
  cs.update(1.0, ctx);
  XCHECK(feq(sc->getValue_changed(), 1.0f, 1e-3f), "chaser reaches destination exactly at duration");
  XCHECK(sc->getIsActive()==false, "chaser inactive after duration");

  // re-base on a new event at t=0.5 (output 0.5) -> 2.0 over D=1: reaches 2.0 at t=1.5.
  auto rc = std::make_shared<ScalarChaser>(); rc->setDurationUnchecked(1.0);
  ChaserSystem<ScalarChaser, float> rs; rs.attach(rc.get(), ctx);
  rc->onSet_destination(1.0f); rs.update(0.0, ctx); rs.update(0.5, ctx); // output ~0.5
  rc->onSet_destination(2.0f);                                            // re-base
  rs.update(1.0, ctx);
  XCHECK(rc->getValue_changed() > 1.1f && rc->getValue_changed() < 1.4f, "chaser re-bases toward new dest");
  rs.update(1.5, ctx);
  XCHECK(feq(rc->getValue_changed(), 2.0f, 1e-2f), "chaser reaches new dest duration after last event");

  // set_value jumps.
  auto vc = std::make_shared<ScalarChaser>(); vc->setDurationUnchecked(1.0);
  ChaserSystem<ScalarChaser, float> vs; vs.attach(vc.get(), ctx);
  vc->onSet_destination(1.0f); vs.update(0.0, ctx); vs.update(0.3, ctx);
  vc->onSet_value(7.0f);
  XCHECK(feq(vc->getValue_changed(), 7.0f), "chaser set_value jumps");

  // Vec3 reaches.
  auto pc = std::make_shared<PositionChaser>(); pc->setDurationUnchecked(1.0);
  ChaserSystem<PositionChaser, SFVec3f> ps; ps.attach(pc.get(), ctx);
  pc->onSet_destination(SFVec3f{4,0,0}); ps.update(0.0, ctx); ps.update(1.0, ctx);
  XCHECK(feq(pc->getValue_changed().x, 4.0f, 1e-2f), "PositionChaser reaches dest at duration");

  // determinism.
  auto run=[&](){ auto n=std::make_shared<ScalarChaser>(); n->setDurationUnchecked(1.0);
    ChaserSystem<ScalarChaser,float> s; s.attach(n.get(),ctx); n->onSet_destination(1.0f);
    for(double t=0.0;t<=1.0;t+=0.1) { s.update(t,ctx); } return n->getValue_changed(); };
  XCHECK(feq(run(), run(), 1e-6f), "chaser deterministic");

  // initial-transition path: initialValue != initialDestination => isActive TRUE on attach.
  {
    X3DExecutionContext ctx2;
    auto ic = std::make_shared<ScalarChaser>();
    ic->setDurationUnchecked(1.0);
    ic->setInitialValueUnchecked(0.0f); ic->setInitialDestinationUnchecked(5.0f);
    ChaserSystem<ScalarChaser, float> isys; isys.attach(ic.get(), ctx2);
    XCHECK(ic->getIsActive()==true, "chaser initial-transition emits isActive TRUE on attach");
  }
}

static void test_mf_followers() {
  // --- CoordinateDamper (MFVec3f): length-1 seed → length-2 destination ---
  {
    X3DExecutionContext ctx;
    auto cd = std::make_shared<CoordinateDamper>();
    // Seed with length-1 initial (default-ish), order 2, tau 0.2.
    cd->setInitialValueUnchecked(MFVec3f{{0,0,0}});
    cd->setOrderUnchecked(2);
    cd->setTauUnchecked(0.2);
    DamperSystem<CoordinateDamper, MFVec3f> dsys;
    dsys.attach(cd.get(), ctx);
    // Destination is length 2 — exercises the reshape path.
    cd->onSet_destination(MFVec3f{{10,0,0},{0,10,0}});
    // Run ~3 s (well beyond 5τ=1s) — must converge.
    for (double t = 0.05; t <= 3.0; t += 0.05) dsys.update(t, ctx);
    auto val = cd->getValue_changed();
    XCHECK(val.size() == 2, "CoordinateDamper MF output size == 2 after reshape");
    XCHECK(val.size() >= 1 && val[0].x > 9.5f, "CoordinateDamper MF element [0].x converges");
    XCHECK(val.size() >= 2 && val[1].y > 9.5f, "CoordinateDamper MF element [1].y converges");
    XCHECK(cd->getIsActive() == false, "CoordinateDamper MF isActive false after convergence (no hang)");
  }

  // --- CoordinateDamper: matching-length (2→2) converges element-wise ---
  {
    X3DExecutionContext ctx;
    auto cd = std::make_shared<CoordinateDamper>();
    cd->setInitialValueUnchecked(MFVec3f{{0,0,0},{0,0,0}});
    cd->setOrderUnchecked(2);
    cd->setTauUnchecked(0.2);
    DamperSystem<CoordinateDamper, MFVec3f> dsys;
    dsys.attach(cd.get(), ctx);
    cd->onSet_destination(MFVec3f{{5,0,0},{0,5,0}});
    for (double t = 0.05; t <= 3.0; t += 0.05) dsys.update(t, ctx);
    auto val = cd->getValue_changed();
    XCHECK(val.size() == 2, "CoordinateDamper matching-length output size == 2");
    XCHECK(val.size() >= 1 && val[0].x > 4.8f, "CoordinateDamper matching-length [0].x converges");
    XCHECK(val.size() >= 2 && val[1].y > 4.8f, "CoordinateDamper matching-length [1].y converges");
    XCHECK(cd->getIsActive() == false, "CoordinateDamper matching-length isActive false");
  }

  // --- CoordinateChaser (MFVec3f): length-1 seed → length-2 destination ---
  {
    X3DExecutionContext ctx;
    auto cc = std::make_shared<CoordinateChaser>();
    cc->setDurationUnchecked(1.0);
    ChaserSystem<CoordinateChaser, MFVec3f> csys;
    csys.attach(cc.get(), ctx);
    cc->onSet_destination(MFVec3f{{4,0,0},{0,4,0}});
    csys.update(0.0, ctx);
    csys.update(1.0, ctx);
    auto val = cc->getValue_changed();
    XCHECK(val.size() == 2, "CoordinateChaser MF output size == 2");
    XCHECK(val.size() >= 1 && feq(val[0].x, 4.0f, 1e-2f), "CoordinateChaser MF element [0].x reaches dest");
    XCHECK(val.size() >= 2 && feq(val[1].y, 4.0f, 1e-2f), "CoordinateChaser MF element [1].y reaches dest");
    XCHECK(cc->getIsActive() == false, "CoordinateChaser MF isActive false after duration");
  }

  // --- ChaserSystem: duration=0 snaps immediately ---
  {
    X3DExecutionContext ctx;
    auto sc = std::make_shared<ScalarChaser>();
    sc->setDurationUnchecked(0.0);
    ChaserSystem<ScalarChaser, float> csys;
    csys.attach(sc.get(), ctx);
    sc->onSet_destination(7.0f);
    csys.update(0.0, ctx);
    XCHECK(feq(sc->getValue_changed(), 7.0f), "chaser duration=0 snaps immediately");
    XCHECK(sc->getIsActive() == false, "chaser duration=0 isActive false after snap");
  }

  // --- ChaserSystem initial re-base: mid-transition set_destination re-bases from current output ---
  {
    X3DExecutionContext ctx;
    auto sc = std::make_shared<ScalarChaser>();
    sc->setDurationUnchecked(1.0);
    sc->setInitialValueUnchecked(0.0f);
    sc->setInitialDestinationUnchecked(10.0f); // initial active transition
    ChaserSystem<ScalarChaser, float> csys;
    csys.attach(sc.get(), ctx); // initiallyActive=true, started=true
    // Advance partway through the initial transition (~50% at t=0.5)
    csys.update(0.0, ctx);
    csys.update(0.5, ctx);
    float midVal = sc->getValue_changed(); // ~5.0
    // Now fire a new destination — should re-base from midVal, not from initialValue (0)
    sc->onSet_destination(20.0f);
    csys.update(0.5, ctx); // immediately after re-base: f~0, output ~midVal
    float afterRebase = sc->getValue_changed();
    // Must NOT jump backward to near 0; should be close to midVal
    XCHECK(afterRebase >= midVal - 1.0f, "chaser initial re-base does not backward-jump");
    csys.update(1.5, ctx); // 1s after re-base event at t=0.5 → reaches 20
    XCHECK(feq(sc->getValue_changed(), 20.0f, 1e-2f), "chaser initial re-base reaches new dest");
  }
}

static void test_wiring() {
  using namespace x3d::runtime;
  // makeFollowerSystems registers all 14 (sanity: non-empty, attaches a damper).
  auto systems = makeFollowerSystems();
  XCHECK(systems.size() == 14, "makeFollowerSystems registers all 14 follower systems");
  X3DExecutionContext ctx;
  auto pd = std::make_shared<PositionDamper>();
  pd->setOrderUnchecked(2); pd->setTauUnchecked(0.2);
  bool lit = false;
  for (auto &s : systems) s->attach(pd.get(), ctx);   // exactly one system claims it
  pd->onSet_destination(SFVec3f{5,0,0});
  for (auto &s : systems) for (double t=0.1;t<=3.0;t+=0.1) s->update(t, ctx);
  lit = pd->getValue_changed().x > 4.5f;
  XCHECK(lit, "a PositionDamper attached via makeFollowerSystems behaves (was inert)");
}

TEST_CASE("follower_conformance_test") {
  test_arith();
  test_damper();
  test_chaser();
  test_mf_followers();
  test_wiring();
  if(g_fail==0) std::fprintf(stderr,"follower_conformance_test: ALL PASS\n");
  CHECK(g_fail==0);
  return;
}
