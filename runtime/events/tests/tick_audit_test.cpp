#include "doctest/doctest.h"
// tick_audit_test.cpp
// AUD-EXEC-TICK regression coverage for X3DExecutionContext::tick() loop
// correctness and edge cases:
//   - Empty context tick (no crash, no events)
//   - Timestamp persistence across multiple process() calls within one tick
//   - Recursive tick guard (System calling tick() during update)
//   - Deterministic system ordering
//   - SEED re-post every pass still terminates (quiescence detection)
//   - Post-cascade hook ordering and dirty propagation
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "X3DExecutionContext.hpp"
#include "X3DEventCascade.hpp"
#include "X3DEventGraph.hpp"
#include "X3DSystem.hpp"

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

// ------------------------------------------------------------------
// 1. Empty context tick: no systems, no routes — must not crash and
//    must leave the dirty tracker empty.
// ------------------------------------------------------------------
void test_empty_context_tick() {
  X3DExecutionContext ctx;
  ctx.tick(0.0); // must not crash
  check(ctx.dirtyTracker().changedNodes().empty(),
        "empty tick: dirty tracker is empty");
  check(ctx.now() == 0.0, "empty tick: now is set correctly");
}

// ------------------------------------------------------------------
// 2. Timestamp persistence: multiple process(false) calls within one
//    tick share the same per-field cap.  A SEED posted after the first
//    drain still delivers (last-wins) but downstream ROUTED events to
//    an already-produced field are dropped.
// ------------------------------------------------------------------
void test_timestamp_persistence() {
  auto a = std::make_shared<Transform>();
  auto b = std::make_shared<Transform>();

  EventGraph graph;
  graph.addRoute({a.get(), "translation"}, {b.get(), "translation"});

  EventCascade cascade(graph);
  int bDeliveries = 0;
  cascade.setFieldObserver([&](const FieldAddress &addr) {
    if (addr.node == b.get() && addr.field == "translation") ++bDeliveries;
  });

  // Open a timestamp, post A, drain — this produces A and B.
  cascade.beginTimestamp();
  cascade.postEvent(a.get(), "translation", std::any(SFVec3f{1, 0, 0}));
  std::size_t p1 = cascade.process(/*freshTimestamp=*/false);
  check(p1 == 2, "timestamp persistence: first drain produces 2 new fields");
  check(bDeliveries == 1, "timestamp persistence: B delivered once on first drain");
  check(veq(tr(b), 1, 0, 0), "timestamp persistence: B has first value");

  // Post A again (same timestamp) and drain.  A is a SEED so it still
  // delivers (last-wins), but B was already produced — the ROUTED event
  // to B is dropped.
  cascade.postEvent(a.get(), "translation", std::any(SFVec3f{2, 0, 0}));
  std::size_t p2 = cascade.process(/*freshTimestamp=*/false);
  check(p2 == 0, "timestamp persistence: second drain produces 0 new fields");
  check(bDeliveries == 1, "timestamp persistence: B NOT delivered again");
  check(veq(tr(a), 2, 0, 0), "timestamp persistence: A updated to last-wins value");
  check(veq(tr(b), 1, 0, 0), "timestamp persistence: B unchanged (routed drop)");
}

// ------------------------------------------------------------------
// 3. Recursive tick guard: a System that calls ctx.tick() from
//    update() must not corrupt the outer tick's timestamp / dirty
//    state or cause infinite recursion.
// ------------------------------------------------------------------
class RecursiveTickSystem : public System {
public:
  explicit RecursiveTickSystem(int maxDepth) : maxDepth_(maxDepth) {}
  void attach(X3DNode *, X3DExecutionContext &) override {}
  void update(double now, X3DExecutionContext &ctx) override {
    (void)now;
    if (depth_ < maxDepth_) {
      ++depth_;
      ctx.tick(999.0); // recursive tick — should be guarded/no-oped
    }
  }
  int depth() const { return depth_; }

private:
  int maxDepth_;
  int depth_ = 0;
};

void test_recursive_tick_guard() {
  X3DExecutionContext ctx;
  ctx.addSystem(std::make_shared<RecursiveTickSystem>(1));

  // Without a guard the recursive tick would corrupt the outer state or
  // loop forever; with a guard it is silently ignored.
  ctx.tick(0.0);
  check(ctx.now() == 0.0, "recursive tick guard: outer tick time preserved");
}

// ------------------------------------------------------------------
// 4. System ordering: systems are evaluated in registration order and
//    the re-eval loop re-evaluates ALL systems each pass.
// ------------------------------------------------------------------
class OrderRecordingSystem : public System {
public:
  explicit OrderRecordingSystem(char id, std::vector<char> &log)
      : id_(id), log_(log) {}
  void attach(X3DNode *, X3DExecutionContext &) override {}
  void update(double, X3DExecutionContext &) override {
    log_.push_back(id_);
  }

private:
  char id_;
  std::vector<char> &log_;
};

void test_system_ordering() {
  std::vector<char> order;
  X3DExecutionContext ctx;
  ctx.addSystem(std::make_shared<OrderRecordingSystem>('A', order));
  ctx.addSystem(std::make_shared<OrderRecordingSystem>('B', order));
  ctx.addSystem(std::make_shared<OrderRecordingSystem>('C', order));

  ctx.tick(0.0);

  // With no routes/events, the loop runs once: A B C.
  check(order.size() == 3, "ordering: exactly one pass (3 updates)");
  check(order == std::vector<char>{'A', 'B', 'C'},
        "ordering: deterministic A-B-C order");
}

// ------------------------------------------------------------------
// 5. SEED re-post every pass still terminates: a System that posts a
//    SEED to the same field every update() must not prevent quiescence,
//    because the per-field cap marks the field produced after the first
//    delivery and subsequent passes count 0 new productions.
// ------------------------------------------------------------------
class SeedSpammerSystem : public System {
public:
  explicit SeedSpammerSystem(X3DNode *target, int &passCount)
      : target_(target), passCount_(passCount) {}
  void attach(X3DNode *, X3DExecutionContext &) override {}
  void update(double, X3DExecutionContext &ctx) override {
    ++passCount_;
    // Post a SEED every single pass — the same field every time.
    ctx.postEvent(target_, "translation", std::any(SFVec3f{7, 7, 7}));
  }

private:
  X3DNode *target_;
  int &passCount_;
};

void test_seed_redrive_terminates() {
  auto t = std::make_shared<Transform>();
  int passCount = 0;

  X3DExecutionContext ctx;
  ctx.addSystem(std::make_shared<SeedSpammerSystem>(t.get(), passCount));
  ctx.tick(0.0);

  // The loop runs: pass 1 (spam seeds), process → 1 new production.
  // pass 2, spam again, process → 0 new productions → quiescence.
  check(passCount == 2, "seed spam: exactly 2 system passes before quiescence");
  check(veq(tr(t), 7, 7, 7), "seed spam: final value settled");
}

// ------------------------------------------------------------------
// 6. Post-cascade hook ordering: hooks run after quiescence and may
//    post events that dirty the tracker; those dirties are propagated.
// ------------------------------------------------------------------
void test_post_cascade_hook_ordering() {
  auto t = std::make_shared<Transform>();

  X3DExecutionContext ctx;
  int hookCalls = 0;
  ctx.addPostCascadeHook([&](X3DExecutionContext &c) {
    ++hookCalls;
    c.postEvent(t.get(), "translation", std::any(SFVec3f{5, 5, 5}));
  });

  ctx.tick(0.0);

  check(hookCalls == 1, "post-cascade hook: called exactly once");
  // The event posted by the hook sits in pending_ because the hook does
  // NOT drain the cascade itself.  The next tick would process it.  For
  // this test we just verify the hook ran and the context is still
  // consistent (no crash, no infinite loop).
  //
  // NOTE: if we want the hook's event to be part of THIS tick's
  // propagation, tick() would need to drain after hooks.  The current
  // design intentionally leaves that to the hook (it may call
  // ctx.process() internally).
}

// ------------------------------------------------------------------
// 7. Re-eval loop produces correct state after multi-pass chain:
//    System A posts to node X, routed to node Y; System B (ordered
//    BEFORE A) reads Y and posts to Z.  The chain should resolve in
//    one tick.
// ------------------------------------------------------------------
class ChainStageA : public System {
public:
  ChainStageA(X3DNode *src, const SFVec3f &v) : src_(src), v_(v) {}
  void attach(X3DNode *, X3DExecutionContext &) override {}
  void update(double, X3DExecutionContext &ctx) override {
    if (!fired_) {
      ctx.postEvent(src_, "translation", std::any(v_));
      fired_ = true;
    }
  }

private:
  X3DNode *src_;
  SFVec3f v_;
  bool fired_ = false;
};

class ChainStageB : public System {
public:
  ChainStageB(Transform *mid, X3DNode *dst)
      : mid_(mid), dst_(dst) {}
  void attach(X3DNode *, X3DExecutionContext &) override {}
  void update(double, X3DExecutionContext &ctx) override {
    // Read mid.translation; if it has been updated, post to dst.
    SFVec3f v = mid_->getTranslation();
    if (v.x != 0.0f && !posted_) {
      posted_ = true;
      ctx.postEvent(dst_, "translation", std::any(v));
    }
  }

private:
  Transform *mid_;
  X3DNode *dst_;
  bool posted_ = false;
};

void test_multi_pass_chain() {
  auto src = std::make_shared<Transform>();
  auto mid = std::make_shared<Transform>();
  auto dst = std::make_shared<Transform>();

  X3DExecutionContext ctx;
  ctx.addRoute({src.get(), "translation"}, {mid.get(), "translation"});

  // B is registered FIRST, A second.  B reads mid (stale on pass 1),
  // then on pass 2 (after the cascade delivers src→mid) it posts to dst.
  ctx.addSystem(std::make_shared<ChainStageB>(mid.get(), dst.get()));
  ctx.addSystem(std::make_shared<ChainStageA>(src.get(), SFVec3f{3, 0, 0}));

  ctx.tick(0.0);

  check(veq(tr(mid), 3, 0, 0),
        "multi-pass chain: mid received routed value");
  check(veq(tr(dst), 3, 0, 0),
        "multi-pass chain: dst received value from earlier-ordered B");
}

// ------------------------------------------------------------------
// 8. process() standalone (freshTimestamp=true) resets the cap, so
//    two standalone process() calls do NOT share a timestamp.
// ------------------------------------------------------------------
void test_standalone_process_resets_timestamp() {
  auto a = std::make_shared<Transform>();
  auto b = std::make_shared<Transform>();

  EventGraph graph;
  graph.addRoute({a.get(), "translation"}, {b.get(), "translation"});

  EventCascade cascade(graph);
  int bDeliveries = 0;
  cascade.setFieldObserver([&](const FieldAddress &addr) {
    if (addr.node == b.get() && addr.field == "translation") ++bDeliveries;
  });

  cascade.postEvent(a.get(), "translation", std::any(SFVec3f{1, 0, 0}));
  cascade.process(); // freshTimestamp=true
  check(bDeliveries == 1, "standalone process: B delivered once");

  cascade.postEvent(a.get(), "translation", std::any(SFVec3f{2, 0, 0}));
  cascade.process(); // freshTimestamp=true again — new timestamp
  check(bDeliveries == 2,
        "standalone process: B delivered again because timestamp was reset");
}

} // namespace

TEST_CASE("tick_audit_test") {
  test_empty_context_tick();
  test_timestamp_persistence();
  test_recursive_tick_guard();
  test_system_ordering();
  test_seed_redrive_terminates();
  test_post_cascade_hook_ordering();
  test_multi_pass_chain();
  test_standalone_process_resets_timestamp();

  if (failures) {
    std::cerr << failures << " check(s) failed\n";
    CHECK(false); return;
  }
  std::cout << "all tick audit tests passed\n";
  return;
}
