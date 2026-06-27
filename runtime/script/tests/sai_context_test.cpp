// sai_context_test.cpp
// Track-A unit test (NO JavaScript): the ScriptEngine seam + in-process
// SaiContext + the MockScriptEngine backend.
//
// Proves:
//   - field get/set round-trips through reflection,
//   - setField on the OWNING script is always allowed,
//   - the directOutput write-gate: a write to a SECOND node is rejected when
//     directOutput=FALSE and allowed (and lands in the cascade) when TRUE,
//   - addRoute/deleteRoute are gated on directOutput and take effect in the graph,
//   - a permitted setField is delivered as an event in the cascade,
//   - browser-info getters (name/version/currentTime/frameRate/print),
//   - the mock backend records the seam calls and can drive the SAI on invoke.
//
// Exit code 0 on success; nonzero on any failed assertion.

#include "SaiContext.hpp"
#include "ScriptEngine.hpp"
#include "tests/MockScriptEngine.hpp"

#include "X3DExecutionContext.hpp"

#include "x3d/nodes/Script.hpp"
#include "x3d/nodes/Transform.hpp"

#include <any>
#include <iostream>
#include <string>

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

SFVec3f transformTranslation(Transform &t) { return t.getTranslation(); }

// --- field get/set round-trip, on the owning script's own node -------------
void testFieldRoundTrip() {
  X3DExecutionContext ctx;
  Transform target;
  target.setTranslation(SFVec3f{1, 2, 3});

  Script script;
  SaiContext sai(ctx, script, "x3d-cpp-gen", "test");

  // getField reads the live value.
  std::any got = sai.getField(&target, "translation");
  check(got.has_value(), "getField returns a value for a known field");
  check(veq(std::any_cast<SFVec3f>(got), 1, 2, 3),
        "getField reads the current translation");

  // getField on an unknown field / null node is empty, not a crash.
  check(!sai.getField(&target, "nope").has_value(),
        "getField on unknown field is empty");
  check(!sai.getField(nullptr, "translation").has_value(),
        "getField on null node is empty");

  // setField on the owning script itself is always allowed (self-write).
  // Use a Script field route into its own node — write metadata-free: just prove
  // self-write does not throw and posts into the cascade.
  bool threw = false;
  try {
    sai.setField(&script, "directOutput", std::any(SFBool(true)));
  } catch (...) {
    threw = true;
  }
  check(!threw, "setField on the owning script never throws (self-write)");
}

// --- directOutput write-gate to a SECOND node ------------------------------
void testDirectOutputGate() {
  // directOutput FALSE (default): cross-node write is rejected.
  {
    X3DExecutionContext ctx;
    Transform other;
    other.setTranslation(SFVec3f{0, 0, 0});
    Script script;  // directOutput defaults FALSE
    SaiContext sai(ctx, script, "b", "v");

    bool threw = false;
    try {
      sai.setField(&other, "translation", std::any(SFVec3f{9, 9, 9}));
    } catch (const std::logic_error &) {
      threw = true;
    }
    check(threw, "directOutput=FALSE: cross-node setField throws (undefined per spec)");
    check(veq(transformTranslation(other), 0, 0, 0),
          "directOutput=FALSE: rejected write left the other node unchanged");
  }

  // directOutput TRUE: cross-node write is allowed and lands via the cascade.
  {
    X3DExecutionContext ctx;
    Transform other;
    other.setTranslation(SFVec3f{0, 0, 0});
    Script script;
    script.setDirectOutputUnchecked(true);
    SaiContext sai(ctx, script, "b", "v");

    bool threw = false;
    try {
      sai.setField(&other, "translation", std::any(SFVec3f{4, 5, 6}));
    } catch (...) {
      threw = true;
    }
    check(!threw, "directOutput=TRUE: cross-node setField is allowed");
    // setField posts into the current cascade; drain it.
    ctx.process();
    check(veq(transformTranslation(other), 4, 5, 6),
          "directOutput=TRUE: permitted write delivered via the cascade");
  }
}

// --- addRoute / deleteRoute gate + effect ----------------------------------
void testRouteGateAndEffect() {
  // FALSE: add/delete route both throw.
  {
    X3DExecutionContext ctx;
    Script script;  // directOutput FALSE
    Transform a, b;
    SaiContext sai(ctx, script, "b", "v");

    bool addThrew = false, delThrew = false;
    try { sai.addRoute(&a, "translation", &b, "translation"); }
    catch (const std::logic_error &) { addThrew = true; }
    try { sai.deleteRoute(&a, "translation", &b, "translation"); }
    catch (const std::logic_error &) { delThrew = true; }
    check(addThrew, "directOutput=FALSE: addRoute throws");
    check(delThrew, "directOutput=FALSE: deleteRoute throws");
    check(ctx.graph().routeCount() == 0,
          "directOutput=FALSE: no route was added");
  }

  // TRUE: add a route, see it carry an event, then delete it.
  {
    X3DExecutionContext ctx;
    Script script;
    script.setDirectOutputUnchecked(true);
    Transform a, b;
    a.setTranslation(SFVec3f{0, 0, 0});
    b.setTranslation(SFVec3f{0, 0, 0});
    SaiContext sai(ctx, script, "b", "v");

    sai.addRoute(&a, "translation", &b, "translation");
    check(ctx.graph().routeCount() == 1, "directOutput=TRUE: addRoute added a route");

    // Drive the source: posting a's translation should fan out to b.
    ctx.postEvent(&a, "translation", std::any(SFVec3f{7, 8, 9}));
    ctx.process();
    check(veq(transformTranslation(b), 7, 8, 9),
          "added route carried the event from a -> b");

    sai.deleteRoute(&a, "translation", &b, "translation");
    check(ctx.graph().routeCount() == 0, "deleteRoute removed the route");

    // After deletion the route no longer fans out.
    b.setTranslation(SFVec3f{0, 0, 0});
    ctx.postEvent(&a, "translation", std::any(SFVec3f{1, 1, 1}));
    ctx.process();
    check(veq(transformTranslation(b), 0, 0, 0),
          "deleted route no longer carries events");
  }
}

// --- browser info ----------------------------------------------------------
void testBrowserInfo() {
  X3DExecutionContext ctx;
  Script script;
  SaiContext sai(ctx, script, "x3d-cpp-gen", "4.0");

  check(sai.getName() == "x3d-cpp-gen", "getName reports the browser name");
  check(sai.getVersion() == "4.0", "getVersion reports the browser version");

  ctx.tick(2.5);
  check(sai.currentTime() == 2.5, "currentTime follows the context clock");

  check(sai.currentFrameRate() == 0.0, "currentFrameRate defaults to 0");
  sai.setCurrentFrameRate(60.0);
  check(sai.currentFrameRate() == 60.0, "currentFrameRate reflects the set value");

  sai.print("hello ");
  sai.print("world");
  check(sai.log() == "hello world", "print accumulates into the log");
}

// --- the seam through the MockScriptEngine ---------------------------------
void testSeamWithMock() {
  X3DExecutionContext ctx;
  Script script;
  script.setDirectOutputUnchecked(true);
  Transform target;
  target.setTranslation(SFVec3f{0, 0, 0});
  SaiContext sai(ctx, script, "b", "v");

  MockScriptEngine engine;
  ScriptEngine &seam = engine;  // exercise via the abstract interface

  ScriptHandle h = seam.load(script, "ecmascript: /* ignored by mock */", sai);
  check(h != kInvalidScriptHandle, "load returns a valid handle");
  check(engine.saiFor(h) == &sai, "engine retained the SaiContext for the handle");

  seam.initialize(h);

  // Arm the mock: on "set_target", write the value into the Transform via SAI.
  engine.on("set_target", [&target](SaiContext &s, const std::any &v, double) {
    s.setField(&target, "translation", v);
  });

  seam.prepareEvents(h, 1.0);
  seam.invoke(h, "set_target", std::any(SFVec3f{3, 3, 3}),
              X3DFieldType::SFVec3f, 1.0);
  seam.eventsProcessed(h, 1.0);
  seam.shutdown(h);

  // The reaction posted into the cascade; drain it.
  ctx.process();
  check(veq(transformTranslation(target), 3, 3, 3),
        "invoke drove the SAI to write through the cascade");

  // The call log records the lifecycle in order.
  check(engine.count("load") == 1, "one load recorded");
  check(engine.count("initialize") == 1, "one initialize recorded");
  check(engine.count("prepareEvents") == 1, "one prepareEvents recorded");
  check(engine.count("invoke") == 1, "one invoke recorded");
  check(engine.count("eventsProcessed") == 1, "one eventsProcessed recorded");
  check(engine.count("shutdown") == 1, "one shutdown recorded");

  // Ordering: load < initialize < prepareEvents < invoke < eventsProcessed < shutdown.
  const auto &c = engine.calls;
  check(c.size() == 6 && c[0].kind == "load" && c[1].kind == "initialize" &&
            c[2].kind == "prepareEvents" && c[3].kind == "invoke" &&
            c[4].kind == "eventsProcessed" && c[5].kind == "shutdown",
        "seam calls were recorded in lifecycle order");
  check(c[3].eventName == "set_target" && c[3].timestamp == 1.0,
        "invoke recorded the event name and timestamp");
}

} // namespace

int main() {
  testFieldRoundTrip();
  testDirectOutputGate();
  testRouteGateAndEffect();
  testBrowserInfo();
  testSeamWithMock();

  if (failures == 0) {
    std::cout << "ALL SAI CONTEXT TESTS PASSED\n";
    return 0;
  }
  std::cerr << failures << " SAI context test(s) FAILED\n";
  return 1;
}
