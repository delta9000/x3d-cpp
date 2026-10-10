#include "RuntimeSession.hpp"
#include "x3d/nodes/ScalarInterpolator.hpp"
#include "x3d/nodes/Viewpoint.hpp"
#include "x3d/nodes/OrthoViewpoint.hpp"
#include "x3d/nodes/TextureBackground.hpp"
#include "x3d/nodes/Fog.hpp"
#include "x3d/nodes/Background.hpp"
#include "x3d/nodes/BooleanFilter.hpp"
#include "x3d/nodes/BooleanSequencer.hpp"
#include "x3d/nodes/BooleanToggle.hpp"
#include "x3d/nodes/BooleanTrigger.hpp"
#include "x3d/nodes/ColorChaser.hpp"
#include "x3d/nodes/ColorDamper.hpp"
#include "x3d/nodes/ColorInterpolator.hpp"
#include "x3d/nodes/CoordinateChaser.hpp"
#include "x3d/nodes/CoordinateDamper.hpp"
#include "x3d/nodes/CoordinateInterpolator.hpp"
#include "x3d/nodes/CoordinateInterpolator2D.hpp"
#include "x3d/nodes/EaseInEaseOut.hpp"
#include "x3d/nodes/GeoPositionInterpolator.hpp"
#include "x3d/nodes/GeoViewpoint.hpp"
#include "x3d/nodes/IntegerSequencer.hpp"
#include "x3d/nodes/IntegerTrigger.hpp"
#include "x3d/nodes/NavigationInfo.hpp"
#include "x3d/nodes/NormalInterpolator.hpp"
#include "x3d/nodes/NurbsOrientationInterpolator.hpp"
#include "x3d/nodes/NurbsPositionInterpolator.hpp"
#include "x3d/nodes/NurbsSurfaceInterpolator.hpp"
#include "x3d/nodes/OrientationChaser.hpp"
#include "x3d/nodes/OrientationDamper.hpp"
#include "x3d/nodes/OrientationInterpolator.hpp"
#include "x3d/nodes/PositionChaser.hpp"
#include "x3d/nodes/PositionChaser2D.hpp"
#include "x3d/nodes/PositionDamper.hpp"
#include "x3d/nodes/PositionDamper2D.hpp"
#include "x3d/nodes/PositionInterpolator.hpp"
#include "x3d/nodes/PositionInterpolator2D.hpp"
#include "x3d/nodes/ScalarChaser.hpp"
#include "x3d/nodes/ScalarDamper.hpp"
#include "x3d/nodes/SplinePositionInterpolator.hpp"
#include "x3d/nodes/SplinePositionInterpolator2D.hpp"
#include "x3d/nodes/SplineScalarInterpolator.hpp"
#include "x3d/nodes/SquadOrientationInterpolator.hpp"
#include "x3d/nodes/TexCoordChaser2D.hpp"
#include "x3d/nodes/TexCoordDamper2D.hpp"
#include "x3d/nodes/TimeTrigger.hpp"
#include "doctest/doctest.h"

#include <functional>
#include <stdexcept>
#include <type_traits>

#if defined(__unix__)
#include <sys/wait.h>
#include <unistd.h>
#endif

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;

static_assert(!std::is_copy_constructible_v<X3DExecutionContext>);
static_assert(!std::is_move_constructible_v<X3DExecutionContext>);
static_assert(!std::is_copy_assignable_v<X3DExecutionContext>);
static_assert(!std::is_move_assignable_v<X3DExecutionContext>);

static_assert(!std::is_copy_constructible_v<BindingSystem>);
static_assert(!std::is_move_constructible_v<BindingSystem>);
static_assert(!std::is_copy_constructible_v<BooleanTriggerSystem>);
static_assert(!std::is_move_constructible_v<TimeSensorSystem>);
static_assert(!std::is_copy_constructible_v<hanim::HAnimMotionSystem>);
static_assert(!std::is_copy_constructible_v<DamperSystem<ScalarDamper, float>>);

namespace {
std::shared_ptr<ScalarInterpolator> interpolator() {
  auto n = std::make_shared<ScalarInterpolator>();
  n->setKey({0, 1});
  n->setKeyValue({1, 9});
  return n;
}
std::unique_ptr<RuntimeSession> sessionFor(const MFNode &nodes) {
  X3DDocument doc;
  doc.scene.rootNodes = nodes;
  return RuntimeSession::create(std::move(doc));
}
class EmptySystem : public System {
public:
  void attach(X3DNode *, X3DExecutionContext &) override {}
};
class FailingAfterInterpolator : public X3DNode {
public:
  explicit FailingAfterInterpolator(std::shared_ptr<ScalarInterpolator> n)
      : observed_(std::move(n)) {}
  const FieldTable &fields() const override {
    // Fail during production attachment, after the ScalarInterpolator system
    // has installed its first node callback but before ctx.addSystem(sys).
    if (observed_->getValue_changed() == 1)
      throw std::runtime_error("injected attachment failure");
    return X3DNode::fields();
  }
private:
  std::shared_ptr<ScalarInterpolator> observed_;
};
}

TEST_CASE("Runtime retirement: retained native nodes are inert after session teardown") {
  auto value = interpolator();
  auto vp = std::make_shared<Viewpoint>();
  auto damper = std::make_shared<ScalarDamper>();
  auto toggle = std::make_shared<BooleanToggle>();
  auto session = sessionFor({vp, value, damper, toggle});
  value->onSet_fraction(0.5f);
  damper->onSet_value(3);
  toggle->onSet_boolean(true);
  session->context().process();
  CHECK(value->getValue_changed() == doctest::Approx(5));
  CHECK(damper->getValue_changed() == 3);
  CHECK(toggle->getToggle());
  CHECK(session->context().boundViewpoint() == vp.get());
  session.reset();

  // Native storage can still be read or mutated; only retired runtime handlers
  // are inert. None may call the freed context, binding stack or follower entry.
  vp->onSet_bind(false);
  value->onSet_fraction(1);
  damper->onSet_destination(17);
  damper->onSet_value(19);
  toggle->onSet_boolean(true);
  CHECK(value->getValue_changed() == doctest::Approx(5));
  CHECK(damper->getValue_changed() == 3);
  CHECK(toggle->getToggle());
  value->setKeyValue({2, 4});
  CHECK(value->getKeyValue().back() == 4);
}

TEST_CASE("Runtime retirement: replacement bindings are independent of old owners") {
  auto value = interpolator();
  auto vp = std::make_shared<Viewpoint>();
  auto old = sessionFor({vp, value});
  REQUIRE(old->context().retireCallbacks());
  REQUIRE(old->context().retireCallbacks()); // idempotent
  value->onSet_fraction(1);
  CHECK(value->getValue_changed() == 1);

  auto next = sessionFor({vp, value});
  old.reset(); // revocation must not clear the replacement's installed handlers
  value->onSet_fraction(0.75f);
  next->context().process();
  CHECK(value->getValue_changed() == doctest::Approx(7));
  vp->onSet_bind(false);
  next->context().process();
  CHECK(next->context().boundViewpoint() == nullptr);
}

TEST_CASE("Runtime retirement: context and behavior lifetimes independently revoke") {
  auto n = std::make_shared<ScalarDamper>();
  auto system = std::make_shared<DamperSystem<ScalarDamper, float>>();
  {
    X3DExecutionContext context;
    system->attach(n.get(), context);
    context.addSystem(system);
    n->onSet_value(2);
    context.process();
  }
  // System and node both survive; context does not.
  n->onSet_value(6);
  CHECK(n->getValue_changed() == 2);
  {
    X3DExecutionContext context;
    {
      DamperSystem<ScalarDamper, float> temporary;
      temporary.attach(n.get(), context);
      n->onSet_value(4);
      context.process();
    }
    // Node and context survive; a not-yet-registered system does not.
    n->onSet_destination(20);
    n->onSet_value(30);
    context.process();
    CHECK(n->getValue_changed() == 4);
  }
}

TEST_CASE("Runtime retirement: failed session construction revokes partial attachment") {
  auto value = interpolator();
  auto vp = std::make_shared<Viewpoint>();
  auto failure = std::make_shared<FailingAfterInterpolator>(value);
  CHECK_THROWS_AS(sessionFor({vp, value, failure}), std::runtime_error);
  REQUIRE(value->getValue_changed() == 1); // reached the intended attach phase
  vp->onSet_bind(false);
  value->onSet_fraction(1);
  CHECK(value->getValue_changed() == 1);
}

TEST_CASE("Runtime retirement: unwound unregistered filters and listeners are neutral") {
  X3DExecutionContext context;
  auto time = std::make_shared<TimeSensor>();
  auto key = std::make_shared<KeySensor>();
  auto trigger = std::make_shared<IntegerTrigger>();
  {
    TimeSensorSystem timing;
    KeyDeviceSensorSystem keys;
    IntegerTriggerSystem integers;
    timing.attach(time.get(), context);
    keys.attach(key.get(), context);
    integers.attach(trigger.get(), context);
  }
  CHECK(context.writeField(time.get(), "startTime", SFTime{3}) == FieldWriteResult::Ok);
  CHECK(time->getStartTime() == 3);
  CHECK(context.writeField(key.get(), "enabled", true) == FieldWriteResult::Ok);
  CHECK(context.writeField(trigger.get(), "integerKey", SFInt32{7}) == FieldWriteResult::Ok);
  context.process();
}

TEST_CASE("Runtime retirement: reentrant retirement is rejected without changing activation") {
  X3DExecutionContext context;
  EmptySystem system;
  int called = 0;
  auto callback = context.guardCallback(system, [&] {
    ++called;
    CHECK_FALSE(context.retireCallbacks());
  });
  callback();
  callback();
  CHECK(called == 2);
  REQUIRE(context.retireCallbacks());
  callback();
  CHECK(called == 2);
}

TEST_CASE("Runtime retirement: process tick and direct writes reject reentrant teardown") {
  for (int mode = 0; mode < 3; ++mode) {
    X3DExecutionContext context;
    Scene scene;
    auto value = interpolator();
    scene.rootNodes = {value};
    context.buildSceneGraph(scene);
    bool called = false;
    context.addFieldWriteListener([&](const FieldAddress &) {
      called = true;
      CHECK_FALSE(context.retireCallbacks());
    });
    if (mode == 0) {
      CHECK(context.writeField(value.get(), "key", MFFloat{0, 1}) == FieldWriteResult::Ok);
    } else {
      context.postEvent(value.get(), "key", MFFloat{0, 1});
      if (mode == 1) context.process(); else context.tick(1);
    }
    CHECK(called);
    CHECK(context.retireCallbacks());
  }
}

#if defined(__unix__)
TEST_CASE("Runtime retirement: destroying an activation inside its callback fails closed") {
  const pid_t pid = fork();
  REQUIRE(pid >= 0);
  if (pid == 0) {
    std::set_terminate([] { _exit(73); });
    auto context = std::make_unique<X3DExecutionContext>();
    EmptySystem system;
    auto callback = context->guardCallback(system, [&] { context.reset(); });
    callback();
    _exit(74);
  }
  int status = 0;
  REQUIRE(waitpid(pid, &status, 0) == pid);
  REQUIRE(WIFEXITED(status));
  CHECK(WEXITSTATUS(status) == 73);
}
#endif

TEST_CASE("Runtime retirement: every standard node callback family survives teardown") {
  MFNode nodes;
#define RETAIN_NODE(T) nodes.push_back(std::make_shared<T>())
  RETAIN_NODE(ScalarInterpolator);
  RETAIN_NODE(PositionInterpolator);
  RETAIN_NODE(PositionInterpolator2D);
  RETAIN_NODE(ColorInterpolator);
  RETAIN_NODE(OrientationInterpolator);
  RETAIN_NODE(CoordinateInterpolator);
  RETAIN_NODE(CoordinateInterpolator2D);
  RETAIN_NODE(NormalInterpolator);
  RETAIN_NODE(SplinePositionInterpolator);
  RETAIN_NODE(SplinePositionInterpolator2D);
  RETAIN_NODE(SplineScalarInterpolator);
  RETAIN_NODE(SquadOrientationInterpolator);
  RETAIN_NODE(EaseInEaseOut);
  RETAIN_NODE(GeoPositionInterpolator);
  RETAIN_NODE(NurbsPositionInterpolator);
  RETAIN_NODE(NurbsOrientationInterpolator);
  RETAIN_NODE(NurbsSurfaceInterpolator);
  RETAIN_NODE(ScalarDamper); RETAIN_NODE(ScalarChaser);
  RETAIN_NODE(PositionDamper); RETAIN_NODE(PositionChaser);
  RETAIN_NODE(PositionDamper2D); RETAIN_NODE(PositionChaser2D);
  RETAIN_NODE(ColorDamper); RETAIN_NODE(ColorChaser);
  RETAIN_NODE(OrientationDamper); RETAIN_NODE(OrientationChaser);
  RETAIN_NODE(CoordinateDamper); RETAIN_NODE(CoordinateChaser);
  RETAIN_NODE(TexCoordDamper2D); RETAIN_NODE(TexCoordChaser2D);
  RETAIN_NODE(BooleanTrigger); RETAIN_NODE(IntegerTrigger); RETAIN_NODE(TimeTrigger);
  RETAIN_NODE(BooleanFilter); RETAIN_NODE(BooleanToggle);
  RETAIN_NODE(BooleanSequencer); RETAIN_NODE(IntegerSequencer);
  RETAIN_NODE(Viewpoint); RETAIN_NODE(OrthoViewpoint); RETAIN_NODE(GeoViewpoint);
  RETAIN_NODE(Background); RETAIN_NODE(TextureBackground); RETAIN_NODE(Fog);
  RETAIN_NODE(NavigationInfo);
#undef RETAIN_NODE
  auto humanoid = std::make_shared<HAnimHumanoid>();
  auto motion = std::make_shared<HAnimMotion>();
  humanoid->setMotions({motion});
  nodes.push_back(humanoid);
  nodes.push_back(motion);
  sessionFor(nodes).reset();
  std::size_t invoked = 0;
  for (const auto &node : nodes) {
    for (const auto &field : node->fields()) {
      if (field.access != AccessType::InputOnly || !field.set) continue;
      std::any value;
      switch (field.type) {
      case X3DFieldType::SFBool: value = SFBool{true}; break;
      case X3DFieldType::SFFloat: value = SFFloat{0.5f}; break;
      case X3DFieldType::SFTime: value = SFTime{2}; break;
      case X3DFieldType::SFVec2f: value = SFVec2f{1, 2}; break;
      case X3DFieldType::SFVec3f: value = SFVec3f{1, 2, 3}; break;
      case X3DFieldType::SFColor: value = SFColor{1, 0, 0}; break;
      case X3DFieldType::SFRotation: value = SFRotation{0, 1, 0, 1}; break;
      case X3DFieldType::MFVec2f: value = MFVec2f{{1, 2}}; break;
      case X3DFieldType::MFVec3f: value = MFVec3f{{1, 2, 3}}; break;
      default: continue;
      }
      CAPTURE(node->nodeTypeName());
      CAPTURE(field.x3dName);
      CHECK_NOTHROW(field.set(*node, value));
      ++invoked;
    }
  }
  CHECK(invoked >= 60);
}

TEST_CASE("Runtime retirement: throwing callbacks release their in-flight lease") {
  X3DExecutionContext context;
  EmptySystem system;
  auto callback = context.guardCallback(system, [] {
    throw std::runtime_error("callback failed");
  });
  CHECK_THROWS_AS(callback(), std::runtime_error);
  CHECK(context.retireCallbacks());
  CHECK_NOTHROW(callback());
}

TEST_CASE("Runtime retirement: low-level teardown never traverses released node storage") {
  auto context = std::make_unique<X3DExecutionContext>();
  std::weak_ptr<Viewpoint> weakViewpoint;
  std::weak_ptr<ScalarDamper> weakFollower;
  {
    Scene scene;
    auto viewpoint = std::make_shared<Viewpoint>();
    auto follower = std::make_shared<ScalarDamper>();
    weakViewpoint = viewpoint;
    weakFollower = follower;
    scene.rootNodes = {viewpoint, follower};
    context->buildSceneGraph(scene);
    attachStandardRuntime(scene, *context);
  }
  // No further runtime calls are permitted once low-level scene storage dies.
  // Teardown itself needs no node traversal and the weak leases retain neither.
  CHECK(weakViewpoint.expired());
  CHECK(weakFollower.expired());
  CHECK_NOTHROW(context.reset());
}

namespace {
struct CallOnDestruction {
  explicit CallOnDestruction(std::function<void()> action)
      : action(std::move(action)) {}
  ~CallOnDestruction() { action(); }
  std::function<void()> action;
};
}

TEST_CASE("Runtime retirement: standalone binding captures cannot reenter destroyed state") {
  // Each public callback slot may own a capture with a user-defined destructor.
  // All three are destroyed after the binding maps; destructor-body retirement
  // must therefore precede every member, not just the lifetime-token member.
  for (int slot = 0; slot < 3; ++slot) {
    CAPTURE(slot);
    Scene scene;
    auto viewpoint = std::make_shared<Viewpoint>();
    scene.rootNodes = {viewpoint};
    int captureDestructions = 0, posted = 0;
    auto bindings = std::make_unique<BindingSystem>();
    auto capture = std::make_shared<CallOnDestruction>([&] {
      ++captureDestructions;
      viewpoint->onSet_bind(true);
    });
    BindingSystem::Poster poster = [&](X3DNode *, const std::string &, std::any) { ++posted; };
    BindingSystem::Clock clock = [] { return 0.0; };
    BindingSystem::TransitionSink sink;
    if (slot == 0) sink = [capture](BindTransition) {};
    if (slot == 1) clock = [capture] { return 0.0; };
    if (slot == 2)
      poster = [capture, &posted](X3DNode *, const std::string &, std::any) { ++posted; };
    bindings->enrollScene(scene, std::move(poster), std::move(clock), std::move(sink));
    bindings->bindDefaults();
    capture.reset(); // BindingSystem now holds the only capture owner.
    bindings.reset();
    CHECK(captureDestructions == 1);
    CHECK(posted == 0);
    viewpoint->onSet_bind(false);
    CHECK(posted == 0);
  }
}

TEST_CASE("Runtime retirement: interpolator callable destructors run after revocation") {
  X3DExecutionContext context;
  bool armed = false;
  int callbackDestructions = 0;
  auto scalar = interpolator();
  auto scalarLerp = [onDestroy = CallOnDestruction([&] {
    if (armed) {
      ++callbackDestructions;
      scalar->onSet_fraction(0.75f);
    }
  })](const float &a, const float &b, float t) { return a + (b - a) * t; };
  auto single = std::make_unique<InterpolatorSystem<ScalarInterpolator, float>>(scalarLerp);
  single->attach(scalar.get(), context);
  armed = true;
  single.reset(); // lerp_ capture destruction must not invoke the live context
  armed = false;
  context.process();
  CHECK(callbackDestructions == 1);
  CHECK(scalar->getValue_changed() == 1);

  auto coordinates = std::make_shared<CoordinateInterpolator>();
  coordinates->setKey({0, 1});
  coordinates->setKeyValue({SFVec3f{1, 0, 0}, SFVec3f{9, 0, 0}});
  auto vectorLerp = [onDestroy = CallOnDestruction([&] {
    if (armed) {
      ++callbackDestructions;
      coordinates->onSet_fraction(0.75f);
    }
  })](const SFVec3f &a, const SFVec3f &b, float t) {
    return SFVec3f{a.x + (b.x - a.x) * t, 0, 0};
  };
  auto multi = std::make_unique<MultiInterpolatorSystem<CoordinateInterpolator, SFVec3f>>(vectorLerp);
  multi->attach(coordinates.get(), context);
  armed = true;
  multi.reset();
  armed = false;
  context.process();
  CHECK(callbackDestructions == 2);
  REQUIRE(coordinates->getValue_changed().size() == 1);
  CHECK(coordinates->getValue_changed().front().x == 1);
}

TEST_CASE("Runtime retirement: custom most-derived owner retires before member callbacks") {
  class CustomSystem : public System {
  public:
    CustomSystem(std::function<void()> onDestroy, int &handled)
        : handled_(handled), capture_(std::move(onDestroy)) {}
    ~CustomSystem() override { retireCallbacksBeforeDestruction(); }
    void attach(X3DNode *node, X3DExecutionContext &context) override {
      dynamic_cast<ScalarDamper *>(node)->setOnSet_valueHandler(
          context.guardCallback(*this, [this](const float &) { ++handled_; }));
    }
  private:
    int &handled_;
    CallOnDestruction capture_;
  };
  X3DExecutionContext context;
  auto node = std::make_shared<ScalarDamper>();
  int captureDestructions = 0, handled = 0;
  auto system = std::make_unique<CustomSystem>([&] {
    ++captureDestructions;
    node->onSet_value(5);
  }, handled);
  system->attach(node.get(), context);
  node->onSet_value(2);
  CHECK(handled == 1);
  system.reset();
  CHECK(captureDestructions == 1);
  CHECK(handled == 1);
  CHECK_NOTHROW(node->onSet_value(7));
  CHECK(handled == 1);
}

#if defined(__unix__)
TEST_CASE("Runtime retirement: standalone owner deletion inside its handler fails closed") {
  for (int kind = 0; kind < 2; ++kind) {
    const pid_t pid = fork();
    REQUIRE(pid >= 0);
    if (pid == 0) {
      std::set_terminate([] { _exit(73); });
      if (kind == 0) {
        Scene scene;
        auto node = std::make_shared<Viewpoint>();
        scene.rootNodes = {node};
        auto owner = std::make_unique<BindingSystem>();
        owner->enrollScene(scene, [](X3DNode *, const std::string &, std::any) {},
                           [] { return 0.0; }, [&](BindTransition) { owner.reset(); });
        node->onSet_bind(true);
      } else {
        X3DExecutionContext context;
        auto node = interpolator();
        using Owner = InterpolatorSystem<ScalarInterpolator, float>;
        std::unique_ptr<Owner> owner;
        owner = std::make_unique<Owner>([&](const float &, const float &, float) {
          owner.reset();
          return 0.0f;
        });
        owner->attach(node.get(), context);
        node->onSet_fraction(0.5f);
      }
      _exit(74);
    }
    int status = 0;
    REQUIRE(waitpid(pid, &status, 0) == pid);
    REQUIRE(WIFEXITED(status));
    CHECK(WEXITSTATUS(status) == 73);
  }
}
#endif
