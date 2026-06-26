#include "doctest/doctest.h"
// viewpoint_bind_test.cpp — CONF-VIEWNAV Phase 2: bind-time offset rules
// (ISO/IEC 19775-1 §23.3.1). A ViewpointBindSystem (post-cascade) applies, on each
// bind, the jump / retainUserOffsets / stored-offset rules to the user offset —
// never the authored fields. (BIND-04, BIND-07, BIND-08.)
//
// Cases: (1) jump=FALSE keeps the effective view continuous across a bind;
// (2) jump=TRUE + retainUserOffsets=FALSE snaps to the authored pose (offset reset);
// (3) retainUserOffsets=TRUE keeps the viewpoint's stored offset on rebind.

#include "ViewpointBindSystem.hpp"
#include "ViewpointOffset.hpp"
#include "X3DDocument.hpp"
#include "X3DExecutionContext.hpp"

#include "Mat4.hpp"
#include "NavigationInfo.hpp"
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
void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields()) if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}

// (1) jump=FALSE bind keeps the effective view continuous (no jump).
void test_jump_false_continuous() {
  auto A = std::make_shared<Viewpoint>(); A->setPosition(SFVec3f{0, 0, 10});
  auto B = std::make_shared<Viewpoint>(); B->setPosition(SFVec3f{0, 0, 100});
  setF(B, "jump", std::any(SFBool{false}));
  Scene scene; scene.addRootNode(A); scene.addRootNode(B);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene); // binds A (first)
  attachViewpointBind(ctx);
  ctx.tick(0.0);
  // Simulate prior navigation on A: eye nudged to (0,0,5).
  ctx.setViewpointOffset(A.get(), ViewpointOffset{Mat4::translation(SFVec3f{0, 0, -5})});
  ctx.tick(1.0); // capture A's effective cam (0,0,5) as lastCam
  SFVec3f before = ctx.cameraWorldPosition();
  check(feq(before.z, 5), "pre-bind effective eye at (0,0,5)");

  ctx.postEvent(B.get(), "set_bind", std::any(SFBool{true}));
  ctx.tick(2.0);
  SFVec3f after = ctx.cameraWorldPosition();
  check(feq(after.z, 5), "jump=FALSE: effective view continuous across bind (still 0,0,5)");
  check(feq(B->getPosition().z, 100), "BIND-01: B authored pose pristine");
}

// (2) jump=TRUE + retainUserOffsets=FALSE snaps to authored pose (offset reset).
void test_jump_true_snaps() {
  auto A = std::make_shared<Viewpoint>(); A->setPosition(SFVec3f{0, 0, 10});
  auto B = std::make_shared<Viewpoint>(); B->setPosition(SFVec3f{0, 0, 100});
  Scene scene; scene.addRootNode(A); scene.addRootNode(B);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  attachViewpointBind(ctx);
  ctx.tick(0.0);
  ctx.setViewpointOffset(A.get(), ViewpointOffset{Mat4::translation(SFVec3f{0, 0, -5})});
  ctx.tick(1.0);

  ctx.postEvent(B.get(), "set_bind", std::any(SFBool{true}));
  ctx.tick(2.0);
  check(feq(ctx.cameraWorldPosition().z, 100), "jump=TRUE: snap to B authored (0,0,100)");
  ctx.postEvent(A.get(), "set_bind", std::any(SFBool{true}));
  ctx.tick(3.0);
  check(feq(ctx.cameraWorldPosition().z, 10),
        "jump=TRUE + retain=FALSE: rebind A resets offset -> authored (0,0,10)");
}

// (3) retainUserOffsets=TRUE keeps the stored offset on rebind.
void test_retain_user_offsets() {
  auto A = std::make_shared<Viewpoint>(); A->setPosition(SFVec3f{0, 0, 10});
  setF(A, "retainUserOffsets", std::any(SFBool{true}));
  auto B = std::make_shared<Viewpoint>(); B->setPosition(SFVec3f{0, 0, 100});
  Scene scene; scene.addRootNode(A); scene.addRootNode(B);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  attachViewpointBind(ctx);
  ctx.tick(0.0);
  ctx.setViewpointOffset(A.get(), ViewpointOffset{Mat4::translation(SFVec3f{0, 0, -5})});
  ctx.tick(1.0);
  ctx.postEvent(B.get(), "set_bind", std::any(SFBool{true}));
  ctx.tick(2.0);
  ctx.postEvent(A.get(), "set_bind", std::any(SFBool{true}));
  ctx.tick(3.0);
  check(feq(ctx.cameraWorldPosition().z, 5),
        "retainUserOffsets=TRUE: rebind A keeps stored offset (0,0,5)");
}

// (4) BIND-02: binding a viewpoint dispatches set_bind to its navigationInfo.
void test_navigationinfo_dispatch() {
  auto ni0 = std::make_shared<NavigationInfo>(); // first -> default bound
  auto ni1 = std::make_shared<NavigationInfo>(); // the viewpoint's navigationInfo
  auto vp = std::make_shared<Viewpoint>(); vp->setPosition(SFVec3f{0, 0, 10});
  vp->setNavigationInfo(std::static_pointer_cast<X3DNode>(ni1));
  Scene scene; scene.addRootNode(ni0); scene.addRootNode(vp);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene); // binds ni0 + vp (defaults)
  attachViewpointBind(ctx);
  ctx.tick(0.0); // onBind(vp) dispatches set_bind to ni1
  ctx.tick(1.0); // the dispatched set_bind drains -> ni1 bound
  check(ctx.boundNavigationInfo() == ni1.get(),
        "BIND-02: binding the viewpoint binds its navigationInfo (ni1)");
}

// (5) BIND-06: removing a bound viewpoint behaves as set_bind FALSE (pop).
void test_delete_detach() {
  auto A = std::make_shared<Viewpoint>(); A->setPosition(SFVec3f{0, 0, 10});
  auto B = std::make_shared<Viewpoint>(); B->setPosition(SFVec3f{0, 0, 100});
  Scene scene; scene.addRootNode(A); scene.addRootNode(B);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene); // A default
  attachViewpointBind(ctx);
  ctx.postEvent(B.get(), "set_bind", std::any(SFBool{true}));
  ctx.tick(0.0);
  check(ctx.boundViewpoint() == B.get(), "B bound after set_bind");
  ctx.removeBoundNode(B.get()); // B deleted -> behaves as set_bind FALSE
  check(ctx.boundViewpoint() == A.get(),
        "BIND-06: removing bound B pops the stack -> A bound again");
}

// (6) BIND-05: a non-teleport bind animates the effective camera over
// transitionTime and emits transitionComplete (not just for LOOKAT).
void test_bind_transition() {
  auto nav = std::make_shared<NavigationInfo>(); nav->setTransitionTime(2.0); // LINEAR default
  auto A = std::make_shared<Viewpoint>(); A->setPosition(SFVec3f{0, 0, 10});
  auto B = std::make_shared<Viewpoint>(); B->setPosition(SFVec3f{0, 0, 110});
  Scene scene; scene.addRootNode(nav); scene.addRootNode(A); scene.addRootNode(B);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  attachViewpointBind(ctx);
  ctx.tick(0.0); // A bound, cam (0,0,10)

  ctx.postEvent(B.get(), "set_bind", std::any(SFBool{true}));
  ctx.tick(0.0); // bind B at t0=0; transition starts at the old cam
  check(feq(ctx.cameraWorldPosition().z, 10, 5.0f), "transition starts near old cam (~10)");
  ctx.tick(1.0); // t=0.5 over 2s -> ~ halfway (z ~ 60)
  check(feq(ctx.cameraWorldPosition().z, 60, 5.0f), "transition halfway (~60)");
  ctx.tick(2.0); // complete -> drives to B + posts transitionComplete
  check(feq(ctx.cameraWorldPosition().z, 110, 1e-2f), "transition ends at B (110)");
  ctx.tick(2.0); // drain the posted transitionComplete into the field
  check(nav->getTransitionComplete() == true, "BIND-05: transitionComplete emitted");
}

// (7) The transition uses the BINDING viewpoint's own navigationInfo (if it
// names one), not the previously-bound NavigationInfo: a TELEPORT navigationInfo
// must jump instantly even if the old NavigationInfo was a slow LINEAR.
void test_per_viewpoint_navinfo_teleport() {
  auto nav0 = std::make_shared<NavigationInfo>(); nav0->setTransitionTime(5.0); // bound, slow LINEAR
  auto niB = std::make_shared<NavigationInfo>();
  niB->setTransitionType(std::vector<NavigationTransitionTypeValues>{NavigationTransitionTypeValues::TELEPORT});
  auto A = std::make_shared<Viewpoint>(); A->setPosition(SFVec3f{0, 0, 10});
  auto B = std::make_shared<Viewpoint>(); B->setPosition(SFVec3f{0, 0, 110});
  B->setNavigationInfo(std::static_pointer_cast<X3DNode>(niB));
  Scene scene; scene.addRootNode(nav0); scene.addRootNode(A); scene.addRootNode(B);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  attachViewpointBind(ctx);
  ctx.tick(0.0);
  ctx.postEvent(B.get(), "set_bind", std::any(SFBool{true}));
  ctx.tick(0.0); // B's own navInfo is TELEPORT -> instant jump (no 5s animation)
  check(feq(ctx.cameraWorldPosition().z, 110),
        "transition uses the binding viewpoint's navigationInfo (TELEPORT -> instant)");
}

// (8) A rotation-only bind (same eye position, different orientation) still
// animates over transitionTime (the guard must consider orientation, not just
// translation).
void test_rotation_only_transition() {
  auto nav = std::make_shared<NavigationInfo>(); nav->setTransitionTime(2.0);
  auto A = std::make_shared<Viewpoint>(); A->setPosition(SFVec3f{0, 0, 10});
  auto B = std::make_shared<Viewpoint>(); B->setPosition(SFVec3f{0, 0, 10});
  B->setOrientation(SFRotation{0, 1, 0, 3.14159f}); // same eye, looks the other way
  Scene scene; scene.addRootNode(nav); scene.addRootNode(A); scene.addRootNode(B);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  attachViewpointBind(ctx);
  ctx.tick(0.0);
  ctx.postEvent(B.get(), "set_bind", std::any(SFBool{true}));
  ctx.tick(0.0); // bind B; rotation differs -> a transition must start
  ctx.tick(2.0); // complete
  ctx.tick(2.0); // drain transitionComplete
  check(nav->getTransitionComplete() == true,
        "rotation-only bind animates (transition ran, not an instant snap)");
}

// (9) BIND-09: a pop (set_bind FALSE on the bound vp) restores the popped-to
// vp's stored offset (§23.3.1 r6.3), NOT resetting it like a fresh push.
void test_pop_restores_stored_offset() {
  auto A = std::make_shared<Viewpoint>(); A->setPosition(SFVec3f{0, 0, 10});
  auto B = std::make_shared<Viewpoint>(); B->setPosition(SFVec3f{0, 0, 100});
  Scene scene; scene.addRootNode(A); scene.addRootNode(B);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  attachViewpointBind(ctx);
  ctx.tick(0.0);
  ctx.setViewpointOffset(A.get(), ViewpointOffset{Mat4::translation(SFVec3f{0, 0, -5})});
  ctx.tick(1.0);

  ctx.postEvent(B.get(), "set_bind", std::any(SFBool{true}));
  ctx.tick(2.0);
  check(ctx.boundViewpoint() == B.get(), "B bound after push");

  ctx.postEvent(B.get(), "set_bind", std::any(SFBool{false}));
  ctx.tick(3.0);
  check(ctx.boundViewpoint() == A.get(), "A bound after pop");
  check(feq(ctx.cameraWorldPosition().z, 5),
        "BIND-09: pop restores A's stored offset (0,0,5), not zero");
}

// (10) PUSH still resets (negative control for BIND-09).
void test_push_still_resets() {
  auto A = std::make_shared<Viewpoint>(); A->setPosition(SFVec3f{0, 0, 10});
  auto B = std::make_shared<Viewpoint>(); B->setPosition(SFVec3f{0, 0, 100});
  Scene scene; scene.addRootNode(A); scene.addRootNode(B);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  attachViewpointBind(ctx);
  ctx.tick(0.0);
  ctx.setViewpointOffset(A.get(), ViewpointOffset{Mat4::translation(SFVec3f{0, 0, -5})});
  ctx.tick(1.0);

  ctx.postEvent(B.get(), "set_bind", std::any(SFBool{true}));
  ctx.tick(2.0);
  ctx.postEvent(A.get(), "set_bind", std::any(SFBool{true}));
  ctx.tick(3.0);
  check(feq(ctx.cameraWorldPosition().z, 10),
        "BIND-09 negative control: push A resets offset (0,0,10), NOT restore");
}

// (11) BIND-09: a pop still animates over transitionTime.
void test_pop_animates_over_transition_time() {
  auto nav = std::make_shared<NavigationInfo>(); nav->setTransitionTime(2.0);
  auto A = std::make_shared<Viewpoint>(); A->setPosition(SFVec3f{0, 0, 10});
  auto B = std::make_shared<Viewpoint>(); B->setPosition(SFVec3f{0, 0, 110});
  Scene scene; scene.addRootNode(nav); scene.addRootNode(A); scene.addRootNode(B);
  X3DExecutionContext ctx; ctx.buildSceneGraph(scene);
  attachViewpointBind(ctx);
  ctx.tick(0.0); // A bound, cam (0,0,10)
  ctx.setViewpointOffset(A.get(), ViewpointOffset{Mat4::translation(SFVec3f{0, 0, -5})});
  ctx.tick(1.0); // A's cam now (0,0,5), lastCam_=(0,0,5)

  // Push B: animates from (0,0,5) to (0,0,110) over 2s starting at t=1.0
  ctx.postEvent(B.get(), "set_bind", std::any(SFBool{true}));
  ctx.tick(1.0);  // bind B; transition starts at old cam (0,0,5)
  ctx.tick(3.0);  // t=(3-1)/2=1.0; complete -> cam at (0,0,110), posts transitionComplete
  ctx.tick(3.0);  // drain transitionComplete; lastCam_=(0,0,110)

  // Reset transitionComplete so we can verify the pop re-emits it.
  nav->emitTransitionComplete(false);

  // Pop B: animates from B's cam (0,0,110) to A's restored cam (0,0,5) over 2s
  ctx.postEvent(B.get(), "set_bind", std::any(SFBool{false}));
  ctx.tick(3.0);  // pop; transition starts at B's cam (0,0,110)
  check(feq(ctx.cameraWorldPosition().z, 110, 5.0f), "pop transition starts near B's cam (~110)");
  ctx.tick(4.0);  // t=(4-3)/2=0.5; halfway -> cam ~57.5
  check(feq(ctx.cameraWorldPosition().z, 57.5f, 5.0f), "pop transition halfway (~57.5)");
  ctx.tick(5.0);  // t=(5-3)/2=1.0; complete -> cam at A's restored (0,0,5), posts transitionComplete
  check(feq(ctx.cameraWorldPosition().z, 5, 1e-2f), "pop transition ends at A's stored offset (5)");
  ctx.tick(5.0);  // drain transitionComplete
  check(nav->getTransitionComplete() == true, "BIND-09: pop emits transitionComplete");
}

} // namespace

TEST_CASE("viewpoint_bind_test") {
  test_jump_false_continuous();
  test_jump_true_snaps();
  test_retain_user_offsets();
  test_navigationinfo_dispatch();
  test_delete_detach();
  test_bind_transition();
  test_per_viewpoint_navinfo_teleport();
  test_rotation_only_transition();
  test_pop_restores_stored_offset();
  test_push_still_resets();
  test_pop_animates_over_transition_time();
  if (failures) { std::cerr << failures << " check(s) failed\n"; CHECK(false); return; }
  std::cout << "all viewpoint-bind tests passed\n";
  return;
}
