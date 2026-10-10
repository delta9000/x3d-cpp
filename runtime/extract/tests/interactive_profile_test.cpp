// Annex C (Interactive profile) acceptance cases for the headless runtime host:
// an XML scene loaded into RuntimeSession with the standard runtime plus the
// interactive pointer/navigation systems, driven only through the public input
// seam (pointer ray/button/screen, navigation keys, key-device events, bind
// events) and observed through field reads and field-write notifications.
// These cover the components Interactive adds to Interchange: pointing device
// sensor 1, key device sensor 1, environmental sensor 1, navigation 1, event
// utilities 1, Anchor activation and the viewpoint bind stack.
#include "RuntimeSession.hpp"
#include "X3DParse.hpp"
#include "doctest/doctest.h"

#include <any>
#include <cmath>
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace x3d::runtime;
using namespace x3d::core;

namespace {

// A session plus a clock, the pointer seam and an event log by DEF name.
struct Host {
  std::unique_ptr<RuntimeSession> session;
  double now = 0;
  std::map<std::string, std::vector<std::any>> log; // "DEF.field" -> values

  explicit Host(const std::string &scene, SessionOptions options = {}) {
    options.interactive = true;
    session = RuntimeSession::create(
        x3d::codec::parseDocument("<X3D profile='Interactive' version='4.0'><Scene>" +
                                  scene + "</Scene></X3D>"),
        std::move(options));
    REQUIRE(session->routes().rejected.empty());
    std::map<const X3DNode *, std::string> names;
    for (const auto &[name, node] : session->scene().defs) names[node.get()] = name;
    ctx().addFieldWriteListener([this, names](const FieldAddress &a) {
      auto found = names.find(a.node);
      if (found == names.end()) return;
      for (const auto &info : a.node->fields())
        if (info.x3dName == a.field && info.get)
          log[found->second + "." + a.field].push_back(info.get(*a.node));
    });
    session->fullSnapshot();
    tick();
  }

  X3DExecutionContext &ctx() { return session->context(); }
  X3DNode &node(const char *def) {
    auto n = session->scene().resolve(def);
    REQUIRE(n);
    return *n;
  }
  template <class T> T get(const char *def, const char *field) {
    auto &n = node(def);
    for (const auto &info : n.fields())
      if (info.x3dName == field && info.get) return std::any_cast<T>(info.get(n));
    FAIL("missing field " << def << '.' << field);
    return T{};
  }
  std::size_t count(const std::string &key) const {
    auto found = log.find(key);
    return found == log.end() ? 0 : found->second.size();
  }
  template <class T> T last(const std::string &key) const {
    auto found = log.find(key);
    REQUIRE(found != log.end());
    return std::any_cast<T>(found->second.back());
  }

  void tick(double dt = 0.1) { session->tick(now += dt); }
  void post(const char *def, const char *field, std::any value) {
    ctx().postEvent(&node(def), field, std::move(value));
    tick();
  }
  void aim(SFVec3f origin, SFVec3f direction = {0, 0, -1}) {
    ctx().setPointerPresent(true);
    ctx().setPointer(Ray{origin, direction});
    tick();
  }
  void button(bool down) {
    ctx().setPointerButton(down);
    tick();
  }
  void click(SFVec3f origin) {
    aim(origin);
    button(true);
    button(false);
  }
  SFVec3f eye() { return ctx().cameraWorldPosition(); }
  SFVec3f forward() {
    return ctx().viewMatrix().inverse().transformDirection(SFVec3f{0, 0, -1});
  }
};

bool near(float a, float b, float e = 1e-3f) { return std::fabs(a - b) < e; }
bool near(const SFVec3f &a, const SFVec3f &b, float e = 1e-3f) {
  return near(a.x, b.x, e) && near(a.y, b.y, e) && near(a.z, b.z, e);
}
float length(const SFVec3f &v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }

// Rodrigues rotation of v by an axis-angle SFRotation.
SFVec3f rotate(const SFRotation &r, const SFVec3f &v) {
  const float n = std::sqrt(r.x * r.x + r.y * r.y + r.z * r.z);
  if (n == 0) return v;
  const SFVec3f k{r.x / n, r.y / n, r.z / n};
  const float c = std::cos(r.angle), s = std::sin(r.angle);
  const float d = k.x * v.x + k.y * v.y + k.z * v.z;
  const SFVec3f x{k.y * v.z - k.z * v.y, k.z * v.x - k.x * v.z, k.x * v.y - k.y * v.x};
  return {v.x * c + x.x * s + k.x * d * (1 - c), v.y * c + x.y * s + k.y * d * (1 - c),
          v.z * c + x.z * s + k.z * d * (1 - c)};
}

} // namespace

TEST_SUITE("Interactive profile acceptance") {

TEST_CASE("Interactive: TouchSensor reports sensor-local hits and touchTime on release over") {
  Host host(R"(<Viewpoint position='0 0 20'/>
<Transform translation='2 0 0' scale='2 2 2'><Group>
  <TouchSensor DEF='T' description='press'/><Shape><Box size='2 2 2'/></Shape>
</Group></Transform>)");
  host.aim({2, 0, 10});
  CHECK(host.get<SFBool>("T", "isOver"));
  // Box front face at local z = 1 (world z = 2): local hit, normal and texture.
  CHECK(near(host.get<SFVec3f>("T", "hitPoint_changed"), {0, 0, 1}));
  CHECK(near(host.get<SFVec3f>("T", "hitNormal_changed"), {0, 0, 1}));
  const auto uv = host.get<SFVec2f>("T", "hitTexCoord_changed");
  CHECK(near(uv.x, 0.5f));
  CHECK(near(uv.y, 0.5f));

  host.button(true);
  CHECK(host.get<SFBool>("T", "isActive"));
  CHECK(host.count("T.touchTime") == 0);
  const double release = host.now + 0.1;
  host.button(false);
  CHECK_FALSE(host.get<SFBool>("T", "isActive"));
  REQUIRE(host.count("T.touchTime") == 1);
  CHECK(host.get<SFTime>("T", "touchTime") == doctest::Approx(release));

  // Pressing over the geometry and releasing elsewhere sends no touchTime.
  host.button(true);
  host.aim({50, 0, 10});
  host.button(false);
  CHECK(host.count("T.touchTime") == 1);
  CHECK_FALSE(host.get<SFBool>("T", "isOver"));

  // A disabled sensor reports nothing.
  host.post("T", "set_enabled", SFBool{false});
  host.click({2, 0, 10});
  CHECK(host.count("T.touchTime") == 1);
}

TEST_CASE("Interactive: the lowest sensors under the pointer win, siblings together") {
  Host host(R"(<Viewpoint position='0 0 20'/>
<Group><TouchSensor DEF='Outer'/>
  <Group><TouchSensor DEF='A'/><TouchSensor DEF='B'/><Shape><Box/></Shape></Group>
  <Transform translation='5 0 0'><Shape><Box/></Shape></Transform>
</Group>)");
  host.click({0, 0, 10});
  CHECK(host.count("A.touchTime") == 1);
  CHECK(host.count("B.touchTime") == 1);
  CHECK(host.count("Outer.touchTime") == 0);
  // Geometry only below Outer's group activates Outer.
  host.click({5, 0, 10});
  CHECK(host.count("Outer.touchTime") == 1);
  CHECK(host.count("A.touchTime") == 1);
  // With the lower sensors disabled, the next enabled ancestor sensor wins.
  host.post("A", "set_enabled", SFBool{false});
  host.post("B", "set_enabled", SFBool{false});
  host.click({0, 0, 10});
  CHECK(host.count("Outer.touchTime") == 2);
  CHECK(host.count("A.touchTime") == 1);
}

TEST_CASE("Interactive: PlaneSensor drags, clamps per axis and keeps its offset") {
  Host host(R"(<Viewpoint position='0 0 20'/>
<Group>
  <PlaneSensor DEF='P' minPosition='0 0' maxPosition='3 -1'/>
  <Transform DEF='Knob'><Shape><Box/></Shape></Transform>
</Group>
<ROUTE fromNode='P' fromField='translation_changed' toNode='Knob' toField='set_translation'/>)");
  host.aim({0, 0, 10});
  host.button(true);
  CHECK(host.get<SFBool>("P", "isActive"));
  host.aim({2, 0.5f, 10});
  // x is clamped to [0, 3]; y is unclamped because minPosition.y > maxPosition.y.
  CHECK(near(host.get<SFVec3f>("P", "translation_changed"), {2, 0.5f, 0}));
  CHECK(near(host.get<SFVec3f>("P", "trackPoint_changed"), {2, 0.5f, 1}));
  CHECK(near(host.get<SFVec3f>("Knob", "translation"), {2, 0.5f, 0}));
  host.aim({5, -2, 10});
  CHECK(near(host.get<SFVec3f>("P", "translation_changed"), {3, -2, 0}));
  CHECK(near(host.get<SFVec3f>("P", "trackPoint_changed"), {5, -2, 1}));
  host.aim({-4, 0, 10});
  CHECK(near(host.get<SFVec3f>("P", "translation_changed"), {0, 0, 0}));
  host.aim({1, 1, 10});
  host.button(false);
  CHECK_FALSE(host.get<SFBool>("P", "isActive"));
  // autoOffset (default TRUE) stores the last translation as the new offset.
  CHECK(near(host.get<SFVec3f>("P", "offset"), {1, 1, 0}));
  CHECK(host.count("P.offset") == 1);

  // A second drag continues from the offset.
  host.aim({1, 1, 10});
  host.button(true);
  host.aim({2, 1, 10});
  CHECK(near(host.get<SFVec3f>("P", "translation_changed"), {2, 1, 0}));
  host.button(false);
}

TEST_CASE("Interactive: PlaneSensor axisRotation rotates the drag plane") {
  // A 90 degree turn about Z maps the sensor's X axis to world Y, so the
  // [0, 1] x clamp limits vertical motion. Outputs are in the local sensor
  // coordinate system that axisRotation creates (§20.4.2), so the clamped
  // vertical drag is translation x and the leftward drag is -y.
  Host host(R"(<Viewpoint position='0 0 20'/>
<Group><PlaneSensor DEF='P' axisRotation='0 0 1 1.5707963' minPosition='0 0'
  maxPosition='1 -1'/><Shape><Box/></Shape></Group>)");
  host.aim({0, 0, 10});
  host.button(true);
  host.aim({0.5f, 3, 10});
  const auto t = host.get<SFVec3f>("P", "translation_changed");
  CHECK(near(t, {1, -0.5f, 0}));
  host.button(false);
}

TEST_CASE("Interactive: CylinderSensor rotates about Y and clamps its angle") {
  Host host(R"(<Viewpoint position='0 0 20'/>
<Group><CylinderSensor DEF='C' minAngle='0' maxAngle='0.25'/>
  <Shape><Cylinder radius='1' height='2'/></Shape></Group>)");
  host.aim({0, 0, 10});
  host.button(true);
  CHECK(host.get<SFBool>("C", "isActive"));
  // Sideways bearing: the virtual cylinder through the hit point (radius 1).
  host.aim({std::sin(0.1f), 0, 10});
  auto r = host.get<SFRotation>("C", "rotation_changed");
  CHECK(near(rotate(r, {0, 0, 1}), {std::sin(0.1f), 0, std::cos(0.1f)}, 0.01f));
  CHECK(near(std::fabs(r.y), length({r.x, r.y, r.z}), 1e-4f)); // about the Y axis
  host.aim({0.9f, 0, 10});
  r = host.get<SFRotation>("C", "rotation_changed");
  CHECK(near(rotate(r, {0, 0, 1}), {std::sin(0.25f), 0, std::cos(0.25f)}, 0.01f));
  host.aim({-0.5f, 0, 10});
  r = host.get<SFRotation>("C", "rotation_changed");
  CHECK(near(rotate(r, {0, 0, 1}), {0, 0, 1}, 0.01f));
  host.aim({std::sin(0.2f), 0, 10});
  host.button(false);
  CHECK(host.get<SFFloat>("C", "offset") == doctest::Approx(0.2f).epsilon(0.02));
}

TEST_CASE("Interactive: CylinderSensor uses the disk for bearings near its axis") {
  // Looking down the Y axis: the activation bearing is within diskAngle, so the
  // drag rotates on the Y=0 disk. Moving from +Z toward +X is positive about +Y.
  Host host(R"(<Viewpoint position='0 20 0' orientation='1 0 0 -1.5707963'/>
<Group><CylinderSensor DEF='C'/><Shape><Cylinder radius='2' height='2'/></Shape></Group>)");
  host.aim({0, 10, 1}, {0, -1, 0});
  host.button(true);
  CHECK(host.get<SFBool>("C", "isActive"));
  host.aim({1, 10, 0}, {0, -1, 0});
  const auto r = host.get<SFRotation>("C", "rotation_changed");
  CHECK(near(rotate(r, {0, 0, 1}), {1, 0, 0}, 0.01f));
  host.button(false);
}

TEST_CASE("Interactive: SphereSensor rotates the hit point on its virtual sphere") {
  Host host(R"(<Viewpoint position='0 0 20'/>
<Group><SphereSensor DEF='S'/><Shape><Sphere radius='1'/></Shape></Group>)");
  host.aim({0, 0, 10});
  host.button(true);
  host.aim({std::sin(0.3f), 0, 10});
  auto r = host.get<SFRotation>("S", "rotation_changed");
  CHECK(near(rotate(r, {0, 0, 1}), {std::sin(0.3f), 0, std::cos(0.3f)}, 0.01f));
  host.aim({0, std::sin(0.3f), 10});
  r = host.get<SFRotation>("S", "rotation_changed");
  CHECK(near(rotate(r, {0, 0, 1}), {0, std::sin(0.3f), std::cos(0.3f)}, 0.01f));
  host.button(false);
  // autoOffset: the next drag composes with the stored rotation.
  CHECK(near(rotate(host.get<SFRotation>("S", "offset"), {0, 0, 1}),
             {0, std::sin(0.3f), std::cos(0.3f)}, 0.01f));
}

TEST_CASE("Interactive: KeySensor and StringSensor follow keyboard input") {
  Host host(R"(<KeySensor DEF='K'/><StringSensor DEF='S'/>)");
  auto &ctx = host.ctx();
  ctx.pushKeyCharacter("a", true);
  host.tick();
  CHECK(host.get<SFBool>("K", "isActive"));
  CHECK(host.get<SFString>("K", "keyPress") == "a");
  CHECK(host.get<SFBool>("S", "isActive"));
  ctx.pushModifierKey(1, true);
  ctx.pushActionKey(13, true); // HOME
  host.tick();
  CHECK(host.get<SFBool>("K", "shiftKey"));
  CHECK(host.get<SFInt32>("K", "actionKeyPress") == 13);
  ctx.pushActionKey(13, false);
  ctx.pushModifierKey(1, false);
  ctx.pushKeyCharacter("a", false);
  host.tick();
  CHECK(host.get<SFInt32>("K", "actionKeyRelease") == 13);
  CHECK_FALSE(host.get<SFBool>("K", "shiftKey"));
  CHECK(host.get<SFString>("K", "keyRelease") == "a");
  CHECK_FALSE(host.get<SFBool>("K", "isActive"));

  for (const char *c : {"b", "c"}) {
    ctx.pushKeyCharacter(c, true);
    ctx.pushKeyCharacter(c, false);
  }
  host.tick();
  CHECK(host.get<SFString>("S", "enteredText") == "abc");
  ctx.pushStringDeletion();
  host.tick();
  CHECK(host.get<SFString>("S", "enteredText") == "ab");
  ctx.pushStringTerminator();
  host.tick();
  CHECK(host.get<SFString>("S", "finalText") == "ab");
  CHECK_FALSE(host.get<SFBool>("S", "isActive"));
  CHECK(host.count("S.finalText") == 1);

  // A disabled KeySensor ignores input.
  host.post("K", "set_enabled", SFBool{false});
  const auto presses = host.count("K.keyPress");
  ctx.pushKeyCharacter("z", true);
  ctx.pushKeyCharacter("z", false);
  host.tick();
  CHECK(host.count("K.keyPress") == presses);
}

TEST_CASE("Interactive: ProximitySensor tracks the bound viewer in its own frame") {
  Host host(R"(<Viewpoint DEF='Out' position='0 0 10'/>
<Viewpoint DEF='In' position='10 0 1'/>
<Transform translation='10 0 0'><ProximitySensor DEF='Prox' size='4 4 4'/></Transform>)");
  CHECK_FALSE(host.get<SFBool>("Prox", "isActive"));
  host.post("In", "set_bind", SFBool{true});
  CHECK(host.get<SFBool>("Prox", "isActive"));
  CHECK(host.count("Prox.enterTime") == 1);
  CHECK(near(host.get<SFVec3f>("Prox", "position_changed"), {0, 0, 1}));
  const auto facing = rotate(host.get<SFRotation>("Prox", "orientation_changed"), {0, 0, -1});
  CHECK(near(facing, {0, 0, -1}));
  // Moving inside the region updates the position in the sensor frame.
  host.post("In", "set_position", SFVec3f{11, 1, 0});
  CHECK(near(host.get<SFVec3f>("Prox", "position_changed"), {1, 1, 0}));
  CHECK(host.count("Prox.enterTime") == 1);
  const double leave = host.now + 0.1;
  host.post("In", "set_bind", SFBool{false});
  CHECK_FALSE(host.get<SFBool>("Prox", "isActive"));
  REQUIRE(host.count("Prox.exitTime") == 1);
  CHECK(host.get<SFTime>("Prox", "exitTime") == doctest::Approx(leave));
}

TEST_CASE("Interactive: VisibilitySensor reports a region in view") {
  // Annex C permits treating the region as always visible; this host tests it.
  Host host(R"(<Viewpoint position='0 0 10'/>
<Transform translation='0 0 -5'><VisibilitySensor DEF='V' size='1 1 1'/></Transform>
<VisibilitySensor DEF='Off' enabled='false'/>)");
  CHECK(host.get<SFBool>("V", "isActive"));
  CHECK(host.count("V.enterTime") == 1);
  CHECK(host.count("Off.enterTime") == 0);
}

TEST_CASE("Interactive: Viewpoint bind stack, orientation and bind outputs") {
  Host host(R"(<Viewpoint DEF='A' position='0 0 10'/>
<Viewpoint DEF='B' position='5 0 0' orientation='0 1 0 1.5707963'/>)");
  CHECK(host.get<SFBool>("A", "isBound"));
  CHECK(near(host.eye(), {0, 0, 10}));
  CHECK(near(host.forward(), {0, 0, -1}));
  host.post("B", "set_bind", SFBool{true});
  CHECK(host.get<SFBool>("B", "isBound"));
  CHECK_FALSE(host.get<SFBool>("A", "isBound"));
  CHECK(host.get<SFTime>("B", "bindTime") == doctest::Approx(host.now));
  CHECK(near(host.eye(), {5, 0, 0}));
  CHECK(near(host.forward(), {-1, 0, 0}));
  // Unbinding the top restores the previous viewpoint.
  host.post("B", "set_bind", SFBool{false});
  CHECK(host.get<SFBool>("A", "isBound"));
  CHECK_FALSE(host.get<SFBool>("B", "isBound"));
  CHECK(near(host.eye(), {0, 0, 10}));
}

TEST_CASE("Interactive: viewpoint transitions animate, and jump FALSE keeps the view") {
  Host host(R"(<NavigationInfo DEF='N' transitionTime='1'/>
<Viewpoint DEF='A' position='0 0 10'/>
<Viewpoint DEF='B' position='0 0 30'/>
<Viewpoint DEF='C' position='40 0 0' jump='false'/>)");
  host.post("B", "set_bind", SFBool{true});
  // Mid-transition: between the two poses.
  host.tick(0.4);
  CHECK(host.eye().z > 10.5f);
  CHECK(host.eye().z < 29.5f);
  for (int i = 0; i < 10; ++i) host.tick();
  CHECK(near(host.eye(), {0, 0, 30}, 1e-2f));
  CHECK(host.count("N.transitionComplete") >= 1);
  // jump FALSE: C becomes bound but the user's view does not move.
  host.post("C", "set_bind", SFBool{true});
  for (int i = 0; i < 12; ++i) host.tick();
  CHECK(host.get<SFBool>("C", "isBound"));
  CHECK(near(host.eye(), {0, 0, 30}, 1e-2f));
}

TEST_CASE("Interactive: EXAMINE orbits about centerOfRotation") {
  Host host(R"(<NavigationInfo type='"EXAMINE"'/>
<Viewpoint position='1 0 10' centerOfRotation='1 0 0'/>)");
  auto &ctx = host.ctx();
  ctx.setPointerPresent(true);
  ctx.setPointer(Ray{{0, 50, 10}, {0, 0, -1}}); // over nothing
  ctx.setPointerScreen(0.5f, 0.5f);
  ctx.setPointerButton(true);
  host.tick();
  ctx.setPointerScreen(0.6f, 0.5f);
  host.tick();
  ctx.setPointerButton(false);
  host.tick();
  const SFVec3f pivot{1, 0, 0}, eye = host.eye();
  const SFVec3f offset{eye.x - pivot.x, eye.y - pivot.y, eye.z - pivot.z};
  CHECK(near(length(offset), 10, 1e-2f));
  CHECK_FALSE(near(eye, {1, 0, 10}, 0.1f));
  // Still looking at the pivot.
  const auto f = host.forward();
  CHECK(near(f, {-offset.x / 10, -offset.y / 10, -offset.z / 10}, 1e-2f));
}

TEST_CASE("Interactive: FLY moves along the view at NavigationInfo speed") {
  Host host(R"(<NavigationInfo type='"FLY" "ANY"' speed='2'/>
<Viewpoint position='0 0 10'/>)");
  host.ctx().setKey(NavigationSystem::kKeyForward, true);
  host.tick(1.0);
  host.ctx().setKey(NavigationSystem::kKeyForward, false);
  host.tick();
  CHECK(near(host.eye(), {0, 0, 8}, 1e-2f));
}

TEST_CASE("Interactive: LOOKAT animates toward the picked object") {
  Host host(R"(<NavigationInfo DEF='N' type='"LOOKAT" "ANY"' transitionTime='1'/>
<Viewpoint DEF='V' position='0 0 10'/>
<Transform translation='3 0 0'><Shape><Box/></Shape></Transform>)");
  host.click({3, 0, 10});
  for (int i = 0; i < 15; ++i) host.tick();
  CHECK(host.count("N.transitionComplete") == 1);
  const auto eye = host.eye(), f = host.forward();
  // The view now centers the box: the box lies straight ahead.
  const SFVec3f toBox{3 - eye.x, -eye.y, -eye.z};
  const float d = length(toBox);
  CHECK(near(f, {toBox.x / d, toBox.y / d, toBox.z / d}, 1e-2f));
  CHECK(d < 10);
  CHECK(near(host.get<SFVec3f>("V", "centerOfRotation"), {3, 0, 0}, 1e-2f));
}

TEST_CASE("Interactive: event utilities run a routed click state machine") {
  Host host(R"(<Viewpoint position='0 0 20'/>
<Group><TouchSensor DEF='Btn'/><Shape><Box/></Shape></Group>
<BooleanFilter DEF='F'/><BooleanToggle DEF='Tog'/><BooleanTrigger DEF='BT'/>
<IntegerTrigger DEF='IT' integerKey='2'/><TimeTrigger DEF='TT'/>
<Switch DEF='Sw' whichChoice='-1'><Group/><Group/><Group/></Switch>
<ROUTE fromNode='Btn' fromField='isActive' toNode='F' toField='set_boolean'/>
<ROUTE fromNode='F' fromField='inputTrue' toNode='Tog' toField='set_boolean'/>
<ROUTE fromNode='Tog' fromField='toggle_changed' toNode='IT' toField='set_boolean'/>
<ROUTE fromNode='IT' fromField='triggerValue' toNode='Sw' toField='set_whichChoice'/>
<ROUTE fromNode='Btn' fromField='touchTime' toNode='BT' toField='set_triggerTime'/>
<ROUTE fromNode='F' fromField='inputFalse' toNode='TT' toField='set_boolean'/>)");
  host.aim({0, 0, 10});
  host.button(true);
  CHECK(host.get<SFBool>("Tog", "toggle"));
  CHECK(host.get<SFInt32>("Sw", "whichChoice") == 2);
  const double release = host.now + 0.1;
  host.button(false);
  CHECK(host.get<SFBool>("BT", "triggerTrue"));
  CHECK(host.get<SFTime>("TT", "triggerTime") == doctest::Approx(release));
  host.post("Sw", "set_whichChoice", SFInt32{0});
  // Second click: toggle goes FALSE, IntegerTrigger ignores FALSE.
  host.button(true);
  CHECK_FALSE(host.get<SFBool>("Tog", "toggle"));
  CHECK(host.get<SFInt32>("Sw", "whichChoice") == 0);
  CHECK(host.count("IT.triggerValue") == 1);
  host.button(false);
  CHECK(host.count("TT.triggerTime") == 2);
}

TEST_CASE("Interactive: sequencers driven by a TimeSensor step once per key interval") {
  Host host(R"(<TimeSensor DEF='Clock' cycleInterval='4' loop='true'/>
<IntegerSequencer DEF='Seq' key='0 0.25 0.75' keyValue='0 1 2'/>
<BooleanSequencer DEF='On' key='0 0.5' keyValue='false true'/>
<Switch DEF='Sw' whichChoice='-1'><Group/><Group/><Group/></Switch>
<ROUTE fromNode='Clock' fromField='fraction_changed' toNode='Seq' toField='set_fraction'/>
<ROUTE fromNode='Clock' fromField='fraction_changed' toNode='On' toField='set_fraction'/>
<ROUTE fromNode='Seq' fromField='value_changed' toNode='Sw' toField='set_whichChoice'/>)");
  // Ten ticks per second across 1.5 cycles.
  for (int i = 0; i < 60; ++i) host.tick();
  // Intervals entered: 0, 1, 2, wrap to 0, 1 -> five events, not one per tick.
  CHECK(host.count("Seq.value_changed") == 5);
  CHECK(host.count("Sw.whichChoice") == 5);
  CHECK(host.count("On.value_changed") == 4); // false, true, wrap false, true at t=6
  CHECK(host.get<SFInt32>("Sw", "whichChoice") == 1);
}

TEST_CASE("Interactive: Anchor binds in-scene viewpoints and hands other urls to the host") {
  std::vector<std::string> opened;
  SessionOptions options;
  options.anchorHandler = [&](X3DNode *, const MFString &url, const MFString &parameter) {
    opened = url;
    opened.insert(opened.end(), parameter.begin(), parameter.end());
  };
  Host host(R"(<Viewpoint DEF='Near' position='0 0 20'/>
<Viewpoint DEF='Far' position='0 0 50'/>
<Anchor url='"#Missing" "#Far"' description='go far'><Shape><Box/></Shape></Anchor>
<Transform translation='5 0 0'><Anchor url='"next.x3d"' parameter='"target=_blank"'>
  <Shape><Box/></Shape></Anchor></Transform>)",
            std::move(options));
  host.click({0, 0, 10});
  CHECK(host.get<SFBool>("Far", "isBound"));
  CHECK(near(host.eye(), {0, 0, 50}));
  host.post("Far", "set_bind", SFBool{false});
  host.click({5, 0, 10});
  CHECK(opened == std::vector<std::string>{"next.x3d", "target=_blank"});
}

} // TEST_SUITE
