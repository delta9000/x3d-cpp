#include "doctest/doctest.h"
// Keyboard-driven sensor outputs and enabled gating (§21).

#include "KeyDeviceSensorSystem.hpp"
#include "X3DExecutionContext.hpp"
#include "X3DSceneBridge.hpp"

#include "x3d/nodes/KeySensor.hpp"
#include "x3d/nodes/StringSensor.hpp"
#include "x3d/nodes/TimeSensor.hpp"
#include "x3d/nodes/WorldInfo.hpp"

#include "X3DFieldAddress.hpp"

#include "X3DDocument.hpp" // out-of-line Scene::addRootNode
#include "x3d/nodes/X3DNodeFactory.hpp"

#include <memory>
#include <string>

using namespace x3d;
using namespace x3d::runtime;
using namespace x3d::core;
using namespace x3d::nodes;

namespace {
// Attach a system to a node + register it, then drive one tick after pushing
// input. tick() runs the System's update (which seeds events) then drains the
// cascade (writing the output fields).
template <class NodeT>
std::shared_ptr<KeyDeviceSensorSystem> wire(X3DExecutionContext &ctx, NodeT *n) {
  auto sys = std::make_shared<KeyDeviceSensorSystem>();
  sys->attach(n, ctx);
  ctx.addSystem(sys);
  return sys;
}

// KeySensor character + isActive
void test_keysensor_character() {
  auto n = std::make_shared<KeySensor>();
  X3DExecutionContext ctx;
  wire(ctx, n.get());

  ctx.pushKeyCharacter("a", /*down=*/true);
  ctx.tick(1.0);
  CHECK_MESSAGE((n->getKeyPress() == "a"), ("KeySensor key down -> keyPress='a'"));
  CHECK_MESSAGE((n->getIsActive() == true), ("KeySensor key down -> isActive=TRUE"));

  ctx.pushKeyCharacter("a", /*down=*/false);
  ctx.tick(2.0);
  CHECK_MESSAGE((n->getKeyRelease() == "a"), ("KeySensor key up -> keyRelease='a'"));
  CHECK_MESSAGE((n->getIsActive() == false), ("KeySensor key up -> isActive=FALSE"));
}

// action keys (Table 21.2)
void test_keysensor_action_key() {
  auto n = std::make_shared<KeySensor>();
  X3DExecutionContext ctx;
  wire(ctx, n.get());

  ctx.pushActionKey(17, /*down=*/true); // up = 17
  ctx.tick(1.0);
  CHECK_MESSAGE((n->getActionKeyPress() == 17), ("KeySensor action down -> actionKeyPress=17 (UP)"));
  ctx.pushActionKey(17, /*down=*/false);
  ctx.tick(2.0);
  CHECK_MESSAGE((n->getActionKeyRelease() == 17), ("KeySensor action up -> actionKeyRelease=17"));
}

// modifier keys
void test_keysensor_modifiers() {
  auto n = std::make_shared<KeySensor>();
  X3DExecutionContext ctx;
  wire(ctx, n.get());

  ctx.pushModifierKey(1, /*down=*/true); // 1=shift
  ctx.tick(1.0);
  CHECK_MESSAGE((n->getShiftKey() == true), ("KeySensor shift down -> shiftKey=TRUE"));
  ctx.pushModifierKey(1, /*down=*/false);
  ctx.tick(2.0);
  CHECK_MESSAGE((n->getShiftKey() == false), ("KeySensor shift up -> shiftKey=FALSE"));
  ctx.pushModifierKey(2, /*down=*/true); // 2=control
  ctx.tick(3.0);
  CHECK_MESSAGE((n->getControlKey() == true), ("KeySensor control down -> controlKey=TRUE"));
  ctx.pushModifierKey(3, /*down=*/true); // 3=alt
  ctx.tick(4.0);
  CHECK_MESSAGE((n->getAltKey() == true), ("KeySensor alt down -> altKey=TRUE"));
}

// enabled gate
void test_keysensor_disabled() {
  auto n = std::make_shared<KeySensor>();
  n->setEnabled(SFBool{false});
  n->emitKeyPress(SFString{"sentinel"});
  X3DExecutionContext ctx;
  wire(ctx, n.get());
  ctx.pushKeyCharacter("a", true);
  ctx.tick(1.0);
  CHECK_MESSAGE((n->getKeyPress() == "sentinel"), ("KeySensor enabled=FALSE -> no keyPress"));
}

// StringSensor accumulate + isActive
void test_stringsensor_accumulate() {
  auto n = std::make_shared<StringSensor>();
  X3DExecutionContext ctx;
  wire(ctx, n.get());

  ctx.pushKeyCharacter("h", true);
  ctx.tick(1.0);
  CHECK_MESSAGE((n->getIsActive() == true), ("StringSensor first char -> isActive=TRUE"));
  CHECK_MESSAGE((n->getEnteredText() == "h"), ("StringSensor -> enteredText='h'"));
  ctx.pushKeyCharacter("i", true);
  ctx.tick(2.0);
  CHECK_MESSAGE((n->getEnteredText() == "hi"), ("StringSensor accumulates -> enteredText='hi'"));
}

// StringSensor terminator (finalText + reset + isActive `FALSE`)
void test_stringsensor_terminate() {
  auto n = std::make_shared<StringSensor>();
  X3DExecutionContext ctx;
  wire(ctx, n.get());

  ctx.pushKeyCharacter("h", true);
  ctx.pushKeyCharacter("i", true);
  ctx.tick(1.0);
  ctx.pushStringTerminator();
  ctx.tick(2.0);
  CHECK_MESSAGE((n->getFinalText() == "hi"), ("StringSensor terminator -> finalText='hi'"));
  CHECK_MESSAGE((n->getIsActive() == false), ("StringSensor terminator -> isActive=FALSE"));
  // After termination the accumulator resets: a new char starts fresh.
  ctx.pushKeyCharacter("x", true);
  ctx.tick(3.0);
  CHECK_MESSAGE((n->getEnteredText() == "x"), ("StringSensor resets after finalText -> enteredText='x'"));
}

// StringSensor deletion (deletionAllowed)
void test_stringsensor_deletion() {
  auto n = std::make_shared<StringSensor>(); // deletionAllowed defaults `TRUE`
  X3DExecutionContext ctx;
  wire(ctx, n.get());
  ctx.pushKeyCharacter("a", true);
  ctx.pushKeyCharacter("b", true);
  ctx.pushStringDeletion();
  ctx.tick(1.0);
  CHECK_MESSAGE((n->getEnteredText() == "a"), ("StringSensor deletion removes last char -> 'a'"));

  auto m = std::make_shared<StringSensor>();
  m->setDeletionAllowed(SFBool{false});
  X3DExecutionContext ctx2;
  wire(ctx2, m.get());
  ctx2.pushKeyCharacter("a", true);
  ctx2.pushStringDeletion();
  ctx2.tick(1.0);
  CHECK_MESSAGE((m->getEnteredText() == "a"), ("StringSensor deletionAllowed=FALSE ignores deletion -> 'a'"));
}

// batched tick: a field's ROUTE gets the last value (last-wins coalesce)
// Two same-field events in one tick must produce one net event; the field and
// its ROUTE consumers must agree on the last value (§4.4.8.3 one-event-per-field-
// per-timestamp). Verified through a ROUTE because the field itself is last-wins
// either way — only the routed fan-out exposes the first-vs-last bug.
void test_keysensor_batched_route_last_wins() {
  auto ks = std::make_shared<KeySensor>();
  auto sink = std::make_shared<TimeSensor>(); // enabled (SFBool inputOutput) default `TRUE`
  X3DExecutionContext ctx;
  wire(ctx, ks.get());
  ctx.addRoute(FieldAddress{ks.get(), "isActive"}, FieldAddress{sink.get(), "enabled"});
  // Press then release 'a' in one tick: isActive `TRUE` then `FALSE` -> net `FALSE`.
  ctx.pushKeyCharacter("a", true);
  ctx.pushKeyCharacter("a", false);
  ctx.tick(1.0);
  CHECK_MESSAGE((sink->getEnabled() == false), ("KeySensor batched tick: isActive ROUTE gets the LAST value (FALSE), not the first"));
}

// production wiring: attachKeyDeviceSensors
void test_attach_key_device_sensors() {
  auto ks = createX3DNode("KeySensor");
  Scene scene;
  scene.addRootNode(ks);
  X3DExecutionContext ctx;
  ctx.buildSceneGraph(scene);
  attachKeyDeviceSensors(scene, ctx); // production wiring — no manual attach
  ctx.pushKeyCharacter("z", true);
  ctx.tick(1.0);
  SFString kp;
  for (auto &f : ks->fields())
    if (f.x3dName == "keyPress" && f.get) kp = std::any_cast<SFString>(f.get(*ks));
  CHECK_MESSAGE((kp == "z"), ("attachKeyDeviceSensors wires KeySensor (keyPress='z')"));
}

} // namespace

TEST_CASE("key_device_sensor_test") {
  test_keysensor_character();
  test_keysensor_action_key();
  test_keysensor_modifiers();
  test_keysensor_disabled();
  test_stringsensor_accumulate();
  test_stringsensor_terminate();
  test_stringsensor_deletion();
  test_keysensor_batched_route_last_wins();
  test_attach_key_device_sensors();

  return;
}

TEST_CASE("StringSensor deletion removes one UTF-8 character") {
  auto sensor = std::make_shared<StringSensor>();
  X3DExecutionContext ctx;
  wire(ctx, sensor.get());
  ctx.pushKeyCharacter("a", true);
  ctx.pushKeyCharacter("\xC3\xA9", true); // U+00E9, encoded in two bytes
  ctx.tick(1.0);
  REQUIRE(sensor->getEnteredText() == "a\xC3\xA9");
  ctx.pushStringDeletion();
  ctx.tick(2.0);
  CHECK(sensor->getEnteredText() == "a");
}

TEST_CASE("StringSensor enteredText field resets silently at termination") {
  auto sensor = std::make_shared<StringSensor>();
  auto sink = std::make_shared<WorldInfo>();
  X3DExecutionContext ctx;
  wire(ctx, sensor.get());
  ctx.addRoute(FieldAddress{sensor.get(), "enteredText"},
               FieldAddress{sink.get(), "title"});
  ctx.pushKeyCharacter("a", true);
  ctx.tick(1.0);
  REQUIRE(sensor->getEnteredText() == "a");
  ctx.pushStringTerminator();
  ctx.tick(2.0);
  CHECK(sensor->getFinalText() == "a");
  CHECK(sensor->getEnteredText().empty());
  CHECK(sink->getTitle() == "a"); // reset has no enteredText event
}
