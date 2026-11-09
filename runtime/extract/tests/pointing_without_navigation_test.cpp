// Host-owned navigation: query the SDK's sensor scope before claiming a press.
#include <x3d/sdk.hpp>
#include "doctest/doctest.h"

#include <any>
#include <vector>

namespace sdk = x3d::sdk;

TEST_CASE("Session: pointing sensors without navigation and pre-press resolution") {
  for (bool interactive : {false, true}) {
    sdk::SessionOptions options;
    options.pointingSensors = true;
    options.interactive = interactive;
    auto session = sdk::RuntimeSession::create(sdk::parseDocument(R"(
<X3D profile='Interactive' version='4.0'><Scene>
  <NavigationInfo type='"FLY"'/><Viewpoint position='0 0 10'/>
  <Group><TouchSensor DEF='Outer'/>
    <Group><TouchSensor DEF='Touch'/><PlaneSensor DEF='Drag'/>
      <TouchSensor USE='Touch'/>
      <TouchSensor DEF='Disabled' enabled='false'/>
      <Shape><Box/></Shape>
    </Group>
  </Group>
  <Transform translation='5 0 0'><Shape><Box/></Shape></Transform>
</Scene></X3D>)"), options);
    CHECK(bool(session->navigation()) == interactive);
    auto &ctx = session->context();
    auto node = [&](const char *name) { return session->scene().resolve(name).get(); };
    auto flag = [&](const char *name, const char *fieldName) {
      for (const auto &field : node(name)->fields())
        if (field.x3dName == fieldName && field.get)
          return std::any_cast<bool>(field.get(*node(name)));
      FAIL("missing sensor flag " << fieldName);
      return false;
    };
    auto active = [&](const char *name) { return flag(name, "isActive"); };
    unsigned activations = 0;
    ctx.addFieldWriteListener([&](const auto &address) {
      if (address.node == node("Touch") && address.field == "isActive" && active("Touch"))
        ++activations;
    });
    session->tick(0);
    const sdk::Ray ray{{0, 0, 10}, {0, 0, -1}};
    const auto pick = ctx.pick(ray);
    REQUIRE(pick.hit);
    const std::vector<sdk::X3DNode *> expected{node("Touch"), node("Drag")};
    const auto revision = ctx.pointerState().revision;
    CHECK(session->pointingSensors(pick) == expected);
    CHECK(ctx.pointerState().revision == revision);
    CHECK_FALSE(flag("Touch", "isOver"));
    CHECK_FALSE(active("Touch")); // querying does not activate or consume input
    CHECK_FALSE(ctx.pointerConsumedBySensor());
    CHECK(session->pointingSensors(sdk::PickResult{}).empty());
    const auto bare = ctx.pick(sdk::Ray{{5, 0, 10}, {0, 0, -1}});
    REQUIRE(bare.hit);
    CHECK(session->pointingSensors(bare).empty());
    CHECK(session->pointingSensors(ctx.pick(sdk::Ray{{20, 0, 10}, {0, 0, -1}})).empty());

    ctx.setPointerPresent(true);
    ctx.setPointer(ray);
    ctx.setPointerButton(true);
    session->tick(0.1);
    CHECK(active("Touch"));
    CHECK(active("Drag"));
    CHECK_FALSE(active("Outer"));
    CHECK_FALSE(active("Disabled"));
    CHECK(activations == 1); // both flags must not install the driver twice
    ctx.setPointerButton(false);
    session->tick(0.2);
    CHECK_FALSE(active("Touch"));
    CHECK_FALSE(active("Drag"));

    // Enablement is resolved afresh, even when reusing a still-current path.
    ctx.postEvent(node("Touch"), "set_enabled", std::any(false));
    ctx.postEvent(node("Drag"), "set_enabled", std::any(false));
    session->tick(0.3);
    const std::vector<sdk::X3DNode *> outer{node("Outer")};
    CHECK(session->pointingSensors(pick) == outer);
    if (!interactive) {
      const auto eye = ctx.cameraWorldPosition();
      ctx.setKey(x3d::runtime::NavigationSystem::kKeyForward, true);
      session->tick(1.3);
      const auto after = ctx.cameraWorldPosition();
      CHECK(after.x == eye.x);
      CHECK(after.y == eye.y);
      CHECK(after.z == eye.z);
    }
  }
}

TEST_CASE("Session: pointing sensors remain opt-in") {
  auto session = sdk::RuntimeSession::create(sdk::parseDocument(R"(
<X3D profile='Interactive' version='4.0'><Scene>
<Group><TouchSensor DEF='Touch'/><Shape><Box/></Shape></Group>
</Scene></X3D>)"));
  auto &ctx = session->context();
  const sdk::Ray ray{{0, 0, 10}, {0, 0, -1}};
  CHECK_FALSE(session->navigation());
  REQUIRE(session->pointingSensors(ctx.pick(ray)).size() == 1);
  ctx.setPointerPresent(true);
  ctx.setPointer(ray);
  ctx.setPointerButton(true);
  session->tick(0.1);
  CHECK_FALSE(ctx.pointerConsumedBySensor());
  auto sensor = session->scene().resolve("Touch");
  for (const auto &field : sensor->fields())
    if (field.x3dName == "isActive" && field.get)
      CHECK_FALSE(std::any_cast<bool>(field.get(*sensor)));
}
