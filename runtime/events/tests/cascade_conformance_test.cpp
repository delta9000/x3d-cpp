#include "doctest/doctest.h"
// cascade_conformance_test.cpp
// Conformance regressions for the event-model semantics the BACKLOG gates on the
// Script/SAI un-tabling push (docs/superpowers/BACKLOG.md rows RTC-5, RTC-6):
//
//   RTC-5 — the cascade enforced per-ROUTE dedup but not the spec's per-FIELD
//           cap: "a node produces at most one event per field per timestamp"
//           (ISO 19775-1 §4.4.8.3). Fan-in (two ROUTEs into one field) must
//           deliver ONCE; a cyclic graph must produce each field at most once.
//
//   RTC-6 — X3DExecutionContext::tick did a single pass (systems update ->
//           cascade drain -> propagate). §4.4.8.3 step 4 requires re-evaluating
//           steps 2-3 until a pass produces no new events, so a sensor whose
//           input is driven by a route from a LATER-ordered sensor resolves
//           within ONE tick instead of lagging a frame. Termination is bounded
//           by the RTC-5 per-field cap.
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "X3DEventCascade.hpp"
#include "X3DEventGraph.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DSystem.hpp"

#include "x3d/nodes/Transform.hpp"

#include <any>
#include <iostream>
#include <map>
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

// RTC-5: fan-in (two ROUTEs into the SAME sink field) delivers ONCE per
// timestamp. The per-ROUTE guard does not catch this — the two edges have
// distinct sources — so without the per-field cap the sink is written twice.
void test_rtc5_fanin_delivers_once() {
  auto a = std::make_shared<Transform>();
  auto b = std::make_shared<Transform>();
  auto z = std::make_shared<Transform>();

  EventGraph graph;
  graph.addRoute({a.get(), "translation"}, {z.get(), "translation"});
  graph.addRoute({b.get(), "translation"}, {z.get(), "translation"});

  // Count deliveries per (node,field) via the field observer.
  std::map<std::pair<const X3DNode *, std::string>, int> delivered;

  EventCascade cascade(graph);
  cascade.setFieldObserver([&](const FieldAddress &addr) {
    delivered[{addr.node, addr.field}]++;
  });

  // Both sources carry the same value (the well-formed fan-in case).
  cascade.postEvent(a.get(), "translation", std::any(SFVec3f{5, 0, 0}));
  cascade.postEvent(b.get(), "translation", std::any(SFVec3f{5, 0, 0}));
  cascade.process();

  check(delivered[{z.get(), "translation"}] == 1,
        "RTC-5: fan-in sink delivered exactly once per timestamp");
  check(veq(tr(z), 5, 0, 0), "RTC-5: fan-in sink holds the routed value");
}

// RTC-5: a cyclic graph produces each field at most once per timestamp. Fan-in
// on a node that is also part of a cycle previously double-drove the loop; the
// per-field cap breaks it. Each (node,field) must be produced exactly once.
void test_rtc5_cyclic_redrive_broken() {
  auto a = std::make_shared<Transform>();
  auto b = std::make_shared<Transform>();

  EventGraph graph;
  graph.addRoute({a.get(), "translation"}, {b.get(), "translation"});
  graph.addRoute({b.get(), "translation"}, {a.get(), "translation"});

  std::map<std::pair<const X3DNode *, std::string>, int> delivered;

  EventCascade cascade(graph);
  cascade.setFieldObserver([&](const FieldAddress &addr) {
    delivered[{addr.node, addr.field}]++;
  });

  cascade.postEvent(a.get(), "translation", std::any(SFVec3f{7, 8, 9}));
  cascade.process(); // must terminate

  check(delivered[{a.get(), "translation"}] == 1,
        "RTC-5: cyclic node A produced at most once");
  check(delivered[{b.get(), "translation"}] == 1,
        "RTC-5: cyclic node B produced at most once");
  check(veq(tr(a), 7, 8, 9) && veq(tr(b), 7, 8, 9),
        "RTC-5: cyclic values settle");
}

// A minimal sensor-like System: each tick it posts the SAME value it observes on
// a watched (node,field) onto a target (node,field) via a route-bearing source.
// Used to build a two-stage sensor chain whose second stage is ordered earlier.

// Stage 1 (registered LATER): on tick, post `srcValue` onto src.translation.
// A ROUTE carries src.translation -> mid.translation.
class ProducerSystem : public System {
public:
  ProducerSystem(X3DNode *src, SFVec3f v) : src_(src), v_(v) {}
  void attach(X3DNode *, X3DExecutionContext &) override {}
  void update(double, X3DExecutionContext &ctx) override {
    if (!fired_) {
      ctx.postEvent(src_, "translation", std::any(v_));
      fired_ = true; // emit exactly once across the whole tick's re-eval passes
    }
  }

private:
  X3DNode *src_;
  SFVec3f v_;
  bool fired_ = false;
};

// Stage 2 (registered EARLIER): on tick, READ mid.translation and record it.
// Models a sensor whose evaluation depends on an input driven by a route from
// the later-ordered producer. Without the §4.4.8.3 step-4 re-eval loop this
// reads the STALE value (the producer + cascade run after it) and lags a frame.
class ConsumerSystem : public System {
public:
  explicit ConsumerSystem(Transform *mid) : mid_(mid) {}
  void attach(X3DNode *, X3DExecutionContext &) override {}
  void update(double, X3DExecutionContext &ctx) override {
    (void)ctx;
    observed_ = mid_->getTranslation();
    ++passes_;
  }
  SFVec3f observed() const { return observed_; }
  int passes() const { return passes_; }

private:
  Transform *mid_;
  SFVec3f observed_{0, 0, 0};
  int passes_ = 0;
};

// RTC-6: a producer->route->consumer chain whose consumer is ordered EARLIER
// must resolve within ONE tick (not lag a frame). The re-eval loop re-runs the
// systems pass after the cascade delivered into mid.translation, so the
// earlier-ordered consumer sees the fresh value on the second pass.
void test_rtc6_reeval_within_one_tick() {
  auto src = std::make_shared<Transform>();
  auto mid = std::make_shared<Transform>();

  X3DExecutionContext ctx;
  ctx.addRoute({src.get(), "translation"}, {mid.get(), "translation"});

  auto consumer = std::make_shared<ConsumerSystem>(mid.get());   // EARLIER
  auto producer = std::make_shared<ProducerSystem>(src.get(), SFVec3f{3, 0, 0});
  ctx.addSystem(consumer); // registered first -> updated first each pass
  ctx.addSystem(producer); // registered later -> updated after the consumer

  ctx.tick(0.0);

  check(veq(consumer->observed(), 3, 0, 0),
        "RTC-6: earlier-ordered consumer saw the routed value within one tick");
  check(veq(tr(mid), 3, 0, 0), "RTC-6: route delivered the value to mid");
  check(consumer->passes() >= 2,
        "RTC-6: tick re-evaluated systems until quiescence");
}

// RTC-6: termination. A producer that re-posts the SAME field every pass must
// not loop forever — the RTC-5 per-field cap bounds the re-eval loop. (If the
// loop did not terminate this test would hang rather than fail; reaching the
// assertions at all proves termination.)
void test_rtc6_terminates_under_per_field_cap() {
  auto src = std::make_shared<Transform>();
  auto mid = std::make_shared<Transform>();

  X3DExecutionContext ctx;
  ctx.addRoute({src.get(), "translation"}, {mid.get(), "translation"});

  // A producer that posts EVERY pass (no one-shot guard): the per-field cap must
  // stop src.translation/mid.translation from re-driving the loop forever.
  class GreedyProducer : public System {
  public:
    explicit GreedyProducer(X3DNode *src) : src_(src) {}
    void attach(X3DNode *, X3DExecutionContext &) override {}
    void update(double, X3DExecutionContext &ctx) override {
      ctx.postEvent(src_, "translation", std::any(SFVec3f{1, 1, 1}));
    }

  private:
    X3DNode *src_;
  };

  ctx.addSystem(std::make_shared<GreedyProducer>(src.get()));
  ctx.tick(0.0); // must return (per-field cap guarantees termination)

  check(veq(tr(mid), 1, 1, 1), "RTC-6: greedy producer's value delivered once");
}

} // namespace

TEST_CASE("cascade_conformance_test") {
  test_rtc5_fanin_delivers_once();
  test_rtc5_cyclic_redrive_broken();
  test_rtc6_reeval_within_one_tick();
  test_rtc6_terminates_under_per_field_cap();

  if (failures) {
    std::cerr << failures << " check(s) failed\n";
    CHECK(false); return;
  }
  std::cout << "all cascade conformance tests passed\n";
  return;
}
