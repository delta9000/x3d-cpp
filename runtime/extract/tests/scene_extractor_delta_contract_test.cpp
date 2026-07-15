// scene_extractor_delta_contract_test.cpp — DELTA-CONTRACT regression.
//
// delta()'s one-delta-per-tick guard used to key on ctx_.now() and enforce itself
// with assert(). That was wrong twice over:
//
//   * `now` is embedder-supplied and may legitimately REPEAT — a paused scene
//     that still ticks, a fixed-timestep driver, deterministic replay. Those
//     consumers tripped `assert(now != lastDeltaNow_)` for doing nothing wrong.
//   * assert() compiles out under NDEBUG, and this repo's `ci` preset is
//     RelWithDebInfo. So the contract a consumer tested against in a debug build
//     was not the contract it shipped with.
//
// The guard now keys on X3DExecutionContext::tickGeneration() — a monotonic
// advance count that cannot repeat — and every previously-asserted misuse has a
// defined answer. These cases pin that the contract is TOTAL: no call sequence
// is undefined, and none of them depend on NDEBUG.

#include "SceneExtractor.hpp"

#include "X3DExecutionContext.hpp"
#include "X3DSceneBridge.hpp" // BridgeResult (buildFrom's return type)
#include "X3DScene.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"

#include <any>
#include "doctest/doctest.h"
#include <cstdint>
#include <memory>
#include <vector>

using namespace x3d::runtime;
using namespace x3d::runtime::extract;
using namespace x3d::core;
using namespace x3d::nodes;

namespace {

void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}
void addChild(const std::shared_ptr<X3DNode> &p,
              const std::shared_ptr<X3DNode> &c) {
  for (auto &f : p->fields())
    if (f.x3dName == "children" && f.set) {
      auto k = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*p));
      k.push_back(c);
      f.set(*p, std::any(std::move(k)));
      return;
    }
}
// One Transform over one Box — the Transform is what we animate.
Scene makeScene(std::shared_ptr<X3DNode> &xfOut) {
  auto box = createX3DNode("Box");
  auto shape = createX3DNode("Shape");
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(box)));
  auto xf = createX3DNode("Transform");
  addChild(xf, shape);
  Scene scene;
  scene.rootNodes.push_back(xf);
  xfOut = xf;
  return scene;
}

} // namespace

TEST_CASE("delta contract: tickGeneration is monotonic and clock-independent") {
  X3DExecutionContext ctx;
  CHECK(ctx.tickGeneration() == 0); // no advance has happened yet.

  ctx.tick(1.0);
  CHECK(ctx.tickGeneration() == 1);

  // A PAUSED consumer: same clock value, real advances. now() cannot tell these
  // apart; the generation can.
  ctx.tick(1.0);
  ctx.tick(1.0);
  CHECK(ctx.tickGeneration() == 3);
  CHECK(ctx.now() == 1.0);

  // Even a clock that goes BACKWARDS (replay/scrub) still advances generation.
  ctx.tick(0.5);
  CHECK(ctx.tickGeneration() == 4);
}

TEST_CASE("delta contract: a PAUSED clock still yields deltas") {
  // The regression that motivated this: tick() at an unchanging timestamp is a
  // legitimate pattern (the cascade still runs), and every delta() after the
  // first used to trip the now()-keyed assert.
  std::shared_ptr<X3DNode> xf;
  Scene scene = makeScene(xf);

  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  ctx.buildFrom(scene);
  SceneExtractor ex(ctx, scene);
  ex.fullSnapshot();

  for (int i = 1; i <= 3; ++i) {
    ctx.tick(2.0); // clock FROZEN across every iteration.
    ctx.writeField(xf.get(), "translation",
                   std::any(SFVec3f{static_cast<float>(i), 0, 0}));
    RenderDelta d = ex.delta();
    // Each frozen-clock tick still reports its own change.
    CHECK(d.updatedTransform.size() == 1);
  }
}

TEST_CASE("delta contract: second delta() with no tick returns EMPTY, not garbage") {
  std::shared_ptr<X3DNode> xf;
  Scene scene = makeScene(xf);

  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  ctx.buildFrom(scene);
  SceneExtractor ex(ctx, scene);
  ex.fullSnapshot();

  ctx.tick(1.0);
  ctx.writeField(xf.get(), "translation", std::any(SFVec3f{5, 0, 0}));

  RenderDelta first = ex.delta();
  CHECK(first.updatedTransform.size() == 1);

  // No tick() in between: nothing CAN have changed, so the honest answer is an
  // empty delta. (Previously: assert in debug, bogus re-diff under NDEBUG.)
  RenderDelta second = ex.delta();
  CHECK(second.updatedTransform.empty());
  CHECK(second.added.empty());
  CHECK(second.removed.empty());
  CHECK(second.updatedGeometry.empty());
  CHECK(second.updatedMaterial.empty());

  // ...and a real advance resumes reporting.
  ctx.tick(2.0);
  ctx.writeField(xf.get(), "translation", std::any(SFVec3f{6, 0, 0}));
  RenderDelta third = ex.delta();
  CHECK(third.updatedTransform.size() == 1);
}

TEST_CASE("delta contract: delta() with no prior fullSnapshot yields the baseline") {
  // No baseline to diff against => a full snapshot IS the baseline. Every item
  // lands in `added`, which is the same upload path frame 0 uses, so a consumer
  // that skipped fullSnapshot() still gets a correct first frame rather than an
  // assert (debug) or a walk over zero-initialised state (NDEBUG).
  std::shared_ptr<X3DNode> xf;
  Scene scene = makeScene(xf);

  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  ctx.buildFrom(scene);
  SceneExtractor ex(ctx, scene);

  RenderDelta d = ex.delta(); // no fullSnapshot() first.
  CHECK(d.added.size() == 1);
  CHECK(ex.itemCount() == 1);
  CHECK(d.cameraChanged);
  CHECK(d.backgroundChanged);
  CHECK(d.lightsChanged);

  // The promotion seeds the guard, so the NEXT delta() with no tick is empty
  // rather than a second full snapshot.
  RenderDelta again = ex.delta();
  CHECK(again.added.empty());
}
