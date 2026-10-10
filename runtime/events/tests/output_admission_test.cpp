#include "doctest/doctest.h"
#include "InterpolatorSystem.hpp"
#include "x3d/nodes/CoordinateInterpolator.hpp"
#include "x3d/nodes/PositionInterpolator.hpp"
#include "x3d/nodes/ScalarInterpolator.hpp"
#include "x3d/nodes/Transform.hpp"

#include <memory>
#include <vector>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;

namespace {
using PositionSystem = InterpolatorSystem<PositionInterpolator, SFVec3f>;
using ScalarSystem = InterpolatorSystem<ScalarInterpolator, SFFloat>;
PositionSystem positionSystem() { return PositionSystem(lerpVec3); }
ScalarSystem scalarSystem() { return ScalarSystem(lerpf); }
void initialize(PositionInterpolator &node) {
  node.setKey(MFFloat{0, 1});
  node.setKeyValue(MFVec3f{{0, 0, 0}, {8, 16, 24}});
}
void initialize(ScalarInterpolator &node) {
  node.setKey(MFFloat{0, 1});
  node.setKeyValue(MFFloat{0, 1});
}
}

TEST_CASE("output admission preserves equal and distinct input occurrences") {
  for (const auto &fractions : {MFFloat{.25f, .25f, .75f}, MFFloat{.25f, .25f, .25f}}) {
    X3DExecutionContext ctx;
    PositionInterpolator node;
    Transform sink;
    initialize(node);
    auto system = positionSystem();
    system.attach(&node, ctx);
    ctx.addRoute({&node, "value_changed"}, {&sink, "set_translation"});
    std::size_t inputs = 0;
    std::vector<SFVec3f> outputs, routed;
    ctx.addFieldWriteListener([&](const FieldAddress &address) {
      if (address.node == &node && address.field == "set_fraction") ++inputs;
      if (address.node == &node && address.field == "value_changed")
        outputs.push_back(node.getValue_changed());
      if (address.node == &sink && address.field == "translation")
        routed.push_back(sink.getTranslation());
    });
    for (auto fraction : fractions) ctx.postEvent(&node, "set_fraction", fraction);
    ctx.tick(10);
    CHECK(inputs == fractions.size());
    REQUIRE(outputs.size() == 1);
    REQUIRE(routed.size() == 1);
    // First-admitted is this native implementation's policy, not ISO's unique
    // answer. Portable conformance checks cardinality and readback coherence.
    CHECK(outputs.front().x == 2);
    CHECK(outputs.front().y == 4);
    CHECK(outputs.front().z == 6);
    CHECK(node.getValue_changed().x == outputs.front().x);
    CHECK(node.getValue_changed().y == outputs.front().y);
    CHECK(node.getValue_changed().z == outputs.front().z);
    CHECK(routed.front().x == outputs.front().x);
    CHECK(routed.front().y == outputs.front().y);
    CHECK(routed.front().z == outputs.front().z);
    ctx.postEvent(&node, "set_fraction", .75f);
    ctx.tick(11);
    CHECK(inputs == fractions.size() + 1);
    REQUIRE(outputs.size() == 2);
    REQUIRE(routed.size() == 2);
    CHECK(outputs.back().x == 6);
    CHECK(node.getValue_changed().x == outputs.back().x);
    CHECK(routed.back().x == outputs.back().x);
  }
}

TEST_CASE("output admission does not suppress inputOnly fan-in or loop arrivals") {
  X3DExecutionContext ctx;
  ScalarInterpolator a, b, sink;
  for (auto *node : {&a, &b, &sink}) initialize(*node);
  auto system = scalarSystem();
  for (auto *node : {&a, &b, &sink}) system.attach(node, ctx);
  ctx.addRoute({&a, "value_changed"}, {&sink, "set_fraction"});
  ctx.addRoute({&b, "value_changed"}, {&sink, "set_fraction"});
  ctx.addRoute({&sink, "value_changed"}, {&a, "set_fraction"});
  std::size_t aInputs = 0, bInputs = 0, sinkInputs = 0;
  std::size_t aOutputs = 0, bOutputs = 0, sinkOutputs = 0;
  ctx.addFieldWriteListener([&](const FieldAddress &address) {
    if (address.field == "set_fraction") {
      if (address.node == &a) ++aInputs;
      if (address.node == &b) ++bInputs;
      if (address.node == &sink) ++sinkInputs;
    } else if (address.field == "value_changed") {
      if (address.node == &a) ++aOutputs;
      if (address.node == &b) ++bOutputs;
      if (address.node == &sink) ++sinkOutputs;
    }
  });
  ctx.postEvent(&a, "set_fraction", .25f);
  ctx.postEvent(&b, "set_fraction", .75f);
  ctx.tick(10);
  CHECK(aInputs == 2); // external input plus the loop's returning ROUTE
  CHECK(bInputs == 1);
  CHECK(sinkInputs == 2); // distinct ROUTEs both deliver to inputOnly
  CHECK(aOutputs == 1);
  CHECK(bOutputs == 1);
  CHECK(sinkOutputs == 1);
  CHECK(a.getValue_changed() == .25f);
  CHECK(b.getValue_changed() == .75f);
  CHECK(sink.getValue_changed() == .25f);
}

TEST_CASE("output admission covers the multi-value interpolator family") {
  X3DExecutionContext ctx;
  CoordinateInterpolator node;
  node.setKey(MFFloat{0, 1});
  node.setKeyValue(MFVec3f{{0, 0, 0}, {4, 8, 12}, {8, 16, 24}, {12, 24, 36}});
  MultiInterpolatorSystem<CoordinateInterpolator, SFVec3f> system(lerpVec3);
  system.attach(&node, ctx);
  std::size_t inputs = 0, outputs = 0;
  ctx.addFieldWriteListener([&](const FieldAddress &address) {
    if (address.field == "set_fraction") ++inputs;
    if (address.field == "value_changed") ++outputs;
  });
  for (auto fraction : {.25f, .25f, .75f}) ctx.postEvent(&node, "set_fraction", fraction);
  ctx.tick(10);
  CHECK(inputs == 3);
  CHECK(outputs == 1);
  const auto &value = node.getValue_changed();
  REQUIRE(value.size() == 2);
  CHECK(value[0].x == 2);
  CHECK(value[1].x == 6);
}

TEST_CASE("output admission guard spans system re-evaluation within a tick") {
  class InputSystem : public System {
  public:
    void attach(X3DNode *, X3DExecutionContext &) override {}
    PositionInterpolator *node = nullptr;
    std::size_t passes = 0;
    void update(double, X3DExecutionContext &ctx) override {
      ctx.postEvent(node, "set_fraction", passes++ == 0 ? .25f : .75f);
    }
  };
  X3DExecutionContext ctx;
  PositionInterpolator node;
  initialize(node);
  auto system = positionSystem();
  system.attach(&node, ctx);
  auto inputs = std::make_shared<InputSystem>();
  inputs->node = &node;
  ctx.addSystem(inputs);
  std::size_t deliveries = 0, outputs = 0;
  ctx.addFieldWriteListener([&](const FieldAddress &address) {
    if (address.field == "set_fraction") ++deliveries;
    if (address.field == "value_changed") ++outputs;
  });
  ctx.tick(10);
  CHECK(inputs->passes == 2);
  CHECK(deliveries == 2);
  CHECK(outputs == 1);
  CHECK(node.getValue_changed().x == 2);
}

TEST_CASE("output admission validates endpoints and never pre-mutates storage") {
  EventGraph graph;
  ScalarInterpolator source, sink;
  graph.addRoute({&source, "value_changed"}, {&sink, "set_fraction"});
  EventCascade cascade(graph);
  std::size_t outputs = 0, inputs = 0;
  sink.setOnSet_fractionHandler([&](const SFFloat &value) {
    ++inputs;
    CHECK(value == .25f);
  });
  cascade.setFieldObserver([&](const FieldAddress &address) {
    if (address.node == &source) {
      ++outputs;
      CHECK(source.getValue_changed() == .25f);
      cascade.postOutputEvent(&source, "value_changed", .9f);
      CHECK(source.getValue_changed() == .25f);
    }
  });
  CHECK_THROWS_AS(cascade.postOutputEvent(nullptr, "value_changed", .5f), std::invalid_argument);
  CHECK_THROWS_AS(cascade.postOutputEvent(&source, "missing", .5f), std::invalid_argument);
  CHECK_THROWS_AS(cascade.postOutputEvent(&source, "set_fraction", .5f), std::invalid_argument);
  CHECK_THROWS_AS(cascade.postOutputEvent(&source, "key", MFFloat{0, 1}), std::invalid_argument);
  cascade.postOutputEvent(&source, "value_changed", .25f);
  cascade.postOutputEvent(&source, "value_changed", .75f);
  CHECK(source.getValue_changed() == 0);
  cascade.beginTimestamp();
  CHECK(cascade.process(false) == 2);
  CHECK(outputs == 1);
  CHECK(inputs == 1);
  CHECK(source.getValue_changed() == .25f);
  cascade.postOutputEvent(&source, "value_changed", .75f);
  CHECK(cascade.process(false) == 0);
  CHECK(outputs == 1);
  CHECK(inputs == 1);
  CHECK(source.getValue_changed() == .25f);
}

TEST_CASE("output admission routes fan-in through the owning author-field store") {
  auto fields = std::make_shared<DynamicFieldStore>();
  ScalarInterpolator a, b;
  Transform sink;
  AuthorFieldDecl input;
  input.x3dName = "receive";
  input.type = X3DFieldType::SFFloat;
  input.access = AccessType::InputOnly;
  fields->addAuthorFields(sink, {input});
  AuthorFieldDecl output = input;
  output.x3dName = "result";
  output.access = AccessType::OutputOnly;
  fields->addAuthorFields(sink, {output});
  EventGraph graph;
  graph.addRoute({&a, "value_changed"}, {&sink, "receive"});
  graph.addRoute({&b, "value_changed"}, {&sink, "receive"});
  EventCascade cascade(graph, fields);
  std::vector<SFFloat> received;
  std::size_t outputs = 0;
  cascade.addAuthorInputListener([&](const FieldAddress &, const FieldInfo &, const std::any &value) {
    received.push_back(std::any_cast<SFFloat>(value));
  });
  cascade.setFieldObserver([&](const FieldAddress &address) {
    if (address.field == "value_changed") ++outputs;
  });
  cascade.postOutputEvent(&a, "value_changed", .25f);
  cascade.postOutputEvent(&a, "value_changed", .5f);
  cascade.postOutputEvent(&b, "value_changed", .75f);
  cascade.process();
  CHECK(outputs == 2);
  CHECK(received == MFFloat{.25f, .75f});
  CHECK(a.getValue_changed() == .25f);
  CHECK(b.getValue_changed() == .75f);
  CHECK(std::any_cast<SFFloat>(fields->getValue(sink, "receive")) == .75f);
  // Author outputOnly currently has no setter thunk. Do not silently accept
  // such output until Script storage/admission is migrated coherently.
  CHECK_THROWS_AS(cascade.postOutputEvent(&sink, "result", .5f), std::invalid_argument);
  EventCascade independent(graph);
  CHECK_THROWS_AS(independent.postOutputEvent(&sink, "result", .5f), std::invalid_argument);
  CHECK(independent.authorFields().entryCount() == 0);
}

TEST_CASE("output admission survives post-cascade process within a tick") {
  X3DExecutionContext ctx;
  PositionInterpolator node;
  Transform sink;
  initialize(node);
  auto system = positionSystem();
  system.attach(&node, ctx);
  ctx.addRoute({&node, "value_changed"}, {&sink, "translation"});
  std::size_t inputs = 0, outputs = 0, routes = 0;
  ctx.addFieldWriteListener([&](const FieldAddress &address) {
    if (address.node == &node && address.field == "set_fraction") ++inputs;
    if (address.node == &node && address.field == "value_changed") ++outputs;
    if (address.node == &sink && address.field == "translation") ++routes;
  });
  ctx.addPostCascadeHook([&](X3DExecutionContext &context) {
    context.postEvent(&node, "set_fraction", .75f);
    context.process();
  });
  ctx.postEvent(&node, "set_fraction", .25f);
  ctx.tick(10);
  CHECK(inputs == 2);
  CHECK(outputs == 1);
  CHECK(routes == 1);
  CHECK(node.getValue_changed().x == 2);
  CHECK(sink.getTranslation().x == 2);
}

TEST_CASE("output admission survives a nested standalone process") {
  X3DExecutionContext ctx;
  PositionInterpolator node;
  Transform sink;
  initialize(node);
  auto system = positionSystem();
  system.attach(&node, ctx);
  ctx.addRoute({&node, "value_changed"}, {&sink, "translation"});
  std::size_t inputs = 0, outputs = 0, routes = 0;
  bool nested = false;
  ctx.addFieldWriteListener([&](const FieldAddress &address) {
    if (address.node == &node && address.field == "set_fraction") ++inputs;
    if (address.node == &node && address.field == "value_changed") {
      ++outputs;
      if (!nested) {
        nested = true;
        ctx.postEvent(&node, "set_fraction", .75f);
        ctx.process();
      }
    }
    if (address.node == &sink && address.field == "translation") ++routes;
  });
  ctx.postEvent(&node, "set_fraction", .25f);
  ctx.process();
  CHECK(inputs == 2);
  CHECK(outputs == 1);
  CHECK(routes == 1);
  CHECK(node.getValue_changed().x == 2);
  CHECK(sink.getTranslation().x == 2);
  // A subsequent outermost standalone process still starts a fresh cascade.
  ctx.postEvent(&node, "set_fraction", .75f);
  ctx.process();
  CHECK(inputs == 3);
  CHECK(outputs == 2);
  CHECK(routes == 2);
  CHECK(node.getValue_changed().x == 6);
  CHECK(sink.getTranslation().x == 6);
}

TEST_CASE("output admission restores drain depth on callback exception") {
  EventGraph graph;
  EventCascade cascade(graph);
  ScalarInterpolator node;
  bool throws = true;
  cascade.setFieldObserver([&](const FieldAddress &) {
    if (throws) throw std::runtime_error("test observer");
  });
  cascade.postOutputEvent(&node, "value_changed", .25f);
  CHECK_THROWS_AS(cascade.process(), std::runtime_error);
  throws = false;
  cascade.postOutputEvent(&node, "value_changed", .75f);
  cascade.process();
  CHECK(node.getValue_changed() == .75f);
}
