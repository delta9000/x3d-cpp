#include "doctest/doctest.h"
// nav_arbitration_test.cpp — NavigationSystem honors the pointer-consumed flag
// (a sensor grab suppresses nav drag) and the forced-mode override.
#include "NavigationSystem.hpp"
#include "X3DDocument.hpp"
#include "X3DExecutionContext.hpp"
#include "x3d/nodes/Viewpoint.hpp"
#include "Ray.hpp"
#include <any>
#include <iostream>
#include <memory>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;

static int failures = 0;
static void check(bool c, const char *what) {
  if (!c) { std::cerr << "FAIL: " << what << "\n"; ++failures; }
  else    { std::cout << "ok: " << what << "\n"; }
}
static bool sameMatrix(const Mat4 &a, const Mat4 &b) {
  for (int i = 0; i < 16; ++i) if (std::fabs(a.m[i] - b.m[i]) > 1e-6f) return false;
  return true;
}

// A system that claims the pointer before NavigationSystem runs (added first).
struct Claimer : System {
  void attach(X3DNode *, X3DExecutionContext &) override {}
  void update(double, X3DExecutionContext &ctx) override {
    ctx.setPointerConsumedBySensor(true);
  }
};

static std::shared_ptr<Scene> sceneWithViewpoint() {
  auto vp = std::make_shared<Viewpoint>();
  auto scene = std::make_shared<Scene>();
  scene->addRootNode(vp);
  return scene;
}

// Feed a two-sample drag (button down, pointer moves) so NavigationSystem sees a
// nonzero dx. Returns the view matrix after the drag tick.
static Mat4 dragAndView(X3DExecutionContext &ctx) {
  ctx.setPointerPresent(true);
  ctx.setPointerButton(true);
  ctx.setPointer(Ray{{0, 0, 10}, {0, 0, -1}});
  ctx.setPointerScreen(0, 0); // mirror nav drag onto the screen pointer
  ctx.tick(1.0); // establishes lastPx_/lastPy_ (dragActive_ becomes true)
  ctx.setPointer(Ray{{0.3f, 0, 10}, {0, 0, -1}}); // move in x ⇒ dx != 0
  ctx.setPointerScreen(0.3f, 0); // mirror nav drag onto the screen pointer
  ctx.tick(2.0);
  return ctx.viewMatrix();
}

static void test_examine_drag_rotates_without_flag() {
  auto scene = sceneWithViewpoint();
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(*scene);
  ctx.addSystem(std::make_shared<NavigationSystem>()); // default EXAMINE
  Mat4 before = ctx.viewMatrix();
  Mat4 after = dragAndView(ctx);
  check(!sameMatrix(before, after), "examine drag rotates the view (control)");
}

static void test_flag_suppresses_nav_drag() {
  auto scene = sceneWithViewpoint();
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(*scene);
  ctx.addSystem(std::make_shared<Claimer>());          // claims pointer FIRST
  ctx.addSystem(std::make_shared<NavigationSystem>());
  Mat4 before = ctx.viewMatrix();
  Mat4 after = dragAndView(ctx);
  check(sameMatrix(before, after), "consumed flag suppresses nav drag");
}

static void test_forced_mode_fly_moves_on_key() {
  auto scene = sceneWithViewpoint();
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(*scene);
  auto nav = std::make_shared<NavigationSystem>();
  nav->setForcedMode(NavigationSystem::Mode::Fly);
  ctx.addSystem(nav);
  ctx.tick(0.0); // seed time
  Mat4 before = ctx.viewMatrix();
  ctx.setKey(NavigationSystem::kKeyForward, true);
  ctx.tick(0.5); // dt = 0.5 ⇒ fly translates forward
  check(!sameMatrix(before, ctx.viewMatrix()), "forced FLY + forward key moves camera");
}

TEST_CASE("nav_arbitration_test") {
  test_examine_drag_rotates_without_flag();
  test_flag_suppresses_nav_drag();
  test_forced_mode_fly_moves_on_key();
  CHECK(failures == 0);
  return;
}
