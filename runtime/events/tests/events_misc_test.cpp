#include "doctest/doctest.h"
#include "scene_test_helpers.hpp"
// events_misc_test.cpp — regression tests for small event-model gaps:
//   §30.4.6 an integerKey write re-emits triggerValue (same value too)
//   §21.2   enabling a key device sensor disables the others
//   §8.2.4.4 pauseTime/resumeTime_changed carry simulation-now
//   §8.2.4.3 set_stopTime == set_startTime restarts an active node
//   §9.4.3  an Inline with a failed nested sub-Inline is not loaded
//   Anchor        §9.4.1  a click binds a "#Name" viewpoint / calls the handler

#include "AnchorSystem.hpp"
#include "EventUtilitySystem.hpp"
#include "KeyDeviceSensorSystem.hpp"
#include "LoadSensorSystem.hpp"
#include "TimeSensorSystem.hpp"
#include "X3DDocument.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DSceneBridge.hpp"
#include "x3d/nodes/X3DNodeFactory.hpp"

#include "x3d/nodes/IntegerTrigger.hpp"
#include "x3d/nodes/KeySensor.hpp"
#include "x3d/nodes/LoadSensor.hpp"
#include "x3d/nodes/StringSensor.hpp"
#include "x3d/nodes/TimeSensor.hpp"
#include "x3d/nodes/Viewpoint.hpp"
#include "x3d/nodes/ScalarInterpolator.hpp"

#include <any>
#include <cmath>
#include <memory>
#include <new>
#include <string>
#include <vector>

using namespace x3d;
using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;
using x3d::test::setF;
using x3d::test::addChild;

namespace {
bool deq(double a, double b, double e = 1e-6) { return std::fabs(a - b) <= e; }

using NodeP = std::shared_ptr<X3DNode>;

void test_integer_trigger_key_write() {
  auto trig = std::make_shared<IntegerTrigger>();
  X3DExecutionContext ctx;
  auto sys = std::make_shared<IntegerTriggerSystem>();
  ctx.addSystem(sys);
  sys->attach(trig.get(), ctx);
  int triggerEvents = 0;
  ctx.addFieldWriteListener([&](const FieldAddress &a) {
    if (a.node == trig.get() && a.field == "triggerValue") ++triggerEvents;
  });
  ctx.postEvent(trig.get(), "integerKey", std::any(SFInt32{5}));
  ctx.tick(0.1);
  CHECK_MESSAGE((trig->getTriggerValue() == 5 && triggerEvents == 1), ("TRIG-4: an integerKey write emits triggerValue with that value"));
  ctx.postEvent(trig.get(), "integerKey", std::any(SFInt32{5}));
  ctx.tick(0.2);
  CHECK_MESSAGE((triggerEvents == 2), ("TRIG-4: resetting integerKey to the same value emits again"));
  ctx.postEvent(trig.get(), "set_boolean", std::any(SFBool{true}));
  ctx.tick(0.3);
  CHECK_MESSAGE((triggerEvents == 3), ("TRIG-4: set_boolean TRUE still emits triggerValue"));
}

// LIFETIME: the IntegerTrigger field-write listener must be disconnected when
// the node is detached (detachNodes is the last gate before the caller drops
// the shared_ptr), and a detach -> attachNewSubtree cycle must not stack
// duplicate listeners (double-emit / unbounded growth on GeoLOD churn).
void test_integer_trigger_listener_lifetime() {
  auto trig = std::make_shared<IntegerTrigger>();
  X3DExecutionContext ctx;
  auto sys = std::make_shared<IntegerTriggerSystem>();
  ctx.addSystem(sys);
  sys->attach(trig.get(), ctx);
  sys->attach(trig.get(), ctx); // double attach must not double-emit
  int triggerEvents = 0;
  ctx.addFieldWriteListener([&](const FieldAddress &a) {
    if (a.node == trig.get() && a.field == "triggerValue") ++triggerEvents;
  });
  ctx.postEvent(trig.get(), "integerKey", std::any(SFInt32{7}));
  ctx.tick(0.1);
  CHECK_MESSAGE((triggerEvents == 1), ("LIFETIME: double attach does not duplicate the integerKey listener"));

  ctx.detachNodes({trig.get()});
  ctx.postEvent(trig.get(), "integerKey", std::any(SFInt32{9}));
  ctx.tick(0.2);
  CHECK_MESSAGE((triggerEvents == 1 && trig->getTriggerValue() == 7), ("LIFETIME: detached node's field-write listener is disconnected"));

  // Re-attach (as InlineRuntimeSystem does on topology churn): exactly one
  // listener again, not the old one plus a new one.
  ctx.attachNewSubtree(trig.get());
  ctx.postEvent(trig.get(), "integerKey", std::any(SFInt32{11}));
  ctx.tick(0.3);
  CHECK_MESSAGE((triggerEvents == 2 && trig->getTriggerValue() == 11), ("LIFETIME: re-attach reinstalls exactly one listener"));
}

void test_key_device_focus() {
  auto k1 = std::make_shared<KeySensor>();
  auto k2 = std::make_shared<KeySensor>();
  auto s1 = std::make_shared<StringSensor>();
  X3DExecutionContext ctx;
  auto sys = std::make_shared<KeyDeviceSensorSystem>();
  ctx.addSystem(sys);
  for (X3DNode *n : std::vector<X3DNode *>{k1.get(), k2.get(), s1.get()}) sys->attach(n, ctx);
  ctx.postEvent(k1.get(), "enabled", std::any(SFBool{true}));
  ctx.tick(0.1);
  CHECK_MESSAGE((k1->getEnabled()), ("KDS-6: the newly enabled sensor stays enabled"));
  CHECK_MESSAGE((!k2->getEnabled() && !s1->getEnabled()), ("KDS-6: enabling one key device sensor disables the others"));
  ctx.postEvent(k2.get(), "enabled", std::any(SFBool{false}));
  ctx.tick(0.2);
  CHECK_MESSAGE((k1->getEnabled()), ("KDS-6: a disable event does not steal focus"));
}

struct TimeRig {
  std::shared_ptr<TimeSensor> ts = std::make_shared<TimeSensor>();
  X3DExecutionContext ctx;
  std::shared_ptr<TimeSensorSystem> sys = std::make_shared<TimeSensorSystem>();
  TimeRig() { ctx.addSystem(sys); sys->attach(ts.get(), ctx); }
};

void test_pause_resume_now() {
  TimeRig r;
  r.ts->setCycleInterval(10.0);
  r.ts->setLoop(true);
  r.ctx.tick(0.0);
  r.ts->setPauseTime(2.0);
  r.ctx.tick(2.3); // the pause is recognised at this tick
  CHECK_MESSAGE((deq(r.ts->getPauseTime(), 2.3)), ("CONF-TDN1V: pauseTime_changed carries simulation now"));
  r.ts->setResumeTime(4.0);
  r.ctx.tick(4.4);
  CHECK_MESSAGE((deq(r.ts->getResumeTime(), 4.4)), ("CONF-TDN1V: resumeTime_changed carries simulation now"));
}

void test_same_instant_restart() {
  TimeRig r;
  r.ts->setCycleInterval(10.0);
  r.ctx.tick(0.0); // active from 0, no loop
  r.ctx.tick(5.0);
  CHECK_MESSAGE((r.ts->X3DTimeDependentNode::getIsActive()), ("CRITIC-1: active before the restart"));
  int isActiveEvents = 0;
  r.ctx.addFieldWriteListener([&](const FieldAddress &a) {
    if (a.node == r.ts.get() && a.field == "isActive") ++isActiveEvents;
  });
  // set_stopTime then set_startTime at the current time, as a ROUTE would.
  r.ctx.postEvent(r.ts.get(), "stopTime", std::any(SFTime{5.0}));
  r.ctx.postEvent(r.ts.get(), "startTime", std::any(SFTime{5.0}));
  r.ctx.tick(5.0);
  r.ctx.tick(6.0);
  CHECK_MESSAGE((r.ts->X3DTimeDependentNode::getIsActive()), ("CRITIC-1: the node is restarted, still active"));
  CHECK_MESSAGE((isActiveEvents == 0), ("CRITIC-1: no isActive FALSE/TRUE toggle for an in-place restart"));
  CHECK_MESSAGE((deq(r.ts->getElapsedTime(), 1.0, 1e-4)), ("CRITIC-1: elapsedTime restarts from the new startTime"));
  CHECK_MESSAGE((deq(r.ts->getFraction_changed(), 0.1, 1e-4)), ("CRITIC-1: fraction restarts from 0"));
  r.ctx.tick(16.0);
  CHECK_MESSAGE((!r.ts->X3DTimeDependentNode::getIsActive()), ("CRITIC-1: the restarted run completes a full cycle"));
}

void test_nested_inline_readiness() {
  // An expanded Inline (a Group recorded in expandedInlines) whose content still
  // holds an un-expanded Inline (load `TRUE`): its nested sub-Inline failed.
  auto good = createX3DNode("Group");
  auto bad = createX3DNode("Group");
  auto nested = createX3DNode("Inline");
  setF(nested, "url", std::any(MFString{"missing.x3d"}));
  addChild(bad, nested);
  auto sensorOk = createX3DNode("LoadSensor");
  auto sensorBad = createX3DNode("LoadSensor");
  setF(sensorOk, "children", std::any(std::vector<NodeP>{good}));
  setF(sensorBad, "children", std::any(std::vector<NodeP>{bad}));
  Scene scene;
  scene.addRootNode(sensorOk);
  scene.addRootNode(sensorBad);
  scene.expandedInlines[good.get()] = createX3DNode("Inline");
  scene.expandedInlines[bad.get()] = createX3DNode("Inline");
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  auto sys = std::make_shared<LoadSensorSystem>();
  sys->setScene(&scene);
  sys->attach(sensorOk.get(), ctx);
  sys->attach(sensorBad.get(), ctx);
  ctx.addSystem(sys);
  ctx.tick(0.1);
  ctx.tick(0.2);
  CHECK_MESSAGE((dynamic_cast<LoadSensor &>(*sensorOk).getIsLoaded()), ("NSN-12: an expanded Inline with no pending sub-Inline is loaded"));
  CHECK_MESSAGE((!dynamic_cast<LoadSensor &>(*sensorBad).getIsLoaded()),
        "NSN-12: an Inline whose nested sub-Inline failed is not loaded");
}

void test_anchor_activation() {
  auto box = createX3DNode("Box");
  auto shape = createX3DNode("Shape");
  setF(shape, "geometry", std::any(NodeP(box)));
  auto toVp = createX3DNode("Anchor");
  setF(toVp, "url", std::any(MFString{"#Far"}));
  addChild(toVp, shape);
  auto vpNear = createX3DNode("Viewpoint");
  setF(vpNear, "position", std::any(SFVec3f{0, 0, 10}));
  auto vpFar = createX3DNode("Viewpoint");
  vpFar->setDEF("Far");
  setF(vpFar, "position", std::any(SFVec3f{0, 0, 50}));
  Scene scene;
  scene.addRootNode(vpNear); // first viewpoint: bound by default
  scene.addRootNode(toVp);
  scene.addRootNode(vpFar);
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  auto nav = attachInteractive(scene, ctx);
  (void)nav;
  CHECK_MESSAGE((ctx.boundViewpoint() == vpNear.get()), ("Anchor: the first viewpoint starts bound"));
  const Ray atBox{{0, 0, 10}, {0, 0, -1}};
  ctx.setPointerPresent(true);
  ctx.setPointer(atBox);
  ctx.setPointerButton(true);
  ctx.tick(0.1);
  ctx.setPointerButton(false);
  ctx.tick(0.2);
  ctx.tick(0.3);
  CHECK_MESSAGE((ctx.boundViewpoint() == vpFar.get()), ("Anchor: a click on '#Far' binds that viewpoint"));

  // A non-fragment url goes to the embedder's handler with url + parameter.
  auto box2 = createX3DNode("Box");
  auto shape2 = createX3DNode("Shape");
  setF(shape2, "geometry", std::any(NodeP(box2)));
  auto link = createX3DNode("Anchor");
  setF(link, "url", std::any(MFString{"other.x3d"}));
  setF(link, "parameter", std::any(MFString{"target=_blank"}));
  addChild(link, shape2);
  Scene s2;
  s2.addRootNode(link);
  X3DExecutionContext c2;
  c2.buildSceneGraph(s2);
  auto anchors = std::make_shared<AnchorSystem>();
  anchors->attach(link.get(), c2);
  c2.addSystem(anchors);
  MFString gotUrl, gotParam;
  anchors->setAnchorHandler([&](X3DNode *, const MFString &u, const MFString &p) {
    gotUrl = u; gotParam = p;
  });
  c2.setPointerPresent(true);
  c2.setPointer(atBox);
  c2.setPointerButton(true);
  c2.tick(0.1);
  c2.setPointerButton(false);
  c2.tick(0.2);
  CHECK_MESSAGE((gotUrl == MFString{"other.x3d"} && gotParam == MFString{"target=_blank"}), ("Anchor: a non-fragment url is handed to the embedder handler"));

  // Press on the Anchor, release elsewhere: no activation.
  gotUrl.clear();
  c2.setPointer(atBox);
  c2.setPointerButton(true);
  c2.tick(0.3);
  c2.setPointer(Ray{{50, 50, 10}, {0, 0, -1}});
  c2.setPointerButton(false);
  c2.tick(0.4);
  CHECK_MESSAGE((gotUrl.empty()), ("Anchor: releasing off the Anchor does not activate it"));
}

void test_anchor_loadsensor_waits_for_viewpoint_bind() {
  auto anchor = createX3DNode("Anchor");
  setF(anchor, "url", std::any(MFString{"#Target"}));
  setF(anchor, "load", std::any(SFBool{false})); // §9.4.1: no effect on Anchor
  auto vp = createX3DNode("Viewpoint");
  vp->setDEF("Target");
  auto firstVp = createX3DNode("Viewpoint");
  auto sensor = createX3DNode("LoadSensor");
  setF(sensor, "children", std::any(std::vector<NodeP>{anchor}));
  Scene scene;
  scene.addRootNode(firstVp);
  scene.addRootNode(anchor);
  scene.addRootNode(vp);
  scene.addRootNode(sensor);
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  CHECK_MESSAGE((ctx.boundViewpoint() == firstVp.get()), ("Anchor target starts unbound"));
  auto sys = std::make_shared<LoadSensorSystem>();
  sys->setScene(&scene);
  sys->attach(sensor.get(), ctx);
  ctx.addSystem(sys);
  ctx.tick(0.1);
  CHECK_MESSAGE((!dynamic_cast<LoadSensor &>(*sensor).getIsLoaded()), ("Anchor target existing but unbound does not count as loaded"));
  ctx.postEvent(vp.get(), "set_bind", std::any(SFBool{true}));
  ctx.tick(0.2);
  ctx.tick(0.3);
  CHECK_MESSAGE((dynamic_cast<LoadSensor &>(*sensor).getIsLoaded()), ("Anchor target counts as loaded after binding, even with load FALSE"));
}

} // namespace

TEST_CASE("events_misc_test") {
  test_integer_trigger_key_write();
  test_integer_trigger_listener_lifetime();
  test_key_device_focus();
  test_pause_resume_now();
  test_same_instant_restart();
  test_nested_inline_readiness();
  test_anchor_activation();
  test_anchor_loadsensor_waits_for_viewpoint_bind();
}

TEST_CASE("structural removal retires listeners before node address reuse") {
  for (const std::string edit : {"removeChildren", "children", "proxy", "directChildren", "directProxy"}) {
    CAPTURE(edit);
    alignas(IntegerTrigger) unsigned char storage[sizeof(IntegerTrigger)];
    auto construct = [&] {
      return std::shared_ptr<IntegerTrigger>(new (storage) IntegerTrigger,
          [](IntegerTrigger *p) { p->~IntegerTrigger(); });
    };
    auto original = construct();
    std::weak_ptr<IntegerTrigger> weak = original;
    const bool proxy = edit == "proxy" || edit == "directProxy";
    auto root = createX3DNode(proxy ? "Collision" : "Group");
    auto subtree = createX3DNode("Group");
    addChild(subtree, original);
    if (proxy) setF(root, "proxy", SFNode(subtree));
    else addChild(root, subtree);
    Scene scene;
    scene.rootNodes = {root};
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    auto system = std::make_shared<IntegerTriggerSystem>();
    ctx.addSystem(system);
    system->attach(original.get(), ctx);
    int emitted = 0;
    ctx.addFieldWriteListener([&](const FieldAddress &a) {
      if (a.field == "triggerValue") ++emitted;
    });
    if (edit == "directChildren")
      REQUIRE(ctx.writeField(root.get(), "children", MFNode{}) == FieldWriteResult::Ok);
    else if (edit == "directProxy")
      REQUIRE(ctx.writeField(root.get(), "proxy", SFNode{}) == FieldWriteResult::Ok);
    else if (proxy) ctx.postEvent(root.get(), "proxy", SFNode{});
    else ctx.postEvent(root.get(), edit, edit == "removeChildren" ? MFNode{subtree} : MFNode{});
    ctx.tick(1);
    subtree.reset();
    original.reset();
    REQUIRE(weak.expired());
    auto replacement = construct();
    ctx.postEvent(replacement.get(), "integerKey", SFInt32{9});
    ctx.tick(2);
    CHECK(emitted == 0);
  }
}

TEST_CASE("structural edits preserve shared nodes and attach newly reachable systems") {
  auto root = createX3DNode("Group");
  auto left = createX3DNode("Group");
  auto right = createX3DNode("Group");
  auto trigger = std::make_shared<IntegerTrigger>();
  addChild(left, trigger);
  addChild(right, trigger);
  addChild(root, left);
  addChild(root, right);
  Scene scene;
  scene.rootNodes = {root};
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  attachStandardRuntime(scene, ctx);
  int emitted = 0;
  ctx.addFieldWriteListener([&](const FieldAddress &a) {
    if (a.field == "triggerValue") ++emitted;
  });
  ctx.postEvent(left.get(), "removeChildren", MFNode{trigger});
  ctx.postEvent(trigger.get(), "integerKey", SFInt32{7});
  ctx.tick(1);
  CHECK(emitted == 1);
  ctx.postEvent(right.get(), "children", MFNode{});
  ctx.tick(2);
  ctx.postEvent(trigger.get(), "integerKey", SFInt32{8});
  ctx.tick(3);
  CHECK(emitted == 1);

  auto subtree = createX3DNode("Group");
  auto clock = std::make_shared<TimeSensor>();
  clock->setLoop(true);
  auto value = std::make_shared<ScalarInterpolator>();
  value->setKey({0, 1});
  value->setKeyValue({2, 10});
  addChild(subtree, clock);
  addChild(subtree, value);
  addChild(subtree, trigger);
  ctx.postEvent(root.get(), "addChildren", MFNode{subtree});
  ctx.tick(4);
  ctx.postEvent(value.get(), "set_fraction", SFFloat{0.5f});
  ctx.postEvent(trigger.get(), "integerKey", SFInt32{9});
  ctx.tick(4.5);
  CHECK(clock->X3DTimeDependentNode::getIsActive());
  CHECK(value->getValue_changed() == doctest::Approx(6));
  CHECK(emitted == 2);
  std::weak_ptr<TimeSensor> weak = clock;
  clock.reset();
  subtree.reset();
  ctx.postEvent(root.get(), "children", MFNode{});
  ctx.tick(5);
  CHECK(weak.expired());
  ctx.tick(6);
}

TEST_CASE("structural removal detaches a subtree before its last owner is released") {
  for (const bool direct : {false, true}) {
    CAPTURE(direct);
    auto root = createX3DNode("Group");
    auto clock = std::make_shared<TimeSensor>();
    clock->setLoop(true);
    addChild(root, clock);
    std::weak_ptr<X3DNode> weak = clock;
    Scene scene;
    scene.rootNodes = {root};
    X3DExecutionContext ctx;
    ctx.buildSceneGraph(scene);
    attachStandardRuntime(scene, ctx);
    class Observer : public System {
    public:
      std::weak_ptr<X3DNode> watched;
      bool detachedWhileAlive = false;
      void attach(X3DNode *, X3DExecutionContext &) override {}
      void detach(X3DNode *node, X3DExecutionContext &) override {
        if (auto target = watched.lock(); target && target.get() == node)
          detachedWhileAlive = true;
      }
    };
    auto observer = std::make_shared<Observer>();
    observer->watched = clock;
    ctx.addSystem(observer);
    clock.reset();
    if (direct)
      REQUIRE(ctx.writeField(root.get(), "children", MFNode{}) == FieldWriteResult::Ok);
    else ctx.postEvent(root.get(), "children", MFNode{});
    ctx.tick(1);
    CHECK(observer->detachedWhileAlive);
    CHECK(weak.expired());
    ctx.tick(2);
  }
}
