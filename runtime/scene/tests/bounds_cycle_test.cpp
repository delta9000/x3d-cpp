// Regression for the corpus-smoke SEGV (stack overflow) on a containment cycle.
// A node that USEs its own DEF name (e.g. the deliberately-malformed
// TestSchematronDiagnostics.x3d: <TouchSensor DEF='X' USE='X'/>) resolves to
// itself, so the scene graph contains a cycle A->B->A. BoundsSystem::index() is
// cycle-safe (its `indexed_` guard stops re-walking) but records the back-edge
// into children_; compute()'s memo is POST-ORDER (local_[n] set only after all
// children finish), so an in-progress node on the cycle is never memoized when
// re-entered -> unbounded recursion -> stack overflow.
//
// Pre-fix: this SEGVs (crashes the process). Post-fix: the in-progress guard
// breaks the back-edge, buildBounds() terminates, and bounds stay finite.
#include "BoundsSystem.hpp"
#include "TransformSystem.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DScene.hpp"
#include "X3DDocument.hpp"
#include <any>
#include "doctest/doctest.h"
#include "scene_test_helpers.hpp"
#include <cmath>
#include <memory>
#include <vector>
using namespace x3d::runtime;
using namespace x3d::core;
using namespace x3d::nodes;
using x3d::test::setF;
using x3d::test::addChild;
using x3d::test::feq;

TEST_CASE("bounds_cycle_test") {
  // A real Box so the union is meaningful: Group L > Shape > Box(size 2).
  auto leaf = createX3DNode("Group");
  auto shape = createX3DNode("Shape");
  auto box = createX3DNode("Box"); setF(box, "size", std::any(SFVec3f{2,2,2}));
  setF(shape, "geometry", std::any(std::shared_ptr<X3DNode>(box)));
  addChild(leaf, shape);

  // Build a 2-cycle: A contains B and the real leaf; B contains A (back-edge).
  auto a = createX3DNode("Group");
  auto b = createX3DNode("Group");
  addChild(a, leaf);
  addChild(a, b);
  addChild(b, a);          // <-- containment cycle A -> B -> A

  // Also a degenerate self-cycle (the exact DEF==USE shape).
  auto selfie = createX3DNode("Group");
  addChild(selfie, selfie);
  addChild(a, selfie);

  Scene scene; scene.addRootNode(a);

  TransformSystem ts; ts.buildIndex(scene);
  BoundsSystem bs; bs.buildBounds(scene, ts);

  Aabb rb = bs.localBounds(a.get());
  CHECK((!rb.empty));
  CHECK((std::isfinite(rb.min.x) && std::isfinite(rb.max.x)));
  CHECK((feq(rb.min.x, -1) && feq(rb.max.x, 1)));   // = the Box bounds

  // Teardown hygiene: the assertions above deliberately keep the containment
  // cycles intact (proving BoundsSystem's in-progress guard is cycle-safe), but
  // those shared_ptr back-edges (b -> a, selfie -> selfie) would keep the nodes
  // alive past scope exit -> LeakSanitizer leak. Sever them after bounds are
  // checked.
  setF(b, "children", std::any(std::vector<std::shared_ptr<X3DNode>>{}));
  setF(selfie, "children", std::any(std::vector<std::shared_ptr<X3DNode>>{}));
  return;
}
