#include "doctest/doctest.h"
// navigation_test.cpp — M2D NavigationSystem driving the bound Viewpoint from
// the input seam (pointer-drag + KeyState) and the bound NavigationInfo, per
// ISO/IEC 19775-1 §23.2.3 / §23.3.1 / §23.4.4 (modes EXAMINE / FLY / LOOKAT /
// NONE, collision-free). Builds small scenes in code, feeds pointer/key state
// via the context, ticks, and reads the Viewpoint pose / NavigationInfo outputs
// back off the generated getters.
//
// Cases:
//   (1) EXAMINE drag orbits the bound Viewpoint about centerOfRotation: the
//       eye stays at the same distance from the pivot, the eye moves, and the
//       camera keeps pointing at the pivot.
//   (2) FLY forward key translates along the view dir scaled by speed*dt; FLY
//       drag yaws/pitches the orientation. speed=0 locks position.
//   (3) LOOKAT animates toward a picked object's bbox over transitionTime and
//       emits transitionComplete; centerOfRotation is set to the bbox center.
//   (4) NONE is inert (no pose change on drag/keys).
//   (5) Switching NavigationInfo.type switches behavior (NONE -> EXAMINE).
//
// Exit code 0 on success; nonzero on any failed check.

#include "NavigationSystem.hpp"
#include "X3DDocument.hpp" // inline Scene::addRootNode
#include "X3DExecutionContext.hpp"
#include "X3DNodeFactory.hpp" // createX3DNode

#include "Box.hpp"
#include "NavigationInfo.hpp"
#include "Shape.hpp"
#include "Transform.hpp"
#include "Viewpoint.hpp"

#include <any>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace x3d;
using namespace x3d::runtime;

namespace {

int failures = 0;
void check(bool cond, const std::string &what) {
  if (!cond) { std::cerr << "FAIL: " << what << "\n"; ++failures; }
  else        { std::cout << "ok: " << what << "\n"; }
}
bool feq(float a, float b, float e = 1e-3f) { return std::fabs(a - b) < e; }

void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}
void addChild(const std::shared_ptr<X3DNode> &p, const std::shared_ptr<X3DNode> &c) {
  for (auto &f : p->fields())
    if (f.x3dName == "children" && f.set) {
      auto k = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*p));
      k.push_back(c); f.set(*p, std::any(std::move(k))); return;
    }
}

float dist(const SFVec3f &a, const SFVec3f &b) {
  float dx = a.x-b.x, dy = a.y-b.y, dz = a.z-b.z;
  return std::sqrt(dx*dx + dy*dy + dz*dz);
}

Viewpoint *vpOf(X3DExecutionContext &ctx) {
  return dynamic_cast<Viewpoint *>(ctx.boundViewpoint());
}
// CONF-VIEWNAV: navigation moves the EFFECTIVE camera (pose ∘ offset), not the
// authored fields. Tests assert the effective eye/forward, not vp->getPosition().
SFVec3f camPos(X3DExecutionContext &ctx) { return ctx.cameraWorldPosition(); }
SFVec3f camFwd(X3DExecutionContext &ctx) {
  return ctx.viewMatrix().inverse().transformDirection(SFVec3f{0, 0, -1});
}
NavigationInfo *navOf(X3DExecutionContext &ctx) {
  return dynamic_cast<NavigationInfo *>(ctx.boundNavigationInfo());
}

// Build a scene with a Viewpoint + NavigationInfo + a Box shape; register the
// NavigationSystem. Returns the context (held by value-ptr keepalives below).
struct World {
  std::shared_ptr<X3DNode> vp, nav, shape, box;
  X3DExecutionContext ctx;
};

std::shared_ptr<World> makeWorld(std::vector<std::string> types,
                                 SFVec3f vpPos = {0,0,10}) {
  auto w = std::make_shared<World>();
  w->box = createX3DNode("Box"); setF(w->box, "size", std::any(SFVec3f{2,2,2}));
  w->shape = createX3DNode("Shape");
  setF(w->shape, "geometry", std::any(std::shared_ptr<X3DNode>(w->box)));
  w->vp = createX3DNode("Viewpoint");
  setF(w->vp, "position", std::any(SFVec3f{vpPos}));
  w->nav = createX3DNode("NavigationInfo");
  dynamic_cast<NavigationInfo &>(*w->nav).setType(types);

  Scene scene;
  scene.addRootNode(w->shape);
  scene.addRootNode(w->vp);
  scene.addRootNode(w->nav);
  w->ctx.buildSceneGraph(scene);
  w->ctx.addSystem(std::make_shared<NavigationSystem>());
  return w;
}

} // namespace

TEST_CASE("navigation_test") {
  // --- (1) EXAMINE drag orbits about centerOfRotation ----------------------
  {
    auto w = makeWorld({"EXAMINE"}, {0,0,10});
    auto *vp = vpOf(w->ctx);
    check(vp != nullptr, "examine: viewpoint bound");
    SFVec3f cor = vp->getCenterOfRotation(); // default (0,0,0)
    SFVec3f p0 = camPos(w->ctx);
    SFVec3f authPos0 = vp->getPosition();
    float r0 = dist(p0, cor);

    // Begin a drag: press, then move the pointer horizontally.
    w->ctx.setPointerPresent(true);
    w->ctx.setPointer(Ray{{0,0,10},{0,0,-1}});
    w->ctx.setPointerButton(true);
    w->ctx.tick(0.0);
    // Horizontal drag (dx > 0).
    w->ctx.setPointer(Ray{{0.3f,0,10},{0,0,-1}}); // consumer reports motion
    w->ctx.tick(0.016);

    SFVec3f p1 = camPos(w->ctx);
    float r1 = dist(p1, cor);
    check(dist(p0, p1) > 1e-3f, "examine: eye moved on drag");
    check(feq(r0, r1, 1e-2f), "examine: orbit preserves distance to pivot");
    check(dist(authPos0, vp->getPosition()) < 1e-6f, "examine: authored position pristine (BIND-01)");
    // Camera must point at the pivot: effective view dir == normalize(cor - eye).
    SFVec3f fwd = camFwd(w->ctx);
    SFVec3f toC{cor.x-p1.x, cor.y-p1.y, cor.z-p1.z};
    float tl = std::sqrt(toC.x*toC.x+toC.y*toC.y+toC.z*toC.z);
    toC = {toC.x/tl, toC.y/tl, toC.z/tl};
    float dotv = fwd.x*toC.x + fwd.y*toC.y + fwd.z*toC.z;
    check(dotv > 0.99f, "examine: camera points at center of rotation");
  }

  // --- (2) FLY forward key + drag, speed-scaled ----------------------------
  {
    auto w = makeWorld({"FLY"}, {0,0,10});
    auto *vp = vpOf(w->ctx);
    auto *nav = navOf(w->ctx);
    nav->setSpeed(2.0f);
    SFVec3f p0 = camPos(w->ctx);

    // Hold the forward key (NavigationSystem maps a fixed forward code).
    w->ctx.setKey(NavigationSystem::kKeyForward, true);
    w->ctx.tick(0.0);    // establish dt baseline
    w->ctx.tick(1.0);    // 1 second elapsed -> move speed*dt = 2 units forward
    SFVec3f p1 = camPos(w->ctx);
    // Default look dir is -Z; forward 2 units => z decreases by ~2.
    check(feq(p1.z, p0.z - 2.0f, 1e-1f), "fly: forward key translates by speed*dt along -Z");
    check(feq(p1.x, 0.0f, 1e-2f) && feq(p1.y, 0.0f, 1e-2f), "fly: forward stays on axis");

    // Drag yaws the effective view direction.
    SFVec3f f0 = camFwd(w->ctx);
    w->ctx.setPointerPresent(true);
    w->ctx.setPointer(Ray{{0,0,0},{0,0,-1}});
    w->ctx.setPointerButton(true);
    w->ctx.tick(1.0);
    w->ctx.setPointer(Ray{{0.3f,0,0},{0,0,-1}});
    w->ctx.tick(1.016);
    SFVec3f f1 = camFwd(w->ctx);
    check(dist(f0, f1) > 1e-3f, "fly: drag rotates the view direction");
    check(feq(vp->getPosition().z, 10.0f), "fly: authored position pristine (BIND-01)");

    // speed == 0 locks position.
    auto w2 = makeWorld({"FLY"}, {0,0,10});
    navOf(w2->ctx)->setSpeed(0.0f);
    SFVec3f q0 = camPos(w2->ctx);
    w2->ctx.setKey(NavigationSystem::kKeyForward, true);
    w2->ctx.tick(0.0);
    w2->ctx.tick(1.0);
    SFVec3f q1 = camPos(w2->ctx);
    check(dist(q0, q1) < 1e-4f, "fly: speed=0 locks position");
  }

  // --- (3) LOOKAT animates toward target bbox + transitionComplete ---------
  {
    auto w = makeWorld({"LOOKAT"}, {0,0,10});
    auto *vp = vpOf(w->ctx);
    auto *nav = navOf(w->ctx);
    nav->setTransitionTime(1.0);
    // LINEAR by default.
    bool gotComplete = false;
    // Observe transitionComplete by routing? Simpler: read after final tick via
    // a flag the System sets. We post it on the NavigationInfo; check via a
    // direct re-pick of the bound node's last emitted value is awkward, so the
    // System also exposes a query. Instead assert the pose converged + that the
    // System reports the transition finished.

    SFVec3f p0 = camPos(w->ctx);
    // Click on the box to trigger LOOKAT.
    w->ctx.setPointerPresent(true);
    w->ctx.setPointer(Ray{{0,0,10},{0,0,-1}});
    w->ctx.setPointerButton(true);
    w->ctx.tick(0.0);   // click registered -> LOOKAT begins at t0=0
    w->ctx.setPointerButton(false);
    w->ctx.tick(0.5);   // halfway
    SFVec3f pmid = camPos(w->ctx);
    check(dist(p0, pmid) > 1e-3f, "lookat: pose moves during transition");
    w->ctx.tick(1.0);   // complete
    SFVec3f pend = camPos(w->ctx);
    // The box is at origin, half-diag = sqrt(3); fov default pi/4 => d ~ 4.24.
    // Camera ends in front of the box on +Z (it approached from +Z).
    check(pend.z > 0.0f && pend.z < 10.0f, "lookat: ends at framing distance in front of box");
    // centerOfRotation set to bbox center (origin).
    SFVec3f cor = vp->getCenterOfRotation();
    check(feq(cor.x,0,1e-1f) && feq(cor.y,0,1e-1f) && feq(cor.z,0,1e-1f),
          "lookat: centerOfRotation set to bbox center");
    gotComplete = nav->getTransitionComplete();
    check(gotComplete, "lookat: transitionComplete emitted TRUE");
  }

  // --- (4) NONE is inert ----------------------------------------------------
  {
    auto w = makeWorld({"NONE"}, {0,0,10});
    SFVec3f p0 = camPos(w->ctx);
    SFVec3f f0 = camFwd(w->ctx);
    w->ctx.setPointerPresent(true);
    w->ctx.setPointer(Ray{{0,0,10},{0,0,-1}});
    w->ctx.setPointerButton(true);
    w->ctx.tick(0.0);
    w->ctx.setPointer(Ray{{0.5f,0.5f,10},{0,0,-1}});
    w->ctx.tick(0.016);
    w->ctx.setKey(NavigationSystem::kKeyForward, true);
    w->ctx.tick(1.0);
    check(dist(p0, camPos(w->ctx)) < 1e-5f, "none: effective camera unchanged");
    check(dist(f0, camFwd(w->ctx)) < 1e-5f, "none: view direction unchanged");
  }

  // --- (5) Switching type switches behavior (ANY -> EXAMINE) ---------------
  {
    auto w = makeWorld({"ANY"}, {0,0,10});
    SFVec3f p0 = camPos(w->ctx);
    w->ctx.setPointerPresent(true);
    w->ctx.setPointer(Ray{{0,0,10},{0,0,-1}});
    w->ctx.setPointerButton(true);
    w->ctx.tick(0.0);
    w->ctx.setPointer(Ray{{0.3f,0,10},{0,0,-1}});
    w->ctx.tick(0.016);
    check(dist(p0, camPos(w->ctx)) > 1e-3f, "any: behaves as EXAMINE (effective eye orbits on drag)");
  }

  // --- (6) LOOKAT framing under non-uniform ancestor scale (NAV-LOOKAT-SCALE) -
  // Bug: worldBounds(non-Transform) returned the LOCAL AABB (worldTransform
  // returns identity for non-Transform nodes), so beginLookat framed at the
  // unscaled distance. Fix: worldBounds now composes ancestor transforms via
  // PickSystem::worldOf. Test: assert the camera ends up at the SCALED
  // framing distance, with hardcoded expected values (not derived from the
  // same worldBounds call on both sides).
  {
    auto box = createX3DNode("Box"); setF(box, "size", std::any(SFVec3f{1,1,1}));
    auto shape = createX3DNode("Shape");
    setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(box)));
    auto xform = createX3DNode("Transform");
    setF(xform, "scale", std::any(SFVec3f{2.0f, 0.5f, 1.0f}));
    addChild(xform, shape);

    auto vp = createX3DNode("Viewpoint");
    setF(vp, "position", std::any(SFVec3f{0, 0, 10}));
    setF(vp, "fieldOfView", std::any(SFFloat{0.7853982f})); // pi/4
    auto nav = createX3DNode("NavigationInfo");
    dynamic_cast<NavigationInfo &>(*nav).setType({"LOOKAT"});
    dynamic_cast<NavigationInfo &>(*nav).setTransitionTime(0.0); // snap

    Scene scene;
    scene.addRootNode(xform);
    scene.addRootNode(vp);
    scene.addRootNode(nav);
    X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
    ctx.addSystem(std::make_shared<NavigationSystem>());
    ctx.tick(0.0);

    // KNOWN scaled world AABB: half-diagonal (1, 0.25, 0.5), center (0,0,0).
    const SFVec3f half{1.0f, 0.25f, 0.5f};
    const float radius = std::sqrt(half.x*half.x + half.y*half.y + half.z*half.z);
    const float fov = 0.7853982f;
    const float expected_d = radius / std::tan(fov * 0.5f);

    // Verify worldBounds now returns the correct SCALED AABB.
    Aabb wb = ctx.worldBounds(box.get());
    SFVec3f wbHalf{(wb.max.x-wb.min.x)*0.5f, (wb.max.y-wb.min.y)*0.5f, (wb.max.z-wb.min.z)*0.5f};
    check(feq(wbHalf.x, 1.0f) && feq(wbHalf.y, 0.25f) && feq(wbHalf.z, 0.5f),
          "lookat-scale: worldBounds(box) returns scaled AABB (1, 0.25, 0.5)");

    // Click on the box to trigger LOOKAT.
    ctx.setPointerPresent(true);
    ctx.setPointer(Ray{{0,0,10},{0,0,-1}});
    ctx.setPointerButton(true);
    ctx.tick(1.0);   // LOOKAT fires (transitionTime=0 -> completes same tick)
    ctx.setPointerButton(false);
    ctx.tick(2.0);   // drain

    SFVec3f cam = ctx.cameraWorldPosition();
    float actual_d = dist(cam, SFVec3f{0,0,0});
    check(feq(actual_d, expected_d, 1e-3f),
          "lookat-scale: world camera distance == scaled radius/tan(fov/2) "
          "(expected " + std::to_string(expected_d) +
          ", got " + std::to_string(actual_d) + ")");
  }

  // --- (7) FLY: no roll accumulates under repeated yaw drags (NAV-FLY-ROLL) ----
  // Bug: q = (Ryaw * Rpitch) * q accumulated compound rotations, so the camera
  // right vector developed a y-component (roll). Fix: decompose to yaw/pitch
  // scalars, reconstruct q = Rpitch * Ryaw each step — right stays horizontal.
  {
    auto w = makeWorld({"FLY"}, {0,0,10});
    w->ctx.setPointerPresent(true);
    w->ctx.setPointer(Ray{{0,0,0},{0,0,-1}});
    w->ctx.setPointerButton(true);

    // 10 yaw-only drag steps.
    for (int i = 1; i <= 10; ++i) {
      w->ctx.setPointer(Ray{{0.05f * i, 0, 0},{0,0,-1}});
      w->ctx.tick(0.016);
    }
    SFVec3f right = w->ctx.viewMatrix().inverse().transformDirection(SFVec3f{1,0,0});
    check(std::fabs(right.y) < 1e-3f,
          "fly-roll: right vector stays horizontal after repeated yaw drags");
  }

  // --- (8) FLY: no roll after combined yaw+pitch drag (NAV-FLY-ROLL) -----------
  {
    auto w = makeWorld({"FLY"}, {0,0,10});
    w->ctx.setPointerPresent(true);
    w->ctx.setPointer(Ray{{0,0,0},{0,0,-1}});
    w->ctx.setPointerButton(true);

    // Alternating yaw and pitch drags.
    for (int i = 0; i < 5; ++i) {
      w->ctx.setPointer(Ray{{0.05f, 0, 0},{0,0,-1}});      // yaw
      w->ctx.tick(0.016);
      w->ctx.setPointer(Ray{{0.05f, 0.05f, 0},{0,0,-1}});  // + pitch
      w->ctx.tick(0.016);
    }
    SFVec3f right = w->ctx.viewMatrix().inverse().transformDirection(SFVec3f{1,0,0});
    check(std::fabs(right.y) < 1e-3f,
          "fly-roll: right stays horizontal after combined yaw+pitch");
  }

  // --- (9) FLY: pitch clamps at +/-90 deg, no flip (NAV-FLY-ROLL) -------------
  {
    auto w = makeWorld({"FLY"}, {0,0,10});
    w->ctx.setPointerPresent(true);
    w->ctx.setPointer(Ray{{0,0,0},{0,0,-1}});
    w->ctx.setPointerButton(true);
    // Extreme downward drag -> pitches up to +90 clamp. (Drag convention: dy<0
    // pitches up.) Should clamp, not flip through the pole.
    w->ctx.setPointer(Ray{{0, 0, 0},{0,0,-1}});
    w->ctx.tick(0.016);  // establish drag baseline
    w->ctx.setPointer(Ray{{0, -5.0f, 0},{0,0,-1}});
    w->ctx.tick(0.016);  // actual drag: dy=-5 -> pitch += +5pi -> clamped
    SFVec3f fwd = camFwd(w->ctx);
    check(fwd.y > 0.99f,
          "fly-roll: extreme pitch clamps near +90 (forward.y near +1)");
    SFVec3f right = w->ctx.viewMatrix().inverse().transformDirection(SFVec3f{1,0,0});
    check(std::fabs(right.y) < 1e-3f, "fly-roll: right stays horizontal at pitch clamp");
  }

  // --- (10) FLY: re-decompose on mode switch preserves orientation -----------
  // Switching FLY -> NONE -> FLY should re-extract yaw/pitch from the current
  // effective orientation, not snap back to zero or carry stale scalars.
  {
    auto w = makeWorld({"FLY"}, {0,0,10});
    w->ctx.setPointerPresent(true);
    w->ctx.setPointer(Ray{{0,0,0},{0,0,-1}});
    w->ctx.setPointerButton(true);
    w->ctx.setPointer(Ray{{0.3f, 0, 0},{0,0,-1}});
    w->ctx.tick(0.016);
    SFVec3f f1 = camFwd(w->ctx);

    // Switch to NONE, then back to FLY.
    navOf(w->ctx)->setType({"NONE"});
    w->ctx.tick(0.016);
    navOf(w->ctx)->setType({"FLY"});
    w->ctx.tick(0.016);

    // Orientation should be preserved (re-decomposed, not reset).
    SFVec3f f2 = camFwd(w->ctx);
    check(dist(f1, f2) < 1e-3f,
          "fly-roll: re-decompose on mode switch preserves orientation");

    // Further yaw should add to the current yaw, not reset to zero.
    w->ctx.setPointer(Ray{{0.6f, 0, 0},{0,0,-1}});
    w->ctx.tick(0.016);
    SFVec3f f3 = camFwd(w->ctx);
    check(dist(f2, f3) > 1e-3f,
          "fly-roll: drag after re-decompose continues from current yaw");
  }

  if (failures) { std::cerr << failures << " check(s) failed\n"; CHECK(false); return; }
  std::cout << "all navigation checks passed\n";
  return;
}

// GEO-1 siblings: NavigationSystem::poseOf/cor read a bound viewpoint's position
// and centerOfRotation. On a GeoViewpoint those are SFVec3d — an EXAMINE drag must
// run without the getField type-mismatch assert and orbit the authored geo eye.
TEST_CASE("navigation_geoviewpoint_examine") {
  auto box = createX3DNode("Box"); setF(box, "size", std::any(SFVec3f{2,2,2}));
  auto shape = createX3DNode("Shape");
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(box)));
  auto gvp = createX3DNode("GeoViewpoint");
  setF(gvp, "position", std::any(SFVec3d{0,0,10}));          // double-precision
  setF(gvp, "centerOfRotation", std::any(SFVec3d{0,0,0}));
  auto nav = createX3DNode("NavigationInfo");
  dynamic_cast<NavigationInfo &>(*nav).setType({"EXAMINE"});

  Scene scene;
  scene.addRootNode(shape);
  scene.addRootNode(gvp);
  scene.addRootNode(nav);
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  ctx.addSystem(std::make_shared<NavigationSystem>());

  REQUIRE(ctx.boundViewpoint() != nullptr);
  SFVec3f p0 = camPos(ctx);
  CHECK(feq(p0.z, 10.0f, 1e-3f)); // authored geo eye, not the origin

  // EXAMINE drag exercises poseOf()+cor() on the GeoViewpoint.
  ctx.setPointerPresent(true);
  ctx.setPointer(Ray{{0,0,10},{0,0,-1}});
  ctx.setPointerButton(true);
  ctx.tick(0.0);
  ctx.setPointer(Ray{{0.3f,0,10},{0,0,-1}});
  ctx.tick(0.016);
  CHECK(dist(p0, camPos(ctx)) > 1e-3f); // orbited about the pivot
}
