#include "doctest/doctest.h"
#include "X3DDocument.hpp"
#include "X3DSceneBridge.hpp"
#include "x3d/nodes/Switch.hpp"
#include "x3d/nodes/TimeSensor.hpp"

#include <any>
#include <initializer_list>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;

namespace {

// Register the ordinary scene-owned systems, preserving their callback guards.
// No test replaces a utility input handler or reaches its private index state.
struct Utilities {
  Scene scene;
  X3DExecutionContext ctx;

  template <class Node> std::shared_ptr<Node> add() {
    auto node = std::make_shared<Node>();
    scene.addRootNode(node);
    return node;
  }
  void attach() {
    ctx.buildSceneGraph(scene);
    attachEventUtilities(scene, ctx);
  }
};

template <class Value> Value read(const X3DNode &node, const std::string &field) {
  for (const auto &info : node.fields())
    if (info.x3dName == field && info.get)
      return std::any_cast<Value>(info.get(node));
  throw std::logic_error("missing readable test field: " + field);
}

// Real generated inputOutput destinations exercise set_ alias normalization.
// These sinks receive no TimeSensor/other behavior-system attachment.
template <class Value> struct Sink;
template <> struct Sink<SFBool> { TimeSensor node; const char *field = "enabled"; };
template <> struct Sink<SFInt32> { Switch node; const char *field = "whichChoice"; };
template <> struct Sink<SFTime> { TimeSensor node; const char *field = "startTime"; };

template <class Value> struct OutputTrace {
  X3DNode &source;
  std::string field;
  Sink<Value> sink;
  Value initialSource, initialSink;
  std::vector<Value> observed, routed;

  OutputTrace(X3DExecutionContext &ctx, X3DNode &node, const char *name)
      : source(node), field(name), initialSource(read<Value>(node, name)),
        initialSink(read<Value>(sink.node, sink.field)) {
    ctx.addRoute({&source, field}, {&sink.node, std::string("set_") + sink.field});
    ctx.addFieldWriteListener([this](const FieldAddress &address) {
      if (address.node == &source && address.field == field)
        observed.push_back(read<Value>(source, field));
      if (address.node == &sink.node && address.field == sink.field)
        routed.push_back(read<Value>(sink.node, sink.field));
    });
  }

  void expect(std::initializer_list<Value> values) const {
    INFO(source.nodeTypeName() << '.' << field);
    const std::vector<Value> expected(values);
    CHECK(observed == expected);
    CHECK(routed == expected);
    CHECK(read<Value>(source, field) == (expected.empty() ? initialSource : expected.back()));
    CHECK(read<Value>(sink.node, sink.field) == (expected.empty() ? initialSink : expected.back()));
  }
};

struct InputTrace {
  std::map<std::string, std::size_t> counts;
  InputTrace(X3DExecutionContext &ctx, X3DNode &node) {
    ctx.addFieldWriteListener([this, &node](const FieldAddress &address) {
      if (address.node == &node) ++counts[address.field];
    });
  }
  std::size_t count(const char *field) const {
    const auto found = counts.find(field);
    return found == counts.end() ? 0 : found->second;
  }
};

void initialize(BooleanSequencer &node) {
  node.setKey({0.f, .5f, 1.f});
  node.setKeyValue({false, false, true});
}
void initialize(IntegerSequencer &node) {
  node.setKey({0.f, .5f, 1.f});
  node.setKeyValue({10, 20, 30});
}

template <class Node, class Value>
void sequencerFractions(Value first, Value middle, Value last) {
  for (bool identical : {false, true}) {
    INFO("identical fractions: " << identical);
    Utilities fixture;
    auto node = fixture.add<Node>();
    initialize(*node);
    fixture.attach();
    auto &ctx = fixture.ctx;
    InputTrace inputs(ctx, *node);
    OutputTrace<Value> output(ctx, *node, "value_changed");
    output.expect({});
    ctx.postEvent(node.get(), "set_fraction", SFFloat{0.f});
    ctx.postEvent(node.get(), "set_fraction", SFFloat{0.f});
    ctx.postEvent(node.get(), "set_fraction", SFFloat{identical ? 0.f : .5f});
    ctx.tick(10);
    CHECK(inputs.count("set_fraction") == 3);
    output.expect({first});

    // Even a suppressed fraction moves the private index. With the distinct
    // inputs it is now 1, so next must reach index 2, not index 1.
    ctx.postEvent(node.get(), "next", SFBool{true});
    ctx.tick(11);
    CHECK(inputs.count("next") == 1);
    output.expect({first, identical ? middle : last});
    // Same-interval fractions across timestamps: see the §30.2.4 interval cases.
  }
}

template <class Node, class Value>
void sequencerSteps(Value first, Value middle, Value last) {
  Utilities fixture;
  auto node = fixture.add<Node>();
  initialize(*node);
  fixture.attach();
  auto &ctx = fixture.ctx;
  InputTrace inputs(ctx, *node);
  OutputTrace<Value> output(ctx, *node, "value_changed");
  ctx.postEvent(node.get(), "set_fraction", SFFloat{0.f});
  ctx.tick(10);
  output.expect({first});

  ctx.postEvent(node.get(), "next", SFBool{false});
  ctx.postEvent(node.get(), "previous", SFBool{false});
  ctx.tick(11);
  CHECK(inputs.count("next") == 1);
  CHECK(inputs.count("previous") == 1);
  output.expect({first});

  ctx.postEvent(node.get(), "next", SFBool{false});
  ctx.postEvent(node.get(), "previous", SFBool{false});
  ctx.postEvent(node.get(), "next", SFBool{true});
  ctx.postEvent(node.get(), "next", SFBool{true});
  ctx.tick(12);
  CHECK(inputs.count("next") == 4);
  CHECK(inputs.count("previous") == 2);
  output.expect({first, middle});
  // Both TRUE arrivals advanced: the next step wraps index 2 to index 0.
  ctx.postEvent(node.get(), "next", SFBool{true});
  ctx.tick(13);
  output.expect({first, middle, first});

  ctx.postEvent(node.get(), "previous", SFBool{false});
  ctx.postEvent(node.get(), "previous", SFBool{true});
  ctx.postEvent(node.get(), "previous", SFBool{true});
  ctx.tick(14);
  CHECK(inputs.count("previous") == 5);
  output.expect({first, middle, first, last});
  // The admitted previous wrapped 0 to 2; the suppressed one still reached 1.
  ctx.postEvent(node.get(), "previous", SFBool{true});
  ctx.tick(15);
  CHECK(inputs.count("next") == 5);
  CHECK(inputs.count("previous") == 6);
  output.expect({first, middle, first, last, first});
}

template <class Node, class Value>
void sequencerRepair(Value first, Value last) {
  for (bool emptyValues : {false, true}) {
    INFO("empty keyValue: " << emptyValues);
    Utilities fixture;
    auto node = fixture.add<Node>();
    initialize(*node);
    if (emptyValues) node->setKeyValue({});
    else node->setKey({});
    fixture.attach();
    auto &ctx = fixture.ctx;
    InputTrace inputs(ctx, *node);
    OutputTrace<Value> output(ctx, *node, "value_changed");
    bool repaired = false;
    ctx.addPostCascadeHook([&](X3DExecutionContext &context) {
      if (repaired) return;
      repaired = true;
      CHECK(inputs.count("set_fraction") == 2);
      CHECK(inputs.count("next") == (emptyValues ? 1 : 0));
      CHECK(inputs.count("previous") == (emptyValues ? 1 : 0));
      output.expect({});
      initialize(*node);
      context.postEvent(node.get(), "set_fraction", SFFloat{0.f});
      context.postEvent(node.get(), "set_fraction", SFFloat{.5f});
      context.process(); // joins this tick; rejected work used no output slot
    });
    ctx.postEvent(node.get(), "set_fraction", SFFloat{0.f});
    ctx.postEvent(node.get(), "set_fraction", SFFloat{.5f});
    if (emptyValues) {
      ctx.postEvent(node.get(), "next", SFBool{true});
      ctx.postEvent(node.get(), "previous", SFBool{true});
    }
    ctx.tick(10);
    CHECK(repaired);
    CHECK(inputs.count("set_fraction") == 4);
    output.expect({first});
    ctx.postEvent(node.get(), "next", SFBool{true});
    ctx.tick(11);
    CHECK(inputs.count("next") == (emptyValues ? 2 : 1));
    output.expect({first, last});
  }
}

} // namespace

TEST_SUITE("event utility output admission") {

TEST_CASE("BooleanTrigger admits repeated equal and distinct time inputs once") {
  for (bool identical : {false, true}) {
    INFO("identical times: " << identical);
    Utilities fixture;
    auto node = fixture.add<BooleanTrigger>();
    fixture.attach();
    auto &ctx = fixture.ctx;
    InputTrace inputs(ctx, *node);
    OutputTrace<SFBool> output(ctx, *node, "triggerTrue");
    ctx.tick(9);
    output.expect({});
    for (auto time : {SFTime{2}, SFTime{2}, SFTime{identical ? 2.0 : 8.0}})
      ctx.postEvent(node.get(), "set_triggerTime", time);
    ctx.tick(10);
    CHECK(inputs.count("set_triggerTime") == 3);
    output.expect({true});
    ctx.postEvent(node.get(), "set_triggerTime", SFTime{12});
    ctx.tick(11);
    CHECK(inputs.count("set_triggerTime") == 4);
    output.expect({true, true});
  }
}

TEST_CASE("IntegerTrigger FALSE is silent and leaves TRUE output admission available") {
  Utilities fixture;
  auto node = fixture.add<IntegerTrigger>();
  node->setIntegerKey(7);
  node->emitTriggerValue(-99);
  fixture.attach();
  auto &ctx = fixture.ctx;
  InputTrace inputs(ctx, *node);
  OutputTrace<SFInt32> output(ctx, *node, "triggerValue");
  ctx.postEvent(node.get(), "set_boolean", SFBool{false});
  ctx.tick(10);
  CHECK(inputs.count("set_boolean") == 1);
  output.expect({});
  for (bool value : {false, true, true})
    ctx.postEvent(node.get(), "set_boolean", SFBool{value});
  ctx.tick(11);
  CHECK(inputs.count("set_boolean") == 4);
  output.expect({7});
  ctx.postEvent(node.get(), "set_boolean", SFBool{true});
  ctx.tick(12);
  CHECK(inputs.count("set_boolean") == 5);
  output.expect({7, 7});
}

TEST_CASE("IntegerTrigger key writes preserve every canonical and aliased occurrence") {
  Utilities fixture;
  auto node = fixture.add<IntegerTrigger>();
  fixture.attach();
  auto &ctx = fixture.ctx;
  OutputTrace<SFInt32> output(ctx, *node, "triggerValue");
  std::vector<SFInt32> keys;
  ctx.addFieldWriteListener([&](const FieldAddress &address) {
    if (address.node == node.get() && address.field == "integerKey")
      keys.push_back(node->getIntegerKey());
  });
  ctx.postEvent(node.get(), "integerKey", SFInt32{7});
  ctx.postEvent(node.get(), "set_integerKey", SFInt32{7});
  ctx.postEvent(node.get(), "integerKey", SFInt32{9});
  ctx.tick(10);
  CHECK(keys == std::vector<SFInt32>{7, 7, 9});
  CHECK(node->getIntegerKey() == 9);
  output.expect({7});
  ctx.postEvent(node.get(), "set_integerKey", SFInt32{11});
  ctx.tick(11);
  CHECK(keys == std::vector<SFInt32>{7, 7, 9, 11});
  CHECK(node->getIntegerKey() == 11);
  output.expect({7, 11});
}

TEST_CASE("IntegerTrigger TRUE and key-write producers share one admission slot") {
  for (bool keyFirst : {false, true}) {
    INFO("key write first: " << keyFirst);
    Utilities fixture;
    auto node = fixture.add<IntegerTrigger>();
    node->setIntegerKey(7);
    fixture.attach();
    auto &ctx = fixture.ctx;
    InputTrace inputs(ctx, *node);
    OutputTrace<SFInt32> output(ctx, *node, "triggerValue");
    if (keyFirst) ctx.postEvent(node.get(), "set_integerKey", SFInt32{9});
    ctx.postEvent(node.get(), "set_boolean", SFBool{true});
    if (!keyFirst) ctx.postEvent(node.get(), "set_integerKey", SFInt32{9});
    ctx.tick(10);
    CHECK(inputs.count("integerKey") == 1);
    CHECK(inputs.count("set_boolean") == 1);
    CHECK(node->getIntegerKey() == 9);
    // First-admitted is this implementation's policy. Cardinality and exact
    // source/observer/ROUTE readback agreement are the portable requirements.
    output.expect({keyFirst ? 9 : 7});
    ctx.postEvent(node.get(), "set_boolean", SFBool{true});
    ctx.tick(11);
    output.expect({keyFirst ? 9 : 7, 9});
  }
}

TEST_CASE("TimeTrigger accepts FALSE and uses current tick time for repeated inputs") {
  for (bool identical : {false, true}) {
    INFO("identical booleans: " << identical);
    Utilities fixture;
    auto node = fixture.add<TimeTrigger>();
    fixture.attach();
    auto &ctx = fixture.ctx;
    InputTrace inputs(ctx, *node);
    OutputTrace<SFTime> output(ctx, *node, "triggerTime");
    for (bool value : {false, false, !identical})
      ctx.postEvent(node.get(), "set_boolean", SFBool{value});
    ctx.tick(12.25);
    CHECK(inputs.count("set_boolean") == 3);
    output.expect({12.25});
    ctx.postEvent(node.get(), "set_boolean", SFBool{false});
    // Current logical-tick compatibility, not ISO timestamp conformance:
    // the host may repeat its numeric clock while opening a fresh runtime
    // tick, so the equal time value still receives new output admission.
    ctx.tick(12.25);
    CHECK(inputs.count("set_boolean") == 4);
    output.expect({12.25, 12.25});
    ctx.postEvent(node.get(), "set_boolean", SFBool{false});
    ctx.tick(13.75);
    CHECK(inputs.count("set_boolean") == 5);
    output.expect({12.25, 12.25, 13.75});
  }
}

TEST_CASE("BooleanFilter admits its three fields independently for mixed repetitions") {
  for (bool identical : {false, true}) {
    INFO("identical booleans: " << identical);
    Utilities fixture;
    auto node = fixture.add<BooleanFilter>();
    node->emitInputFalse(true);
    node->emitInputNegate(true);
    fixture.attach();
    auto &ctx = fixture.ctx;
    InputTrace inputs(ctx, *node);
    OutputTrace<SFBool> yes(ctx, *node, "inputTrue");
    OutputTrace<SFBool> no(ctx, *node, "inputFalse");
    OutputTrace<SFBool> negate(ctx, *node, "inputNegate");
    for (bool value : {true, true, identical})
      ctx.postEvent(node.get(), "set_boolean", SFBool{value});
    ctx.tick(10);
    CHECK(inputs.count("set_boolean") == 3);
    yes.expect({true});
    if (identical) no.expect({}); else no.expect({false});
    negate.expect({false});
    ctx.postEvent(node.get(), "set_boolean", SFBool{false});
    ctx.tick(11);
    CHECK(inputs.count("set_boolean") == 4);
    yes.expect({true});
    if (identical) no.expect({false}); else no.expect({false, false});
    negate.expect({false, true});
  }
}

TEST_CASE("BooleanFilter paired output admission survives a nested drain") {
  Utilities fixture;
  auto node = fixture.add<BooleanFilter>();
  fixture.attach();
  auto &ctx = fixture.ctx;
  InputTrace inputs(ctx, *node);
  OutputTrace<SFBool> yes(ctx, *node, "inputTrue");
  OutputTrace<SFBool> no(ctx, *node, "inputFalse");
  OutputTrace<SFBool> negate(ctx, *node, "inputNegate");
  bool nested = false;
  ctx.addFieldWriteListener([&](const FieldAddress &address) {
    if (address.node == node.get() && address.field == "inputTrue" && !nested) {
      nested = true;
      ctx.postEvent(node.get(), "set_boolean", SFBool{false});
      ctx.process();
    }
  });
  ctx.postEvent(node.get(), "set_boolean", SFBool{true});
  ctx.tick(10);
  CHECK(nested);
  CHECK(inputs.count("set_boolean") == 2);
  yes.expect({true});
  no.expect({false});
  negate.expect({false});
  ctx.postEvent(node.get(), "set_boolean", SFBool{false});
  ctx.tick(11);
  CHECK(inputs.count("set_boolean") == 3);
  yes.expect({true});
  no.expect({false, false});
  negate.expect({false, true});
}

TEST_CASE("sequencer suppressed fractions still advance the private index") {
  SUBCASE("BooleanSequencer") { sequencerFractions<BooleanSequencer>(false, false, true); }
  SUBCASE("IntegerSequencer") { sequencerFractions<IntegerSequencer>(10, 20, 30); }
}

TEST_CASE("sequencer repeated steps preserve FALSE no-ops advancement and wraparound") {
  SUBCASE("BooleanSequencer") { sequencerSteps<BooleanSequencer>(false, false, true); }
  SUBCASE("IntegerSequencer") { sequencerSteps<IntegerSequencer>(10, 20, 30); }
}

TEST_CASE("sequencer empty data leaves same-cascade admission available after repair") {
  SUBCASE("BooleanSequencer") { sequencerRepair<BooleanSequencer>(false, true); }
  SUBCASE("IntegerSequencer") { sequencerRepair<IntegerSequencer>(10, 30); }
}

TEST_CASE("BooleanFilter fan-in and loop preserve inputs while capping outputs and routes") {
  Utilities fixture;
  auto a = fixture.add<BooleanFilter>();
  auto b = fixture.add<BooleanFilter>();
  auto c = fixture.add<BooleanFilter>();
  fixture.attach();
  auto &ctx = fixture.ctx;
  InputTrace inputA(ctx, *a), inputB(ctx, *b), inputC(ctx, *c);
  OutputTrace<SFBool> yesA(ctx, *a, "inputTrue"), yesB(ctx, *b, "inputTrue"), yesC(ctx, *c, "inputTrue");
  OutputTrace<SFBool> noA(ctx, *a, "inputFalse"), noB(ctx, *b, "inputFalse"), noC(ctx, *c, "inputFalse");
  OutputTrace<SFBool> negateA(ctx, *a, "inputNegate"), negateB(ctx, *b, "inputNegate"), negateC(ctx, *c, "inputNegate");
  ctx.addRoute({a.get(), "inputTrue"}, {c.get(), "set_boolean"});
  ctx.addRoute({b.get(), "inputTrue"}, {c.get(), "set_boolean"});
  ctx.addRoute({c.get(), "inputTrue"}, {a.get(), "set_boolean"});
  for (int cascade = 1; cascade <= 2; ++cascade) {
    for (auto *node : {a.get(), b.get()}) {
      ctx.postEvent(node, "set_boolean", SFBool{true});
      ctx.postEvent(node, "set_boolean", SFBool{true});
    }
    ctx.tick(10 + cascade);
    CHECK(inputA.count("set_boolean") == 3 * cascade);
    CHECK(inputB.count("set_boolean") == 2 * cascade);
    CHECK(inputC.count("set_boolean") == 2 * cascade);
    for (auto *trace : {&yesA, &yesB, &yesC}) {
      if (cascade == 1) trace->expect({true}); else trace->expect({true, true});
    }
    for (auto *trace : {&negateA, &negateB, &negateC}) {
      if (cascade == 1) trace->expect({false}); else trace->expect({false, false});
    }
    noA.expect({}); noB.expect({}); noC.expect({});
  }
}

// §30.2.4: one value_changed per key interval, across timestamps.
template <class Node, class Value>
void sequencerIntervals(Value first, Value middle, Value last) {
  Utilities fixture;
  auto node = fixture.add<Node>();
  initialize(*node);
  fixture.attach();
  auto &ctx = fixture.ctx;
  InputTrace inputs(ctx, *node);
  OutputTrace<Value> output(ctx, *node, "value_changed");
  double now = 10;
  auto fraction = [&](float f) {
    ctx.postEvent(node.get(), "set_fraction", SFFloat{f});
    ctx.tick(now++);
  };
  fraction(.1f);
  output.expect({first});
  fraction(.2f); // still key[0] interval
  fraction(.4f);
  CHECK(inputs.count("set_fraction") == 3);
  output.expect({first});
  fraction(.5f); // key[1] boundary enters the next interval
  output.expect({first, middle});
  fraction(.9f);
  output.expect({first, middle});
  fraction(1.f);
  fraction(1.5f); // beyond the last key stays in the final interval
  output.expect({first, middle, last});
  fraction(.0f); // backwards into key[0] is a new interval
  output.expect({first, middle, last, first});

  // A step leaves the fraction interval: the same interval emits again.
  ctx.postEvent(node.get(), "next", SFBool{true});
  ctx.tick(now++);
  output.expect({first, middle, last, first, middle});
  fraction(.1f);
  output.expect({first, middle, last, first, middle, first});
  fraction(.2f);
  output.expect({first, middle, last, first, middle, first});

  // Editing keyValue forgets the interval: the same index may hold a new value.
  node->setKeyValue({last, middle, first});
  CHECK(ctx.writeField(node.get(), "keyValue", std::any(node->getKeyValue())) ==
        FieldWriteResult::Ok);
  fraction(.2f);
  output.expect({first, middle, last, first, middle, first, last});
}

TEST_CASE("sequencer emits one value_changed per key interval across timestamps") {
  SUBCASE("BooleanSequencer") { sequencerIntervals<BooleanSequencer>(false, false, true); }
  SUBCASE("IntegerSequencer") { sequencerIntervals<IntegerSequencer>(10, 20, 30); }
}

TEST_CASE("BooleanToggle flips once per TRUE and publishes one toggle per cascade") {
  Utilities fixture;
  auto node = fixture.add<BooleanToggle>();
  fixture.attach();
  auto &ctx = fixture.ctx;
  InputTrace inputs(ctx, *node);
  OutputTrace<SFBool> toggle(ctx, *node, "toggle");

  ctx.postEvent(node.get(), "set_boolean", SFBool{true});
  ctx.tick(10);
  toggle.expect({true});

  // Two TRUE inputs in one cascade flip twice. Each flip reads the previous
  // flip's state; storage, observers and the ROUTE agree on the final value.
  ctx.postEvent(node.get(), "set_boolean", SFBool{true});
  ctx.postEvent(node.get(), "set_boolean", SFBool{true});
  ctx.tick(11);
  CHECK(inputs.count("set_boolean") == 3);
  toggle.expect({true, true});

  // Three TRUE inputs and interleaved FALSE no-ops: odd count flips once.
  ctx.postEvent(node.get(), "set_boolean", SFBool{true});
  ctx.postEvent(node.get(), "set_boolean", SFBool{false});
  ctx.postEvent(node.get(), "set_boolean", SFBool{true});
  ctx.postEvent(node.get(), "set_boolean", SFBool{true});
  ctx.tick(12);
  CHECK(inputs.count("set_boolean") == 7);
  toggle.expect({true, true, false});

  // FALSE alone produces nothing.
  ctx.postEvent(node.get(), "set_boolean", SFBool{false});
  ctx.tick(13);
  toggle.expect({true, true, false});

  // An explicit set_toggle is ordinary inputOutput delivery, and a later TRUE
  // flips from the set value.
  ctx.postEvent(node.get(), "set_toggle", SFBool{true});
  ctx.tick(14);
  toggle.expect({true, true, false, true});
  ctx.postEvent(node.get(), "set_boolean", SFBool{true});
  ctx.tick(15);
  toggle.expect({true, true, false, true, false});

  // In one cascade, set_toggle and a later TRUE are two inputOutput deliveries.
  // State and observers follow both; the ROUTE keeps its legacy one-delivery
  // cap per timestamp, as for any inputOutput field written twice.
  ctx.postEvent(node.get(), "set_toggle", SFBool{true});
  ctx.postEvent(node.get(), "set_boolean", SFBool{true});
  ctx.tick(16);
  CHECK(toggle.observed == std::vector<SFBool>{true, true, false, true, false, true, false});
  CHECK(node->getToggle() == false);
}

TEST_CASE("BooleanToggle routed fan-in flips per arrival") {
  Utilities fixture;
  auto a = fixture.add<BooleanFilter>();
  auto b = fixture.add<BooleanFilter>();
  auto node = fixture.add<BooleanToggle>();
  fixture.attach();
  auto &ctx = fixture.ctx;
  OutputTrace<SFBool> toggle(ctx, *node, "toggle");
  ctx.addRoute({a.get(), "inputTrue"}, {node.get(), "set_boolean"});
  ctx.addRoute({b.get(), "inputTrue"}, {node.get(), "set_boolean"});
  ctx.postEvent(a.get(), "set_boolean", SFBool{true});
  ctx.tick(10);
  toggle.expect({true});
  ctx.postEvent(a.get(), "set_boolean", SFBool{true});
  ctx.postEvent(b.get(), "set_boolean", SFBool{true});
  ctx.tick(11);
  toggle.expect({true, true});
}

} // TEST_SUITE
