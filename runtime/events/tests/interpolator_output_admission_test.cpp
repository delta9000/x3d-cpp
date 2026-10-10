#include "doctest/doctest.h"
#include "InterpolatorRegistration.hpp"
#include "x3d/nodes/GeoLocation.hpp"
#include "x3d/nodes/TextureTransform.hpp"
#include "x3d/nodes/Transform.hpp"

#include <any>
#include <cmath>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;

namespace {

// Exercise the same registry used by the scene bridge, including its callback
// ownership. No test substitutes an interpolator's input handler.
void registerNode(X3DExecutionContext &ctx, X3DNode &node) {
  for (auto &system : makeInterpolatorSystems()) {
    system->attach(&node, ctx);
    ctx.addSystem(std::move(system));
  }
}

template <class Value> Value read(const X3DNode &node, const std::string &field) {
  for (const auto &info : node.fields())
    if (info.x3dName == field && info.get)
      return std::any_cast<Value>(info.get(node));
  throw std::logic_error("missing readable test field: " + field);
}

void checkNear(double actual, double expected) {
  CHECK(actual == doctest::Approx(expected).epsilon(0.0002).scale(1));
}
void checkNear(const SFVec2f &actual, const SFVec2f &expected) {
  checkNear(actual.x, expected.x); checkNear(actual.y, expected.y);
}
void checkNear(const SFVec3f &actual, const SFVec3f &expected) {
  checkNear(actual.x, expected.x); checkNear(actual.y, expected.y);
  checkNear(actual.z, expected.z);
}
void checkNear(const SFVec3d &actual, const SFVec3d &expected) {
  checkNear(actual.x, expected.x); checkNear(actual.y, expected.y);
  checkNear(actual.z, expected.z);
}
void checkNear(const SFRotation &actual, const SFRotation &expected) {
  checkNear(actual.x, expected.x); checkNear(actual.y, expected.y);
  checkNear(actual.z, expected.z); checkNear(actual.angle, expected.angle);
}

// Ordinary generated inputOutput sinks also make the destination's stored value
// available for comparison with its observer and the source's outputOnly field.
template <class Value> struct Sink;
template <> struct Sink<SFFloat> { TextureTransform node; const char *field = "rotation"; };
template <> struct Sink<SFVec2f> { TextureTransform node; const char *field = "translation"; };
template <> struct Sink<SFVec3f> { Transform node; const char *field = "translation"; };
template <> struct Sink<SFRotation> { Transform node; const char *field = "rotation"; };
template <> struct Sink<SFVec3d> { GeoLocation node; const char *field = "geoCoords"; };

template <class Value> struct OutputTrace {
  X3DNode &source;
  std::string field;
  Sink<Value> sink;
  Value initial, first, next;
  std::vector<Value> observed, routed;

  OutputTrace(X3DExecutionContext &ctx, X3DNode &node, const char *name,
              Value initialValue, Value firstValue, Value nextValue)
      : source(node), field(name), initial(initialValue), first(firstValue), next(nextValue) {
    ctx.addRoute({&source, field}, {&sink.node, std::string("set_") + sink.field});
    ctx.addFieldWriteListener([this](const FieldAddress &address) {
      if (address.node == &source && address.field == field)
        observed.push_back(read<Value>(source, field));
      if (address.node == &sink.node && address.field == sink.field)
        routed.push_back(read<Value>(sink.node, sink.field));
    });
  }

  void silent() const {
    INFO(source.nodeTypeName() << '.' << field);
    CHECK(observed.empty());
    CHECK(routed.empty());
    CHECK(read<Value>(source, field) == initial);
  }

  void admitted(std::size_t count, bool later = false) const {
    INFO(source.nodeTypeName() << '.' << field);
    REQUIRE(observed.size() == count);
    REQUIRE(routed.size() == count);
    checkNear(observed.back(), later ? next : first);
    CHECK(read<Value>(source, field) == observed.back());
    CHECK(routed.back() == observed.back());
    CHECK(read<Value>(sink.node, sink.field) == observed.back());
  }
};

template <class Input, class... Traces>
void repeatedInputs(X3DExecutionContext &ctx, X3DNode &node, Input first,
                    Input next, bool identical, Traces &...traces) {
  std::size_t received = 0;
  ctx.addFieldWriteListener([&](const FieldAddress &address) {
    if (address.node == &node && address.field == "set_fraction") ++received;
  });
  registerNode(ctx, node);
  (traces.silent(), ...);
  ctx.tick(9);
  CHECK(received == 0);
  (traces.silent(), ...);
  for (const auto &fraction : {first, first, identical ? first : next})
    ctx.postEvent(&node, "set_fraction", fraction);
  ctx.tick(10);
  CHECK(received == 3);
  // First-admitted is this implementation's policy. The portable requirements
  // are event cardinality and agreement between output storage and delivery.
  (traces.admitted(1), ...);
  ctx.postEvent(&node, "set_fraction", next);
  ctx.tick(11);
  CHECK(received == 4);
  (traces.admitted(2, true), ...);
}

template <class Node, class Value, class Initialize>
void singleOutput(Initialize initialize, Value initial, Value first, Value next,
                  const char *field = "value_changed") {
  for (bool identical : {false, true}) {
    INFO("identical arrivals: " << identical);
    Node node;
    initialize(node);
    X3DExecutionContext ctx;
    OutputTrace<Value> trace(ctx, node, field, initial, first, next);
    repeatedInputs(ctx, node, .25f, .75f, identical, trace);
  }
}

void initialize(SplineScalarInterpolator &node) {
  node.setKey({0, 1}); node.setKeyValue({2, 10});
  node.setKeyVelocity({0, 0});
}
void initialize(SplinePositionInterpolator &node) {
  node.setKey({0, 1}); node.setKeyValue({{2, 4, 6}, {10, 20, 30}});
  node.setKeyVelocity({{0, 0, 0}, {0, 0, 0}});
}
void initialize(SplinePositionInterpolator2D &node) {
  node.setKey({0, 1}); node.setKeyValue({{2, 4}, {10, 20}});
  node.setKeyVelocity({{0, 0}, {0, 0}});
}
void initialize(SquadOrientationInterpolator &node) {
  node.setKey({0, 1}); node.setKeyValue({{0, 0, 1, .4f}, {0, 0, 1, 2.f}});
}
void initialize(EaseInEaseOut &node) {
  node.setKey({0, 1}); node.setEaseInEaseOut({{0, .5f}, {.5f, 0}});
}
void initialize(NurbsPositionInterpolator &node) {
  auto cp = std::make_shared<Coordinate>();
  cp->setPoint({{2, 4, 6}, {10, 20, 30}});
  node.setControlPoint(cp); node.setOrder(2);
}
void initialize(NurbsOrientationInterpolator &node) {
  // Quadratic Bezier P(t)=(t,t*t,0); tangent=(1,2*t,0).
  auto cp = std::make_shared<Coordinate>();
  cp->setPoint({{0, 0, 0}, {.5f, 0, 0}, {1, 1, 0}});
  node.setControlPoint(cp); node.setOrder(3);
}
void initialize(NurbsSurfaceInterpolator &node) {
  // Bilinear P(u,v)=(u,v,u*v), so both position and normal vary.
  auto cp = std::make_shared<Coordinate>();
  cp->setPoint({{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 1}});
  node.setControlPoint(cp);
  node.setUDimensionUnchecked(2); node.setVDimensionUnchecked(2);
  node.setUOrderUnchecked(2); node.setVOrderUnchecked(2);
}
void initialize(GeoPositionInterpolator &node) {
  node.setGeoSystemUnchecked({"GC"});
  node.setKey({0, 1}); node.setKeyValue({{2, 4, 6}, {10, 20, 30}});
}

constexpr float halfPi = 1.5707963267948966f;
SFRotation tangentRotation(float slope) {
  const float scale = std::sqrt(1 + slope * slope);
  return {-slope / scale, 1 / scale, 0, halfPi};
}
SFVec3f surfaceNormal(float u, float v) {
  const float scale = std::sqrt(1 + u * u + v * v);
  return {-v / scale, -u / scale, 1 / scale};
}

template <class Input, class Invalidate, class Repair, class... Traces>
void rejectedThenValid(X3DExecutionContext &ctx, X3DNode &node,
                       Input first, Input next, Invalidate invalidate,
                       Repair repair, Traces &...traces) {
  std::size_t received = 0;
  ctx.addFieldWriteListener([&](const FieldAddress &address) {
    if (address.node == &node && address.field == "set_fraction") ++received;
  });
  registerNode(ctx, node);
  (traces.silent(), ...);
  invalidate();
  bool repaired = false;
  ctx.addPostCascadeHook([&](X3DExecutionContext &context) {
    if (repaired) return;
    repaired = true;
    CHECK(received == 2);
    (traces.silent(), ...);
    repair();
    context.postEvent(&node, "set_fraction", first);
    context.postEvent(&node, "set_fraction", next);
    context.process(); // still the same tick, with no new admission budget
  });
  ctx.postEvent(&node, "set_fraction", first);
  ctx.postEvent(&node, "set_fraction", next);
  ctx.tick(10);
  CHECK(repaired);
  CHECK(received == 4);
  (traces.admitted(1), ...);
  ctx.postEvent(&node, "set_fraction", next);
  ctx.tick(11);
  CHECK(received == 5);
  (traces.admitted(2, true), ...);
}

template <class Node, class Value, class Invalidate>
void rejectedSingle(Invalidate invalidate, Value initial, Value first, Value next) {
  Node node;
  initialize(node);
  X3DExecutionContext ctx;
  OutputTrace<Value> trace(ctx, node, "value_changed", initial, first, next);
  rejectedThenValid(ctx, node, .25f, .75f, [&] { invalidate(node); },
                    [&] { initialize(node); }, trace);
}

template <class Input, class... Traces>
void reentrantPair(X3DExecutionContext &ctx, X3DNode &node,
                   const char *trigger, Input first, Input next, Traces &...traces) {
  registerNode(ctx, node);
  std::size_t received = 0;
  bool nested = false;
  ctx.addFieldWriteListener([&](const FieldAddress &address) {
    if (address.node != &node) return;
    if (address.field == "set_fraction") ++received;
    if (address.field == trigger && !nested) {
      nested = true;
      ctx.postEvent(&node, "set_fraction", next);
      ctx.process();
    }
  });
  ctx.postEvent(&node, "set_fraction", first);
  ctx.tick(10);
  CHECK(nested);
  CHECK(received == 2);
  (traces.admitted(1), ...);
  ctx.postEvent(&node, "set_fraction", next);
  ctx.tick(11);
  CHECK(received == 3);
  (traces.admitted(2, true), ...);
}

} // namespace

TEST_CASE("registered nonlinear interpolators admit coherent repeated outputs") {
  SUBCASE("SplineScalarInterpolator") {
    singleOutput<SplineScalarInterpolator>([](auto &n) { initialize(n); }, 2.f, 3.25f, 8.75f);
  }
  SUBCASE("SplinePositionInterpolator") {
    singleOutput<SplinePositionInterpolator>([](auto &n) { initialize(n); },
        SFVec3f{2, 4, 6}, SFVec3f{3.25f, 6.5f, 9.75f}, SFVec3f{8.75f, 17.5f, 26.25f});
  }
  SUBCASE("SplinePositionInterpolator2D") {
    singleOutput<SplinePositionInterpolator2D>([](auto &n) { initialize(n); },
        SFVec2f{2, 4}, SFVec2f{3.25f, 6.5f}, SFVec2f{8.75f, 17.5f});
  }
  SUBCASE("SquadOrientationInterpolator") {
    singleOutput<SquadOrientationInterpolator>([](auto &n) { initialize(n); },
        SFRotation{0, 0, 1, .4f}, SFRotation{0, 0, 1, .8f}, SFRotation{0, 0, 1, 1.6f});
  }
  SUBCASE("EaseInEaseOut") {
    singleOutput<EaseInEaseOut>([](auto &n) { initialize(n); },
        0.f, .125f, .875f, "modifiedFraction_changed");
  }
  SUBCASE("NurbsPositionInterpolator") {
    singleOutput<NurbsPositionInterpolator>([](auto &n) { initialize(n); },
        SFVec3f{0, 0, 0}, SFVec3f{4, 8, 12}, SFVec3f{8, 16, 24});
  }
  SUBCASE("NurbsOrientationInterpolator") {
    singleOutput<NurbsOrientationInterpolator>([](auto &n) { initialize(n); },
        SFRotation{0, 0, 0, 0}, tangentRotation(.5f), tangentRotation(1.5f));
  }
}

TEST_CASE("registered paired interpolator outputs each receive admission") {
  SUBCASE("NurbsSurfaceInterpolator") {
    for (bool identical : {false, true}) {
      INFO("identical arrivals: " << identical);
      NurbsSurfaceInterpolator node;
      initialize(node);
      X3DExecutionContext ctx;
      OutputTrace<SFVec3f> position(ctx, node, "position_changed", {0, 0, 0},
          {.25f, .25f, .0625f}, {.75f, .75f, .5625f});
      OutputTrace<SFVec3f> normal(ctx, node, "normal_changed", {0, 0, 0},
          surfaceNormal(.25f, .25f), surfaceNormal(.75f, .75f));
      repeatedInputs(ctx, node, SFVec2f{.25f, .25f}, SFVec2f{.75f, .75f}, identical, position, normal);
    }
  }
  SUBCASE("GeoPositionInterpolator") {
    for (bool identical : {false, true}) {
      INFO("identical arrivals: " << identical);
      GeoPositionInterpolator node;
      initialize(node);
      X3DExecutionContext ctx;
      OutputTrace<SFVec3d> authored(ctx, node, "geovalue_changed", {2, 4, 6}, {4, 8, 12}, {8, 16, 24});
      OutputTrace<SFVec3f> world(ctx, node, "value_changed", {2, 4, 6}, {4, 8, 12}, {8, 16, 24});
      repeatedInputs(ctx, node, .25f, .75f, identical, authored, world);
    }
  }
}

TEST_CASE("rejected interpolator evaluations leave same-tick output admission available") {
  const auto emptyKeys = [](auto &node) { node.setKey({}); };
  SUBCASE("SplineScalarInterpolator empty keys") {
    rejectedSingle<SplineScalarInterpolator>(emptyKeys, 2.f, 3.25f, 8.75f);
  }
  SUBCASE("SplinePositionInterpolator empty keys") {
    rejectedSingle<SplinePositionInterpolator>(emptyKeys, SFVec3f{2, 4, 6},
        SFVec3f{3.25f, 6.5f, 9.75f}, SFVec3f{8.75f, 17.5f, 26.25f});
  }
  SUBCASE("SplinePositionInterpolator2D empty keys") {
    rejectedSingle<SplinePositionInterpolator2D>(emptyKeys, SFVec2f{2, 4},
        SFVec2f{3.25f, 6.5f}, SFVec2f{8.75f, 17.5f});
  }
  SUBCASE("SquadOrientationInterpolator empty keys") {
    rejectedSingle<SquadOrientationInterpolator>(emptyKeys, SFRotation{0, 0, 1, .4f},
        SFRotation{0, 0, 1, .8f}, SFRotation{0, 0, 1, 1.6f});
  }
  SUBCASE("NurbsPositionInterpolator missing control points") {
    rejectedSingle<NurbsPositionInterpolator>([](auto &n) { n.setControlPoint(nullptr); },
        SFVec3f{0, 0, 0}, SFVec3f{4, 8, 12}, SFVec3f{8, 16, 24});
  }
  SUBCASE("NurbsOrientationInterpolator degenerate tangent") {
    rejectedSingle<NurbsOrientationInterpolator>([](auto &n) {
      auto cp = std::make_shared<Coordinate>();
      cp->setPoint({{1, 1, 1}, {1, 1, 1}, {1, 1, 1}});
      n.setControlPoint(cp);
    }, SFRotation{0, 0, 0, 0}, tangentRotation(.5f), tangentRotation(1.5f));
  }
  SUBCASE("NurbsSurfaceInterpolator incomplete control net") {
    NurbsSurfaceInterpolator node;
    initialize(node);
    X3DExecutionContext ctx;
    OutputTrace<SFVec3f> position(ctx, node, "position_changed", {0, 0, 0},
        {.25f, .25f, .0625f}, {.75f, .75f, .5625f});
    OutputTrace<SFVec3f> normal(ctx, node, "normal_changed", {0, 0, 0},
        surfaceNormal(.25f, .25f), surfaceNormal(.75f, .75f));
    rejectedThenValid(ctx, node, SFVec2f{.25f, .25f}, SFVec2f{.75f, .75f},
        [&] { node.setControlPoint(nullptr); }, [&] { initialize(node); }, position, normal);
  }
  SUBCASE("GeoPositionInterpolator empty keys or values") {
    for (bool emptyValues : {false, true}) {
      INFO("empty keyValue: " << emptyValues);
      GeoPositionInterpolator node;
      initialize(node);
      X3DExecutionContext ctx;
      OutputTrace<SFVec3d> authored(ctx, node, "geovalue_changed", {2, 4, 6}, {4, 8, 12}, {8, 16, 24});
      OutputTrace<SFVec3f> world(ctx, node, "value_changed", {2, 4, 6}, {4, 8, 12}, {8, 16, 24});
      rejectedThenValid(ctx, node, .25f, .75f, [&] {
        if (emptyValues) node.setKeyValue({}); else node.setKey({});
      }, [&] { initialize(node); }, authored, world);
    }
  }
  SUBCASE("GeoPositionInterpolator failed projection") {
    GeoPositionInterpolator node;
    initialize(node);
    X3DExecutionContext ctx;
    OutputTrace<SFVec3d> authored(ctx, node, "geovalue_changed", {2, 4, 6}, {4, 8, 12}, {8, 16, 24});
    OutputTrace<SFVec3f> world(ctx, node, "value_changed", {2, 4, 6}, {4, 8, 12}, {8, 16, 24});
    // UTM without a zone has an existing conversion-failure path. The unchecked
    // authoring setter is used only to construct and repair this fixture.
    rejectedThenValid(ctx, node, .25f, .75f,
        [&] { node.setGeoSystemUnchecked({"UTM"}); },
        [&] { node.setGeoSystemUnchecked({"GC"}); }, authored, world);
  }
}

TEST_CASE("EaseInEaseOut insufficient data keeps its admitted passthrough") {
  singleOutput<EaseInEaseOut>([](auto &) {}, 0.f, .25f, .75f, "modifiedFraction_changed");
}

TEST_CASE("paired interpolator admission survives reentrant evaluation between outputs") {
  SUBCASE("NurbsSurfaceInterpolator") {
    NurbsSurfaceInterpolator node;
    initialize(node);
    X3DExecutionContext ctx;
    OutputTrace<SFVec3f> position(ctx, node, "position_changed", {0, 0, 0},
        {.25f, .25f, .0625f}, {.75f, .75f, .5625f});
    OutputTrace<SFVec3f> normal(ctx, node, "normal_changed", {0, 0, 0},
        surfaceNormal(.25f, .25f), surfaceNormal(.75f, .75f));
    reentrantPair(ctx, node, "position_changed", SFVec2f{.25f, .25f},
                  SFVec2f{.75f, .75f}, position, normal);
  }
  SUBCASE("GeoPositionInterpolator") {
    GeoPositionInterpolator node;
    initialize(node);
    X3DExecutionContext ctx;
    OutputTrace<SFVec3d> authored(ctx, node, "geovalue_changed", {2, 4, 6}, {4, 8, 12}, {8, 16, 24});
    OutputTrace<SFVec3f> world(ctx, node, "value_changed", {2, 4, 6}, {4, 8, 12}, {8, 16, 24});
    reentrantPair(ctx, node, "geovalue_changed", .25f, .75f, authored, world);
  }
}
