// walker_cycle_guard_test.cpp
// Regression for MEM-1 (#13): BoundsSystem/TransformSystem already break a
// containment cycle (bounds_cycle_test), but SceneExtractor::walk and
// PickSystem::pickNode/worldOfRec carried neither a cycle nor a depth guard, so
// a USE-cyclic scene (A -> B -> A) SIGSEGV'd at extraction/pick time.
//
// Pre-fix each case below crashes the process. Post-fix the path-membership
// guard breaks the back-edge and every walker terminates.
#include "PickSystem.hpp"
#include "SceneExtractor.hpp"
#include "TransformSystem.hpp"
#include "BoundsSystem.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DNodeFactory.hpp"
#include "X3DScene.hpp"
#include "X3DDocument.hpp"

#include <any>
#include "doctest/doctest.h"
#include <memory>
#include <vector>

using namespace x3d::runtime;

static void setF(const std::shared_ptr<X3DNode> &n, const char *nm, std::any v) {
  for (auto &f : n->fields())
    if (f.x3dName == nm && f.set) { f.set(*n, std::move(v)); return; }
}
static void addChild(const std::shared_ptr<X3DNode> &p, const std::shared_ptr<X3DNode> &c) {
  for (auto &f : p->fields())
    if (f.x3dName == "children" && f.set) {
      auto k = std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*p));
      k.push_back(c);
      f.set(*p, std::any(std::move(k)));
      return;
    }
}

// Group A contains a real Box Shape and Group B; B contains A (the back-edge).
static std::shared_ptr<X3DNode> makeCyclicScene() {
  auto box = createX3DNode("Box");
  setF(box, "size", std::any(SFVec3f{2, 2, 2}));
  auto shape = createX3DNode("Shape");
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(box)));

  auto a = createX3DNode("Group");
  auto b = createX3DNode("Group");
  addChild(a, shape);
  addChild(a, b);
  addChild(b, a); // containment cycle A -> B -> A
  return a;
}

TEST_CASE("scene_extractor_terminates_on_containment_cycle") {
  // NB: deliberately NOT routed through X3DExecutionContext::buildSceneGraph,
  // which would sever the cycle up front (breakContainmentCycles). The point of
  // MEM-1 is that walk() must be self-protecting when handed an unsanitized
  // graph directly, exactly as the standalone pick walkers are below.
  Scene scene;
  scene.addRootNode(makeCyclicScene());
  X3DExecutionContext ctx; // default ctx; the cycle in `scene` stays live.

  extract::SceneExtractor ex(ctx, scene);
  extract::RenderDelta snap = ex.fullSnapshot(); // pre-fix: SIGSEGV
  // Reaching here proves the walk terminated; the one real Box Shape is emitted.
  CHECK(snap.added.size() == 1);

  // Teardown hygiene: the walk above ran on the UNSANITIZED cycle on purpose;
  // now sever the A->B->A back-edge so the scene's shared_ptrs are collectible
  // (the cycle would otherwise leak — caught by LeakSanitizer under the san preset).
  breakContainmentCycles(scene);
}

TEST_CASE("pick_system_terminates_on_containment_cycle") {
  Scene scene;
  scene.addRootNode(makeCyclicScene());
  TransformSystem ts; ts.buildIndex(scene);
  BoundsSystem bs; bs.buildBounds(scene, ts);
  PickSystem ps; ps.build(scene);

  PickResult hit = ps.pickClosest(Ray{{0, 0, 10}, {0, 0, -1}}, bs); // pre-fix: SIGSEGV
  CHECK(hit.hit); // the Box at the origin is picked, and the walk terminated.

  breakContainmentCycles(scene); // teardown: sever A->B->A so the scene is collectible
}

TEST_CASE("pick_worldOf_terminates_on_containment_cycle") {
  Scene scene;
  scene.addRootNode(makeCyclicScene());
  PickSystem ps; ps.build(scene);

  // A target absent from the graph forces a full (cyclic) traversal.
  auto stranger = createX3DNode("Group");
  Mat4 w = ps.worldOf(stranger.get()); // pre-fix: SIGSEGV
  // Not found -> identity, and the search terminated.
  CHECK(w.transformPoint({1, 2, 3}).x == doctest::Approx(1.0f));

  breakContainmentCycles(scene); // teardown: sever A->B->A so the scene is collectible
}
