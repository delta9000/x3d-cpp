#include "doctest/doctest.h"
// cascade_test.cpp
// Tests for the X3D event cascade engine (runtime/events/). Builds nodes
// programmatically, wires ROUTEs, posts initial events, runs the cascade, and
// asserts values propagate with X3D single-timestamp semantics (fan-out +
// per-route loop-breaking).
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "X3DEventCascade.hpp"
#include "X3DEventGraph.hpp"

#include "PositionInterpolator.hpp"
#include "Transform.hpp"

#include <any>
#include <iostream>
#include <memory>
#include <string>

using namespace x3d;
using namespace x3d::runtime;

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

// A single ROUTE A.translation -> B.translation delivers the posted value.
void test_single_route() {
  auto a = std::make_shared<Transform>();
  auto b = std::make_shared<Transform>();

  EventGraph graph;
  graph.addRoute({a.get(), "translation"}, {b.get(), "translation"});

  EventCascade cascade(graph);
  cascade.postEvent(a.get(), "translation", std::any(SFVec3f{1, 2, 3}));
  cascade.process();

  check(veq(tr(a), 1, 2, 3), "source field set by postEvent");
  check(veq(tr(b), 1, 2, 3), "single route propagated value to sink");
}

// Fan-out: one source field routed to two sinks updates both.
void test_fanout() {
  auto a = std::make_shared<Transform>();
  auto b = std::make_shared<Transform>();
  auto c = std::make_shared<Transform>();

  EventGraph graph;
  graph.addRoute({a.get(), "translation"}, {b.get(), "translation"});
  graph.addRoute({a.get(), "translation"}, {c.get(), "translation"});

  EventCascade cascade(graph);
  cascade.postEvent(a.get(), "translation", std::any(SFVec3f{4, 5, 6}));
  cascade.process();

  check(veq(tr(b), 4, 5, 6), "fan-out reached sink b");
  check(veq(tr(c), 4, 5, 6), "fan-out reached sink c");
}

// A cyclic route graph (A->B->A) must terminate: each route fires at most once
// per cascade (X3D single-timestamp loop-breaking).
void test_cycle_terminates() {
  auto a = std::make_shared<Transform>();
  auto b = std::make_shared<Transform>();

  EventGraph graph;
  graph.addRoute({a.get(), "translation"}, {b.get(), "translation"});
  graph.addRoute({b.get(), "translation"}, {a.get(), "translation"});

  EventCascade cascade(graph);
  cascade.postEvent(a.get(), "translation", std::any(SFVec3f{7, 8, 9}));
  cascade.process(); // must return (no infinite loop)

  check(veq(tr(a), 7, 8, 9), "cycle: a holds value");
  check(veq(tr(b), 7, 8, 9), "cycle: b received value once, then loop broke");
}

// An event delivered to an inputOnly field invokes its registered handler
// exactly once with the delivered value.
void test_input_only_delivery() {
  auto interp = std::make_shared<PositionInterpolator>();

  int calls = 0;
  SFFloat received = -1.0f;
  interp->setOnSet_fractionHandler([&](const SFFloat &v) {
    ++calls;
    received = v;
  });

  EventGraph graph; // no routes needed: deliver straight to the sink field
  EventCascade cascade(graph);
  cascade.postEvent(interp.get(), "set_fraction", std::any(SFFloat{0.5f}));
  cascade.process();

  check(calls == 1, "inputOnly handler invoked exactly once");
  check(received == 0.5f, "inputOnly handler received the delivered value");
}

// An outputOnly field can be written (an emitted output event), which both
// updates the field and cascades along its ROUTEs to inputOutput sinks.
void test_output_only_source() {
  auto interp = std::make_shared<PositionInterpolator>();
  auto target = std::make_shared<Transform>();

  EventGraph graph;
  graph.addRoute({interp.get(), "value_changed"}, {target.get(), "translation"});

  EventCascade cascade(graph);
  // A behavior would emit its output here; we drive it directly.
  cascade.postEvent(interp.get(), "value_changed", std::any(SFVec3f{1, 0, 2}));
  cascade.process();

  check(veq(interp->getValue_changed(), 1, 0, 2),
        "outputOnly field updated by emitted event");
  check(veq(tr(target), 1, 0, 2),
        "outputOnly source cascaded to inputOutput sink");
}

} // namespace

TEST_CASE("cascade_test") {
  test_single_route();
  test_fanout();
  test_cycle_terminates();
  test_input_only_delivery();
  test_output_only_source();

  if (failures) {
    std::cerr << failures << " check(s) failed\n";
    CHECK(false); return;
  }
  std::cout << "all cascade tests passed\n";
  return;
}
