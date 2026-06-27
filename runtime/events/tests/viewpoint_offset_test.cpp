#include "doctest/doctest.h"
// viewpoint_offset_test.cpp — CONF-VIEWNAV Phase 1: the user-offset model.
// Navigation must NOT mutate the authored Viewpoint pose (§23.2.3, BIND-01); it
// accumulates a process-local offset, and a CAVE head-pose seam composes on top.
// The effective view (viewMatrix/cameraWorldPosition) = worldOf · pose · offset · head.
//
// Cases: (1) setViewpointOffset moves the effective camera, authored fields
// untouched; (2) setHeadPose composes; (3) EXAMINE navigation leaves authored
// position/orientation byte-unchanged but moves the effective camera (BIND-01);
// (4) an OrthoViewpoint navigates too (BIND-03); (5) rotationFromMatrix round-trips.

#include "NavigationSystem.hpp"
#include "ViewpointOffset.hpp"
#include "X3DDocument.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DNodeFactory.hpp"

#include "Mat4.hpp"
#include "NavigationInfo.hpp"
#include "OrthoViewpoint.hpp"
#include "Viewpoint.hpp"

#include <any>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>

using namespace x3d;
using namespace x3d::runtime;

namespace {
int failures = 0;
void check(bool c, const std::string &w) {
  if (!c) { std::cerr << "FAIL: " << w << "\n"; ++failures; }
  else std::cout << "ok: " << w << "\n";
}
bool feq(float a, float b, float e = 1e-3f) { return std::fabs(a - b) < e; }
// (setF removed: dead — the test uses the public reflection API directly.)

// (1) offset moves the effective camera; authored fields stay pristine.
void test_offset_moves_camera() {
  auto vp = std::make_shared<Viewpoint>();
  vp->setPosition(SFVec3f{0, 0, 10});
  Scene scene; scene.addRootNode(vp);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  SFVec3f e0 = ctx.cameraWorldPosition();
  check(feq(e0.x, 0) && feq(e0.y, 0) && feq(e0.z, 10), "no offset -> camera at authored (0,0,10)");

  ctx.setViewpointOffset(vp.get(), ViewpointOffset{Mat4::translation(SFVec3f{0, 0, -5})});
  SFVec3f e1 = ctx.cameraWorldPosition();
  check(feq(e1.z, 5), "offset T(0,0,-5) -> camera at (0,0,5)");
  SFVec3f auth = vp->getPosition();
  check(feq(auth.x, 0) && feq(auth.y, 0) && feq(auth.z, 10),
        "BIND-01: authored position unchanged by offset");
}

// (2) head-pose composes after the offset.
void test_head_pose_composes() {
  auto vp = std::make_shared<Viewpoint>(); vp->setPosition(SFVec3f{0, 0, 10});
  Scene scene; scene.addRootNode(vp);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  ctx.setHeadPose(SFVec3f{0, 1, 0}, SFRotation{0, 0, 1, 0});
  SFVec3f e = ctx.cameraWorldPosition();
  check(feq(e.x, 0) && feq(e.y, 1) && feq(e.z, 10), "head T(0,1,0) -> camera (0,1,10)");
}

// (3) BIND-01: EXAMINE navigation leaves authored pose byte-unchanged.
void test_navigation_preserves_authored() {
  auto box = createX3DNode("Box");
  auto vp = std::make_shared<Viewpoint>(); vp->setPosition(SFVec3f{0, 0, 10});
  auto nav = std::make_shared<NavigationInfo>();
  nav->setType(std::vector<std::string>{"EXAMINE"});
  Scene scene; scene.addRootNode(box); scene.addRootNode(vp); scene.addRootNode(nav);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  ctx.addSystem(std::make_shared<NavigationSystem>());

  const SFVec3f authPos = vp->getPosition();
  const SFRotation authOri = vp->getOrientation();
  ctx.setPointerPresent(true);
  ctx.setPointer(Ray{{0, 0, 10}, {0, 0, -1}});
  ctx.setPointerButton(true);
  ctx.tick(0.0);
  ctx.setPointer(Ray{{0.3f, 0, 10}, {0, 0, -1}});
  ctx.tick(0.016);

  SFVec3f p = vp->getPosition();
  check(feq(p.x, authPos.x) && feq(p.y, authPos.y) && feq(p.z, authPos.z),
        "BIND-01: EXAMINE drag does NOT change authored position");
  SFRotation o = vp->getOrientation();
  check(feq(o.x, authOri.x) && feq(o.y, authOri.y) && feq(o.z, authOri.z) && feq(o.angle, authOri.angle),
        "BIND-01: EXAMINE drag does NOT change authored orientation");
  SFVec3f cam = ctx.cameraWorldPosition();
  check(std::fabs(cam.x - authPos.x) > 1e-3f || std::fabs(cam.z - authPos.z) > 1e-3f,
        "EXAMINE moved the EFFECTIVE camera (via the offset)");
}

// (4) BIND-03: an OrthoViewpoint navigates (not just Viewpoint).
void test_ortho_navigates() {
  auto vp = std::make_shared<OrthoViewpoint>(); vp->setPosition(SFVec3f{0, 0, 10});
  auto nav = std::make_shared<NavigationInfo>();
  nav->setType(std::vector<std::string>{"EXAMINE"});
  Scene scene; scene.addRootNode(vp); scene.addRootNode(nav);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  ctx.addSystem(std::make_shared<NavigationSystem>());
  SFVec3f e0 = ctx.cameraWorldPosition();
  ctx.setPointerPresent(true);
  ctx.setPointer(Ray{{0, 0, 10}, {0, 0, -1}});
  ctx.setPointerButton(true);
  ctx.tick(0.0);
  ctx.setPointer(Ray{{0.3f, 0, 10}, {0, 0, -1}});
  ctx.tick(0.016);
  SFVec3f e1 = ctx.cameraWorldPosition();
  check(std::fabs(e1.x - e0.x) > 1e-3f || std::fabs(e1.z - e0.z) > 1e-3f,
        "BIND-03: OrthoViewpoint navigates (effective camera moved)");
}

// (5) rotationFromMatrix round-trips a rotation.
void test_rotation_from_matrix() {
  SFRotation r{0, 1, 0, 0.7f};
  Mat4 m = Mat4::rotation(r);
  SFRotation r2 = rotationFromMatrix(m);
  Mat4 m2 = Mat4::rotation(r2);
  SFVec3f d = m.transformDirection(SFVec3f{1, 0, 0});
  SFVec3f d2 = m2.transformDirection(SFVec3f{1, 0, 0});
  check(feq(d.x, d2.x) && feq(d.y, d2.y) && feq(d.z, d2.z),
        "rotationFromMatrix round-trips (same rotated +X)");
}

} // namespace

TEST_CASE("viewpoint_offset_test") {
  test_offset_moves_camera();
  test_head_pose_composes();
  test_navigation_preserves_authored();
  test_ortho_navigates();
  test_rotation_from_matrix();
  if (failures) { std::cerr << failures << " check(s) failed\n"; CHECK(false); return; }
  std::cout << "all viewpoint-offset tests passed\n";
  return;
}
