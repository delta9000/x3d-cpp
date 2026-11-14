// Extraction and picking must reject containment cycles even when called
// without buildSceneGraph() sanitizing the scene first.
#include "PickSystem.hpp"
#include "SceneExtractor.hpp"
#include "TransformSystem.hpp"
#include "BoundsSystem.hpp"
#include "X3DExecutionContext.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DScene.hpp"
#include "X3DDocument.hpp"

#include <any>
#include "doctest/doctest.h"
#include "scene_test_helpers.hpp"
#include <memory>
#include <vector>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;
using x3d::test::setF;
using x3d::test::addChild;

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
  // Bypass buildSceneGraph() so walk() receives an unsanitized graph.
  Scene scene;
  scene.addRootNode(makeCyclicScene());
  X3DExecutionContext ctx; // default ctx; the cycle in `scene` stays live.

  extract::SceneExtractor ex(ctx, scene);
  extract::RenderDelta snap = ex.fullSnapshot();
  CHECK(snap.added.size() == 1);

  // Sever the back-edge so the scene's shared_ptrs are collectible.
  breakContainmentCycles(scene);
}

TEST_CASE("pick_system_terminates_on_containment_cycle") {
  Scene scene;
  scene.addRootNode(makeCyclicScene());
  TransformSystem ts; ts.buildIndex(scene);
  BoundsSystem bs; bs.buildBounds(scene, ts);
  PickSystem ps; ps.build(scene);

  PickResult hit = ps.pickClosest(Ray{{0, 0, 10}, {0, 0, -1}}, bs);
  CHECK(hit.hit); // the Box at the origin is picked, and the walk terminated.

  breakContainmentCycles(scene); // teardown: sever A->B->A so the scene is collectible
}

TEST_CASE("pick_worldOf_terminates_on_containment_cycle") {
  Scene scene;
  scene.addRootNode(makeCyclicScene());
  PickSystem ps; ps.build(scene);

  // A target absent from the graph forces a full (cyclic) traversal.
  auto stranger = createX3DNode("Group");
  Mat4 w = ps.worldOf(stranger.get());
  // Not found -> identity, and the search terminated.
  CHECK(w.transformPoint({1, 2, 3}).x == doctest::Approx(1.0f));

  breakContainmentCycles(scene); // teardown: sever A->B->A so the scene is collectible
}
