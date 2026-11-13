// SaiContext field access, directOutput gates, and route delivery.
#include "doctest/doctest.h"
#include "SaiContext.hpp"
#include "ScriptEngine.hpp"
#include "tests/MockScriptEngine.hpp"

#include "DynamicField.hpp"
#include "X3DExecutionContext.hpp"

#include "x3d/nodes/Script.hpp"
#include "x3d/nodes/Transform.hpp"

#include <any>
#include <iostream>
#include <memory>
#include <string>

using namespace x3d;
using namespace x3d::runtime;
using x3d::runtime::test::MockScriptEngine;
using namespace x3d::core;
using namespace x3d::nodes;

namespace {
bool veq(const SFVec3f &a, float x, float y, float z) {
  return a.x == x && a.y == y && a.z == z;
}

SFVec3f transformTranslation(Transform &t) { return t.getTranslation(); }

// The same node identity must resolve author state through the SAI's context,
// never through another live context's table.
TEST_CASE("author field owner isolation") {
  Script script;
  X3DExecutionContext first;
  X3DExecutionContext second;
  first.authorFields().addAuthorField(
      script, AuthorFieldDecl{"level", X3DFieldType::SFFloat,
                              AccessType::InputOutput, std::any(SFFloat(1))});
  second.authorFields().addAuthorField(
      script, AuthorFieldDecl{"level", X3DFieldType::SFFloat,
                              AccessType::InputOutput, std::any(SFFloat(2))});
  first.authorFields().addAuthorField(
      script, AuthorFieldDecl{"onlyFirst", X3DFieldType::SFBool,
                              AccessType::InputOutput, std::any(SFBool(true))});
  SaiContext firstSai(first, script, "b", "v");
  SaiContext secondSai(second, script, "b", "v");
  CHECK_MESSAGE((&first.authorFields() != &second.authorFields()),
                "default contexts own distinct author-field stores");
  CHECK_MESSAGE((std::any_cast<SFFloat>(firstSai.getField(&script, "level")) == 1 &&
            std::any_cast<SFFloat>(secondSai.getField(&script, "level")) == 2), "SAI reads same-node author values from its own context");
  CHECK_MESSAGE((!secondSai.getField(&script, "onlyFirst").has_value()),
                "SAI cannot discover another context's author declarations");

  firstSai.setField(&script, "level", std::any(SFFloat(3)));
  first.process();
  CHECK_MESSAGE((std::any_cast<SFFloat>(firstSai.getField(&script, "level")) == 3 &&
            std::any_cast<SFFloat>(secondSai.getField(&script, "level")) == 2), "author-field cascade writes stay in the originating context");
  first.authorFields().erase(script);
  CHECK_MESSAGE((!firstSai.getField(&script, "level").has_value() &&
            std::any_cast<SFFloat>(secondSai.getField(&script, "level")) == 2), "erasing one context's author state preserves another context's state");

  auto declaredFields = std::make_shared<DynamicFieldStore>();
  declaredFields->addAuthorField(
      script, AuthorFieldDecl{"level", X3DFieldType::SFFloat,
                              AccessType::InputOutput, std::any(SFFloat(4))});
  X3DExecutionContext explicitOwner(declaredFields);
  SaiContext explicitSai(explicitOwner, script, "b", "v");
  CHECK_MESSAGE((&explicitOwner.authorFields() == declaredFields.get() &&
            std::any_cast<SFFloat>(explicitSai.getField(&script, "level")) == 4), "explicit context owner retains fields declared before construction");
}

// field get/set round-trip, on the owning script's own node
TEST_CASE("field round trip") {
  X3DExecutionContext ctx;
  Transform target;
  target.setTranslation(SFVec3f{1, 2, 3});

  Script script;
  SaiContext sai(ctx, script, "x3d-cpp-gen", "test");

  // getField reads the live value.
  std::any got = sai.getField(&target, "translation");
  CHECK_MESSAGE((got.has_value()), "getField returns a value for a known field");
  CHECK_MESSAGE((veq(std::any_cast<SFVec3f>(got), 1, 2, 3)),
                "getField reads the current translation");

  // getField on an unknown field / null node is empty, not a crash.
  CHECK_MESSAGE((!sai.getField(&target, "nope").has_value()), "getField on unknown field is empty");
  CHECK_MESSAGE((!sai.getField(nullptr, "translation").has_value()),
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
  CHECK_MESSAGE((!threw), "setField on the owning script never throws (self-write)");
}

// directOutput write-gate to a second node
TEST_CASE("direct output gate") {
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
    CHECK_MESSAGE((threw), "directOutput=FALSE: cross-node setField throws (undefined per spec)");
    CHECK_MESSAGE((veq(transformTranslation(other), 0, 0, 0)),
                  "directOutput=FALSE: rejected write left the other node unchanged");
  }

  // directOutput TRUE: cross-node write is allowed and lands via the cascade.
  {
    X3DExecutionContext ctx;
    Transform other;
    other.setTranslation(SFVec3f{0, 0, 0});
    Transform sink;               // a ROUTE target off the written field
    sink.setTranslation(SFVec3f{0, 0, 0});
    Script script;
    script.setDirectOutputUnchecked(true);
    SaiContext sai(ctx, script, "b", "v");

    // A ROUTE leaving the written field (other.translation, inputOutput).
    ctx.addRoute(FieldAddress{&other, "translation"},
                 FieldAddress{&sink, "translation"});

    bool threw = false;
    try {
      sai.setField(&other, "translation", std::any(SFVec3f{4, 5, 6}));
    } catch (...) {
      threw = true;
    }
    CHECK_MESSAGE((!threw), "directOutput=TRUE: cross-node setField is allowed");
    ctx.process();
    CHECK_MESSAGE((veq(transformTranslation(other), 4, 5, 6)),
                  "directOutput=TRUE: permitted write mutated the target field");
    // DO-CASCADE (SAI §4.5.2 / ADR-0029): the direct write is a value mutation,
    // not a cascade seed — no ROUTE fans out from the written field.
    CHECK_MESSAGE((veq(transformTranslation(sink), 0, 0, 0)),
                  "DO-CASCADE: no ROUTE fans out from a directOutput cross-node write");
  }

  // A self-write to the Script's own field keeps the routable path (§29.2.4):
  // it must still fan out along a ROUTE leaving that field.
  {
    X3DExecutionContext ctx;
    Script script;
    script.setDirectOutputUnchecked(true);
    Transform sink;
    sink.setTranslation(SFVec3f{0, 0, 0});
    // An outputOnly author field on the Script, ROUTEd to the sink.
    ctx.authorFields().addAuthorField(
        script, AuthorFieldDecl{"out", X3DFieldType::SFVec3f,
                                AccessType::OutputOnly, {}});
    ctx.addRoute(FieldAddress{&script, "out"},
                 FieldAddress{&sink, "translation"});
    SaiContext sai(ctx, script, "b", "v");

    sai.setField(&script, "out", std::any(SFVec3f{7, 8, 9}));
    ctx.process();
    CHECK_MESSAGE((veq(transformTranslation(sink), 7, 8, 9)),
                  "DO-CASCADE: a self-write keeps the routable path (fans out)");
    ctx.authorFields().erase(script);
  }
}

// addRoute / deleteRoute gate + effect
TEST_CASE("route gate and effect") {
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
    CHECK_MESSAGE((addThrew), "directOutput=FALSE: addRoute throws");
    CHECK_MESSAGE((delThrew), "directOutput=FALSE: deleteRoute throws");
    CHECK_MESSAGE((ctx.graph().routeCount() == 0), "directOutput=FALSE: no route was added");
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
    CHECK_MESSAGE((ctx.graph().routeCount() == 1), "directOutput=TRUE: addRoute added a route");

    // Drive the source: posting a's translation should fan out to b.
    ctx.postEvent(&a, "translation", std::any(SFVec3f{7, 8, 9}));
    ctx.process();
    CHECK_MESSAGE((veq(transformTranslation(b), 7, 8, 9)),
                  "added route carried the event from a -> b");

    sai.deleteRoute(&a, "translation", &b, "translation");
    CHECK_MESSAGE((ctx.graph().routeCount() == 0), "deleteRoute removed the route");

    // Deleted routes do not fan out.
    b.setTranslation(SFVec3f{0, 0, 0});
    ctx.postEvent(&a, "translation", std::any(SFVec3f{1, 1, 1}));
    ctx.process();
    CHECK_MESSAGE((veq(transformTranslation(b), 0, 0, 0)),
                  "deleted route no longer carries events");
  }
}

// browser info
TEST_CASE("browser info") {
  X3DExecutionContext ctx;
  Script script;
  SaiContext sai(ctx, script, "x3d-cpp-gen", "4.0");

  CHECK_MESSAGE((sai.getName() == "x3d-cpp-gen"), "getName reports the browser name");
  CHECK_MESSAGE((sai.getVersion() == "4.0"), "getVersion reports the browser version");

  ctx.tick(2.5);
  CHECK_MESSAGE((sai.currentTime() == 2.5), "currentTime follows the context clock");

  CHECK_MESSAGE((sai.currentFrameRate() == 0.0), "currentFrameRate defaults to 0");
  sai.setCurrentFrameRate(60.0);
  CHECK_MESSAGE((sai.currentFrameRate() == 60.0), "currentFrameRate reflects the set value");

  sai.print("hello ");
  sai.print("world");
  CHECK_MESSAGE((sai.log() == "hello world"), "print accumulates into the log");
}

// the seam through the MockScriptEngine
TEST_CASE("seam with mock") {
  X3DExecutionContext ctx;
  Script script;
  script.setDirectOutputUnchecked(true);
  Transform target;
  target.setTranslation(SFVec3f{0, 0, 0});
  SaiContext sai(ctx, script, "b", "v");

  MockScriptEngine engine;
  ScriptEngine &seam = engine;  // exercise via the abstract interface

  ScriptHandle h = seam.load(script, "ecmascript: /* ignored by mock */", sai);
  CHECK_MESSAGE((h != kInvalidScriptHandle), "load returns a valid handle");
  CHECK_MESSAGE((engine.saiFor(h) == &sai), "engine retained the SaiContext for the handle");

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
  CHECK_MESSAGE((veq(transformTranslation(target), 3, 3, 3)),
                "invoke drove the SAI to write through the cascade");

  // The call log records the lifecycle in order.
  CHECK_MESSAGE((engine.count("load") == 1), "one load recorded");
  CHECK_MESSAGE((engine.count("initialize") == 1), "one initialize recorded");
  CHECK_MESSAGE((engine.count("prepareEvents") == 1), "one prepareEvents recorded");
  CHECK_MESSAGE((engine.count("invoke") == 1), "one invoke recorded");
  CHECK_MESSAGE((engine.count("eventsProcessed") == 1), "one eventsProcessed recorded");
  CHECK_MESSAGE((engine.count("shutdown") == 1), "one shutdown recorded");

  // Ordering: load < initialize < prepareEvents < invoke < eventsProcessed < shutdown.
  const auto &c = engine.calls;
  CHECK_MESSAGE((c.size() == 6 && c[0].kind == "load" && c[1].kind == "initialize" &&
            c[2].kind == "prepareEvents" && c[3].kind == "invoke" &&
            c[4].kind == "eventsProcessed" && c[5].kind == "shutdown"), "seam calls were recorded in lifecycle order");
  CHECK_MESSAGE((c[3].eventName == "set_target" && c[3].timestamp == 1.0),
                "invoke recorded the event name and timestamp");
}

} // namespace
