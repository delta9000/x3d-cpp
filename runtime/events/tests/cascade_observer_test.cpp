// cascade_observer_test.cpp — the cascade notifies an observer for every field
// it delivers (the dirty-tracking feed).
#include "X3DEventCascade.hpp"
#include "X3DEventGraph.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"
#include "doctest/doctest.h"
#include <string>
#include <vector>
using namespace x3d::runtime;
using namespace x3d::core;
using namespace x3d::nodes;

TEST_CASE("cascade_observer_test") {
  auto a = createX3DNode("TimeSensor");
  auto b = createX3DNode("PositionInterpolator");
  // ROUTE a.fraction_changed -> b.set_fraction so a delivered event fans out.
  EventGraph g;
  g.addRoute(FieldAddress{a.get(), "fraction_changed"},
             FieldAddress{b.get(), "set_fraction"});
  EventCascade cascade(g);

  std::vector<std::pair<const X3DNode*, std::string>> seen;
  cascade.setFieldObserver([&](const FieldAddress &addr) {
    seen.emplace_back(addr.node, addr.field);
  });

  cascade.postEvent(a.get(), "fraction_changed", std::any(0.5f));
  cascade.process();

  // Observer fired for the seeded delivery AND the routed sink delivery.
  bool sawSource = false, sawSink = false;
  for (auto &p : seen) {
    if (p.first == a.get() && p.second == "fraction_changed") sawSource = true;
    if (p.first == b.get() && p.second == "set_fraction") sawSink = true;
  }
  CHECK((sawSource && sawSink));
  return;
}
