#include "doctest/doctest.h"
// cascade_dynamic_route_test.cpp
// Regression tests for RTC-7: mid-cascade ROUTE mutation must not crash (no
// iterator/reference invalidation), dynamically-added routes are honored on the
// NEXT cascade, and EventGraph::removeRoute stops delivery (dynamic rerouting,
// X3D 19775-1 §4.3.7).
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "X3DEventCascade.hpp"
#include "X3DEventGraph.hpp"

#include "x3d/nodes/Transform.hpp"

#include <any>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace x3d;
using namespace x3d::runtime;
using namespace x3d::core;
using namespace x3d::nodes;

namespace {

int failures = 0;

void check(bool cond, const std::string &what) {
  if (!cond) {
    std::cerr << "FAIL: " << what << "\n";
    ++failures;
  } else {
    std::cout << "ok: " << what << "\n";
  }
}

SFVec3f tr(const std::shared_ptr<Transform> &t) { return t->getTranslation(); }
bool veq(const SFVec3f &a, float x, float y, float z) {
  return a.x == x && a.y == y && a.z == z;
}

// A handler that fires mid-cascade adds a brand-new ROUTE to the very graph the
// cascade is iterating. This must not invalidate any reference held across the
// handler call (no crash/UB); the new edge is honored on the NEXT cascade.
void test_mid_cascade_addroute_no_crash() {
  auto a = std::make_shared<Transform>();
  auto b = std::make_shared<Transform>();
  auto c = std::make_shared<Transform>();

  EventGraph graph;
  // Many sources fanning from a.translation, to force the routes_ map to hold
  // several buckets so a mid-iteration insert is likely to rehash/realloc.
  graph.addRoute({a.get(), "translation"}, {b.get(), "translation"});

  // Real nodes used only as distinct route-source keys to force a rehash of
  // routes_; held alive for the duration of the cascade. (Using real nodes —
  // not fake pointers — lets EventGraph dereference every key safely for
  // §4.4.2.2 alias resolution.)
  std::vector<std::shared_ptr<Transform>> fillers;
  for (int i = 0; i < 64; ++i) fillers.push_back(std::make_shared<Transform>());

  EventCascade cascade(graph);
  bool added = false;
  cascade.setFieldObserver([&](const FieldAddress &addr) {
    // When b is delivered, dynamically wire a new route b -> c. This mutates
    // the same routes_ container the cascade is mid-drain over.
    if (!added && addr.node == b.get() && addr.field == "translation") {
      added = true;
      for (int i = 0; i < 64; ++i) {
        // Insert many distinct keys to provoke a rehash of routes_.
        graph.addRoute({fillers[i].get(), "translation"},
                       {c.get(), "translation"});
      }
      graph.addRoute({b.get(), "translation"}, {c.get(), "translation"});
    }
  });

  cascade.postEvent(a.get(), "translation", std::any(SFVec3f{1, 2, 3}));
  cascade.process(); // must not crash even though the graph was mutated mid-drain

  check(veq(tr(b), 1, 2, 3), "mid-cascade mutation: b still got its value");
  // The new b->c route was added during the cascade; X3D delivers it on the
  // NEXT cascade, not this one.
  check(veq(tr(c), 0, 0, 0), "dynamically-added route NOT honored on current cascade");

  // NEXT cascade: the b->c edge is now live.
  EventCascade next(graph);
  next.postEvent(b.get(), "translation", std::any(SFVec3f{4, 5, 6}));
  next.process();
  check(veq(tr(c), 4, 5, 6), "dynamically-added route honored on next cascade");
}

// removeRoute stops delivery along that edge.
void test_remove_route_stops_delivery() {
  auto a = std::make_shared<Transform>();
  auto b = std::make_shared<Transform>();

  EventGraph graph;
  graph.addRoute({a.get(), "translation"}, {b.get(), "translation"});
  check(graph.routeCount() == 1, "route registered");

  graph.removeRoute({a.get(), "translation"}, {b.get(), "translation"});
  check(graph.routeCount() == 0, "removeRoute removed the edge");

  EventCascade cascade(graph);
  cascade.postEvent(a.get(), "translation", std::any(SFVec3f{9, 9, 9}));
  cascade.process();
  check(veq(tr(a), 9, 9, 9), "source still set");
  check(veq(tr(b), 0, 0, 0), "removed route delivers nothing to sink");
}

} // namespace

TEST_CASE("cascade_dynamic_route_test") {
  test_mid_cascade_addroute_no_crash();
  test_remove_route_stops_delivery();

  if (failures) {
    std::cerr << failures << " check(s) failed\n";
    CHECK(false); return;
  }
  std::cout << "all dynamic-route tests passed\n";
  return;
}
