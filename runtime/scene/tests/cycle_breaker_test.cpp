// cycle_breaker_test.cpp — breakContainmentCycles severs back-edges, preserves DAGs.
// Regression for the corpus-smoke SEGV: a malformed <X DEF='a' USE='a'/> creates a
// containment cycle that made every recursive walker stack-overflow. The sanitizer
// runs once in buildSceneGraph and turns the graph into a DAG; this test pins its
// contract directly.
#include "CycleBreaker.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DScene.hpp"
#include "X3DDocument.hpp"  // out-of-line Scene::addRootNode definition
#include <any>
#include "doctest/doctest.h"
#include <memory>
#include <vector>
using namespace x3d::runtime;
using namespace x3d::nodes;

static std::vector<std::shared_ptr<X3DNode>> kids(const std::shared_ptr<X3DNode>& p) {
  for (auto& f : p->fields())
    if (f.x3dName == "children" && f.get)
      return std::any_cast<std::vector<std::shared_ptr<X3DNode>>>(f.get(*p));
  return {};
}
static void addChild(const std::shared_ptr<X3DNode>& p, const std::shared_ptr<X3DNode>& c) {
  for (auto& f : p->fields()) if (f.x3dName == "children" && f.set) {
    auto k = kids(p); k.push_back(c); f.set(*p, std::any(std::move(k))); return;
  }
}

TEST_CASE("cycle_breaker_test") {
  // 1) 2-cycle A -> B -> A : one back-edge severed; A's legit child kept.
  {
    auto a = createX3DNode("Group");
    auto b = createX3DNode("Group");
    auto leaf = createX3DNode("Shape");
    addChild(a, leaf);
    addChild(a, b);
    addChild(b, a);                       // back-edge
    Scene s; s.addRootNode(a);
    int n = breakContainmentCycles(s);
    CHECK((n == 1));
    CHECK((kids(b).empty()));              // back-edge to ancestor A removed
    CHECK((kids(a).size() == 2));          // A's own children untouched
    CHECK((breakContainmentCycles(s) == 0));  // idempotent
  }

  // 2) Self-cycle (the exact DEF==USE shape): severed.
  {
    auto self = createX3DNode("Group");
    addChild(self, self);
    Scene s; s.addRootNode(self);
    CHECK((breakContainmentCycles(s) == 1));
    CHECK((kids(self).empty()));
  }

  // 3) Legitimate DAG: a shared node reached by two DISTINCT non-ancestor paths
  //    is NOT a cycle and must be preserved (USE-sharing).
  {
    auto root = createX3DNode("Group");
    auto p1 = createX3DNode("Group");
    auto p2 = createX3DNode("Group");
    auto shared = createX3DNode("Shape");
    addChild(root, p1);
    addChild(root, p2);
    addChild(p1, shared);
    addChild(p2, shared);                 // shared, but neither path is an ancestor
    Scene s; s.addRootNode(root);
    CHECK((breakContainmentCycles(s) == 0));   // nothing severed
    CHECK((kids(p1).size() == 1 && kids(p2).size() == 1));
  }
  return;
}
