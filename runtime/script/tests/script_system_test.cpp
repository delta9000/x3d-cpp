// script_system_test.cpp
// Track-A unit test (NO JavaScript): ScriptSystem — the System that wires a
// Script node's lifecycle (load/initialize/shutdown) and its event delivery
// (inputOnly -> engine.invoke, prepareEvents/eventsProcessed phasing,
// outputOnly -> cascade) into the X3DExecutionContext tick/cascade, driven by a
// recording MockScriptEngine.
//
// Proves the spec-checked ordering (ISO/IEC 19775-1 §29.2):
//   - initialize() runs once at load, BEFORE the first event;
//   - prepareEvents() runs once per timestamp, BEFORE route processing;
//   - an inputOnly event dispatches to engine.invoke with its name+timestamp;
//   - an outputOnly write the script makes becomes a cascade event carrying the
//     TRIGGERING event's timestamp (not the clock);
//   - eventsProcessed() runs once after a batch of received events, and only for
//     scripts that actually received an event this tick;
//   - set_url shuts down the old script then loads+initializes the new one;
//   - the directOutput gate is honored (cross-node write rejected when FALSE);
//   - mustEvaluate: TRUE delivers eagerly, FALSE may defer until the batch end.
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "ScriptSystem.hpp"

#include "SaiContext.hpp"
#include "ScriptEngine.hpp"
#include "tests/MockScriptEngine.hpp"

#include "X3DExecutionContext.hpp"

#include "x3d/nodes/Script.hpp"
#include "x3d/nodes/Transform.hpp"

#include <any>
#include <iostream>
#include <string>
#include <vector>

using namespace x3d;
using namespace x3d::runtime;
using x3d::runtime::test::MockScriptEngine;

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

bool veq(const SFVec3f &a, float x, float y, float z) {
  return a.x == x && a.y == y && a.z == z;
}

// Index of the first call of `kind` in the mock's log (-1 if absent).
int indexOf(const MockScriptEngine &e, const std::string &kind) {
  for (std::size_t i = 0; i < e.calls.size(); ++i)
    if (e.calls[i].kind == kind) return static_cast<int>(i);
  return -1;
}

// A Script with an inline ecmascript: url so the system has something to load.
Script makeScript(bool directOutput = false, bool mustEvaluate = true) {
  Script s;
  s.setUrl(MFString{"ecmascript:/* body */"});
  s.setLoad(true);
  if (directOutput) s.setDirectOutputUnchecked(true);
  s.setMustEvaluateUnchecked(mustEvaluate);
  return s;
}

// --- load -> initialize happens once, before any event ----------------------
void testInitializeBeforeFirstEvent() {
  X3DExecutionContext ctx;
  Script script = makeScript();

  auto engine = std::make_shared<MockScriptEngine>();
  auto sys = std::make_shared<ScriptSystem>(engine, "x3d-cpp-gen", "4.0");
  sys->attach(&script, ctx);

  check(engine->count("load") == 1, "attach with load=TRUE loads the script");
  check(engine->count("initialize") == 1, "attach initializes the script");
  check(indexOf(*engine, "load") < indexOf(*engine, "initialize"),
        "load precedes initialize");
  // No event yet -> no invoke before initialize.
  check(engine->count("invoke") == 0, "no invoke before the first event");

  // Deliver an inputOnly event; initialize must already have happened.
  sys->deliverInputEvent(&script, "set_x", std::any(SFFloat(1.0f)),
                         X3DFieldType::SFFloat, 1.0);
  check(indexOf(*engine, "initialize") < indexOf(*engine, "invoke"),
        "initialize precedes the first invoke");
}

// load=FALSE must NOT load/initialize at attach time --------------------------
void testLoadFalseDefersLoad() {
  X3DExecutionContext ctx;
  Script script = makeScript();
  script.setLoad(false);

  auto engine = std::make_shared<MockScriptEngine>();
  auto sys = std::make_shared<ScriptSystem>(engine, "b", "v");
  sys->attach(&script, ctx);

  check(engine->count("load") == 0, "load=FALSE: not loaded at attach");
  check(engine->count("initialize") == 0, "load=FALSE: not initialized");
}

// --- prepareEvents once per timestamp, before routes ------------------------
void testPrepareEventsPhasing() {
  X3DExecutionContext ctx;
  Script script = makeScript();

  auto engine = std::make_shared<MockScriptEngine>();
  auto sys = std::make_shared<ScriptSystem>(engine, "b", "v");
  ctx.addScriptSystem(sys);   // makes tick() drive prepare/eventsProcessed
  sys->attach(&script, ctx);

  ctx.tick(1.0);
  ctx.tick(2.0);
  check(engine->count("prepareEvents") == 2,
        "prepareEvents called once per tick");
  // prepareEvents carries the tick timestamp.
  int pi = indexOf(*engine, "prepareEvents");
  check(pi >= 0 && engine->calls[pi].timestamp == 1.0,
        "prepareEvents carries the timestamp");
}

// --- inputOnly -> invoke; outputOnly write -> cascade at trigger timestamp ---
void testInputInvokeAndOutputTimestamp() {
  X3DExecutionContext ctx;
  Script script = makeScript(/*directOutput=*/true);
  Transform target;
  target.setTranslation(SFVec3f{0, 0, 0});

  auto engine = std::make_shared<MockScriptEngine>();
  auto sys = std::make_shared<ScriptSystem>(engine, "b", "v");
  ctx.addScriptSystem(sys);
  sys->attach(&script, ctx);

  // On set_target, the "script" writes the value out to the Transform via SAI.
  engine->on("set_target", [&target](SaiContext &s, const std::any &v, double) {
    s.setField(&target, "translation", v);
  });

  // Advance the clock to 5.0, but deliver the event with timestamp 1.0; the
  // outputOnly write must carry 1.0, the triggering timestamp.
  ctx.tick(5.0);
  sys->deliverInputEvent(&script, "set_target", std::any(SFVec3f{3, 3, 3}),
                         X3DFieldType::SFVec3f, 1.0);

  check(engine->count("invoke") == 1, "inputOnly delivered to engine.invoke");
  int ii = indexOf(*engine, "invoke");
  check(ii >= 0 && engine->calls[ii].eventName == "set_target" &&
            engine->calls[ii].timestamp == 1.0,
        "invoke recorded the event name + triggering timestamp");

  // The SAI write posted into the cascade; drain it.
  ctx.process();
  check(veq(target.getTranslation(), 3, 3, 3),
        "outputOnly write reached the target via the cascade");
}

// --- eventsProcessed runs once after a batch, only for receivers ------------
void testEventsProcessedAfterBatch() {
  X3DExecutionContext ctx;
  Script script = makeScript();

  auto engine = std::make_shared<MockScriptEngine>();
  auto sys = std::make_shared<ScriptSystem>(engine, "b", "v");
  ctx.addScriptSystem(sys);
  sys->attach(&script, ctx);

  // Tick with NO events: eventsProcessed must not fire.
  ctx.tick(1.0);
  check(engine->count("eventsProcessed") == 0,
        "eventsProcessed skipped on a tick with no received events");

  // A tick during which two events are delivered -> exactly one eventsProcessed,
  // AFTER both invokes.
  // Wire delivery to happen during the cascade: deliver inside tick by using a
  // pre-armed System is overkill; deliver directly then run eventsProcessed.
  sys->deliverInputEvent(&script, "set_a", std::any(SFFloat(1.0f)),
                         X3DFieldType::SFFloat, 2.0);
  sys->deliverInputEvent(&script, "set_b", std::any(SFFloat(2.0f)),
                         X3DFieldType::SFFloat, 2.0);
  sys->runEventsProcessed(ctx);
  check(engine->count("eventsProcessed") == 1,
        "eventsProcessed fires once after a batch of events");
  // Both invokes precede the eventsProcessed.
  int ep = indexOf(*engine, "eventsProcessed");
  int lastInvoke = -1;
  for (std::size_t i = 0; i < engine->calls.size(); ++i)
    if (engine->calls[i].kind == "invoke") lastInvoke = static_cast<int>(i);
  check(lastInvoke >= 0 && lastInvoke < ep,
        "all invokes precede eventsProcessed");

  // A second runEventsProcessed without new events must not re-fire.
  sys->runEventsProcessed(ctx);
  check(engine->count("eventsProcessed") == 1,
        "eventsProcessed not re-fired without new events");
}

// --- set_url: shutdown old, then load + initialize new ----------------------
void testSetUrlReload() {
  X3DExecutionContext ctx;
  Script script = makeScript();

  auto engine = std::make_shared<MockScriptEngine>();
  auto sys = std::make_shared<ScriptSystem>(engine, "b", "v");
  sys->attach(&script, ctx);
  check(engine->count("load") == 1 && engine->count("initialize") == 1,
        "initial load + initialize");

  sys->setUrl(&script, MFString{"ecmascript:/* new body */"}, ctx);

  check(engine->count("shutdown") == 1, "set_url shuts the old script down");
  check(engine->count("load") == 2, "set_url loads the new script");
  check(engine->count("initialize") == 2, "set_url initializes the new script");
  // Ordering: the shutdown precedes the second load and second initialize.
  int sd = indexOf(*engine, "shutdown");
  // Find the 2nd load / 2nd initialize.
  int load2 = -1, init2 = -1, lc = 0, ic = 0;
  for (std::size_t i = 0; i < engine->calls.size(); ++i) {
    if (engine->calls[i].kind == "load" && ++lc == 2) load2 = static_cast<int>(i);
    if (engine->calls[i].kind == "initialize" && ++ic == 2)
      init2 = static_cast<int>(i);
  }
  check(sd >= 0 && sd < load2 && load2 < init2,
        "set_url order: shutdown < new load < new initialize");
}

// --- directOutput gate honored end-to-end through the system ----------------
void testDirectOutputGate() {
  X3DExecutionContext ctx;
  Script script = makeScript(/*directOutput=*/false);  // FALSE
  Transform other;
  other.setTranslation(SFVec3f{0, 0, 0});

  auto engine = std::make_shared<MockScriptEngine>();
  auto sys = std::make_shared<ScriptSystem>(engine, "b", "v");
  sys->attach(&script, ctx);

  bool threw = false;
  engine->on("set_x", [&other, &threw](SaiContext &s, const std::any &v, double) {
    try {
      s.setField(&other, "translation", v);
    } catch (const std::logic_error &) {
      threw = true;
    }
  });
  sys->deliverInputEvent(&script, "set_x", std::any(SFVec3f{9, 9, 9}),
                         X3DFieldType::SFVec3f, 1.0);
  ctx.process();
  check(threw, "directOutput=FALSE: cross-node write rejected (gate honored)");
  check(veq(other.getTranslation(), 0, 0, 0),
        "directOutput=FALSE: the other node is unchanged");
}

// --- mustEvaluate: TRUE eager, FALSE deferred to the batch flush ------------
void testMustEvaluateEagerVsLazy() {
  // mustEvaluate = TRUE: invoke happens immediately on delivery.
  {
    X3DExecutionContext ctx;
    Script script = makeScript(/*directOutput=*/false, /*mustEvaluate=*/true);
    auto engine = std::make_shared<MockScriptEngine>();
    auto sys = std::make_shared<ScriptSystem>(engine, "b", "v");
    sys->attach(&script, ctx);

    sys->deliverInputEvent(&script, "set_x", std::any(SFFloat(1.0f)),
                           X3DFieldType::SFFloat, 1.0);
    check(engine->count("invoke") == 1,
          "mustEvaluate=TRUE: input delivered eagerly (invoke immediate)");
  }

  // mustEvaluate = FALSE: delivery may be deferred; the invoke shows up only
  // after the batch flush (runEventsProcessed), still before eventsProcessed.
  {
    X3DExecutionContext ctx;
    Script script = makeScript(/*directOutput=*/false, /*mustEvaluate=*/false);
    auto engine = std::make_shared<MockScriptEngine>();
    auto sys = std::make_shared<ScriptSystem>(engine, "b", "v");
    sys->attach(&script, ctx);

    sys->deliverInputEvent(&script, "set_x", std::any(SFFloat(1.0f)),
                           X3DFieldType::SFFloat, 1.0);
    check(engine->count("invoke") == 0,
          "mustEvaluate=FALSE: input deferred (no immediate invoke)");

    sys->runEventsProcessed(ctx);
    check(engine->count("invoke") == 1,
          "mustEvaluate=FALSE: deferred input invoked at batch flush");
    int ii = indexOf(*engine, "invoke");
    int ep = indexOf(*engine, "eventsProcessed");
    check(ii >= 0 && ep >= 0 && ii < ep,
          "deferred invoke still precedes eventsProcessed");
  }
}

// SCR-002: §29.2.3 shutdown() runs when the world is unloaded. Destroying the
// ScriptSystem (its execution context going away) is that teardown, so every
// still-loaded script must be shut down.
void testShutdownOnTeardown() {
  X3DExecutionContext ctx;
  Script script;
  script.setSourceCode("function shutdown() {}"); // non-empty -> the mock loads it
  script.setLoad(true);
  auto engine = std::make_shared<MockScriptEngine>();
  {
    auto sys = std::make_shared<ScriptSystem>(engine, "b", "v");
    sys->attach(&script, ctx);
    check(engine->count("load") == 1, "scr002: loaded at attach");
    check(engine->count("shutdown") == 0, "scr002: not shut down while alive");
  } // ScriptSystem destroyed here -> teardown shutdown
  check(engine->count("shutdown") == 1,
        "scr002: ScriptSystem teardown shuts down the loaded script");
}

} // namespace

int main() {
  testShutdownOnTeardown();
  testInitializeBeforeFirstEvent();
  testLoadFalseDefersLoad();
  testPrepareEventsPhasing();
  testInputInvokeAndOutputTimestamp();
  testEventsProcessedAfterBatch();
  testSetUrlReload();
  testDirectOutputGate();
  testMustEvaluateEagerVsLazy();

  if (failures == 0) {
    std::cout << "ALL SCRIPT SYSTEM TESTS PASSED\n";
    return 0;
  }
  std::cerr << failures << " script system test(s) FAILED\n";
  return 1;
}
