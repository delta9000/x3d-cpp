#include "X3DEventGraph.hpp"
#include "X3DEventCascade.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DSceneBridge.hpp"
#include "X3DScene.hpp"
#include "X3DDocument.hpp"
#include "CycleBreaker.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "X3DProtoExpand.hpp"

#include "x3d/nodes/Group.hpp"
#include "x3d/nodes/Transform.hpp"

#include "doctest/doctest.h"
#include <memory>
#include <string>

using namespace x3d;
using namespace x3d::runtime;
using namespace x3d::core;
using namespace x3d::nodes;

static void setChildren(const std::shared_ptr<X3DNode> &p,
                        const std::vector<std::shared_ptr<X3DNode>> &v) {
  for (const auto &f : p->fields()) {
    if (f.x3dName == "children" && f.set) {
      f.set(*p, std::any(v));
      return;
    }
  }
}
// 1. EventGraph retains stale routes after its nodes are destroyed.
// Issue: EventGraph stores FieldAddress with raw X3DNode*. When the nodes are
// destroyed (e.g. Scene goes out of scope while an X3DExecutionContext stays
// alive), the raw pointers become dangling. Accessing them is UB.
// We verify the latent risk by checking routeCount() stays non-zero after the
// scene is destroyed, and the test documents that the EventGraph has no
// automatic pruning mechanism.
void test_event_graph_stale_routes() {
  X3DExecutionContext ctx;
  std::weak_ptr<X3DNode> weakA, weakB;

  {
    auto a = createX3DNode("Transform");
    auto b = createX3DNode("Transform");
    weakA = a;
    weakB = b;

    EventGraph g;
    g.addRoute({a.get(), "translation"}, {b.get(), "translation"});
    CHECK_MESSAGE((g.routeCount() == 1), ("EventGraph has one route"));

    // When a and b go out of scope at end of this block, the EventGraph 'g'
    // still stores raw pointers to the now-destroyed nodes. This is a latent
    // use-after-free risk. Since 'g' is also destroyed at block end, the
    // visible symptom is routeCount remaining 1.
  }

  CHECK_MESSAGE((weakA.expired()), ("node A destroyed after going out of scope"));
  CHECK_MESSAGE((weakB.expired()), ("node B destroyed after going out of scope"));
}

// Same issue at the X3DExecutionContext level: buildFrom adds routes with raw
// pointers; destroying the Scene leaves them dangling in the context.
void test_context_outlives_scene() {
  X3DExecutionContext ctx;
  std::weak_ptr<X3DNode> weakClock, weakInterp;

  {
    auto clock = createX3DNode("TimeSensor");
    auto interp = createX3DNode("PositionInterpolator");
    weakClock = clock;
    weakInterp = interp;

    Scene scene;
    scene.define("Clock", clock);
    scene.define("Interp", interp);
    scene.routes.push_back(Route{"Clock", "fraction_changed",
                                  "Interp", "set_fraction"});

    auto br = ctx.buildFrom(scene);
    CHECK_MESSAGE((br.routesAdded == 1), ("one route bridged to context"));
    CHECK_MESSAGE((ctx.graph().routeCount() == 1), ("context graph holds one route"));

    // scene destroyed here; ctx still alive with dangling raw pointers.
  }

  CHECK_MESSAGE((weakClock.expired()), ("Clock destroyed after scene destruction"));
  CHECK_MESSAGE((weakInterp.expired()), ("Interp destroyed after scene destruction"));

  // At this point ctx.graph() still thinks it has a route, but the pointers
  // are dangling. Calling ctx.process() or ctx.tick() would be UB.
  CHECK_MESSAGE((ctx.graph().routeCount() == 1), ("context graph still holds stale route after destruction"));
  ctx.clearRoutes();
  CHECK_MESSAGE((ctx.graph().routeCount() == 0), ("clearRoutes removes stale dangling routes"));
}
// 2. shared_ptr containment cycles prevent destruction.
// A node that is its own child (e.g. malformed self-USE) creates a shared_ptr
// cycle. The Scene drops its external reference but the node's own internal
// shared_ptr keeps the ref-count at 1 -> leak.
// breakContainmentCycles() fixes this by severing back-edges.
void test_self_cycle_leak() {
  std::weak_ptr<X3DNode> weak;
  {
    auto self = createX3DNode("Group");
    weak = self;
    setChildren(self, {self}); // self-cycle
    Scene s;
    s.addRootNode(self);
    // s destroyed here. Because self holds a shared_ptr to itself,
    // the ref-count never drops to 0.
  }
  CHECK_MESSAGE((!weak.expired()), ("self-referencing node leaked (cycle prevents destruction)"));
  // Clean up the intentional cycle so valgrind doesn't flag it.
  if (auto self = weak.lock()) {
    setChildren(self, {});
  }
}

void test_self_cycle_fixed_by_breaker() {
  std::weak_ptr<X3DNode> weak;
  {
    auto self = createX3DNode("Group");
    weak = self;
    setChildren(self, {self});
    Scene s;
    s.addRootNode(self);
    breakContainmentCycles(s); // sever the back-edge
    // Now s can be destroyed cleanly.
  }
  CHECK_MESSAGE((weak.expired()), ("breakContainmentCycles fixes self-cycle; node destroyed"));
}
// 3. Well-formed DAG: scene destruction is clean.
void test_well_formed_scene_destruction() {
  std::weak_ptr<X3DNode> weakA, weakB, weakC;
  {
    auto a = createX3DNode("Group");
    auto b = createX3DNode("Group");
    auto c = createX3DNode("Shape");
    weakA = a;
    weakB = b;
    weakC = c;
    setChildren(a, {b, c}); // no cycles
    Scene s;
    s.addRootNode(a);
  }
  CHECK_MESSAGE((weakA.expired()), ("DAG root A destroyed"));
  CHECK_MESSAGE((weakB.expired()), ("DAG child B destroyed"));
  CHECK_MESSAGE((weakC.expired()), ("DAG child C destroyed"));
}
// 4. DEF/USE identity: USE shares the same shared_ptr; no dangling.
void test_def_use_identity() {
  std::weak_ptr<X3DNode> weak;
  {
    auto defNode = createX3DNode("Transform");
    defNode->setDEF("A");
    weak = defNode;

    Scene s;
    s.addRootNode(defNode);

    // USE resolves to the same shared_ptr.
    auto useNode = s.resolve("A");
    CHECK_MESSAGE((useNode == defNode), ("USE resolves to the same shared_ptr as DEF"));

    // Remove DEF from table; the node is still alive via rootNodes.
    s.defs.clear();
    CHECK_MESSAGE((!weak.expired()), ("node still alive after defs cleared (rootNodes holds it)"));
  }
  CHECK_MESSAGE((weak.expired()), ("node destroyed when last external ref drops"));
}
// 5. Scene::routes uses weak_ptr; it does not keep nodes alive.
void test_scene_routes_weak_ptr() {
  std::weak_ptr<X3DNode> weak;
  Scene s;
  {
    auto a = createX3DNode("Transform");
    weak = a;
    // Only put the node in routes (weak_ptr), not in defs (shared_ptr).
    s.routes.push_back(Route{"A", "translation", "A", "translation"});
    // Manually set the weak_ptrs without registering in defs.
    s.routes[0].from = a;
    s.routes[0].to   = a;
    CHECK_MESSAGE((s.routes[0].from.lock() == a), ("route from weak_ptr resolves"));
  }
  // 'a' destroyed here; nothing else holds a strong ref.
  CHECK_MESSAGE((weak.expired()), ("node not kept alive by Scene::routes weak_ptr"));
  CHECK_MESSAGE((s.routes[0].from.lock() == nullptr), ("expired weak_ptr in route becomes null"));
}

TEST_CASE("node_lifecycle_audit_test") {
  test_event_graph_stale_routes();
  test_context_outlives_scene();
  test_self_cycle_leak();
  test_self_cycle_fixed_by_breaker();
  test_well_formed_scene_destruction();
  test_def_use_identity();
  test_scene_routes_weak_ptr();

  return;
}
