// Standalone ext-gated cases. Each CTest case selects one function by argument.
// Checks remain active in Release builds; no assertions with side effects.
#include "x3d/ext.hpp"
#include "SceneExtractor.hpp"
#include "AssetDocumentResolvers.hpp"
#include "X3DCodecs.hpp"
#include "X3DParse.hpp"
#include "x3d/nodes/Transform.hpp"
#include "x3d/nodes/Group.hpp"

#include <any>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <map>
#include <stdexcept>
#include <string>
#include <type_traits>

using namespace x3d::core;
using namespace x3d::runtime;
using namespace x3d::runtime::ext;
using namespace x3d::codec;

namespace {
void check(bool condition, const char *message) {
  if (!condition) throw std::runtime_error(message);
}
void near(const Mat4 &a, const Mat4 &b) {
  for (std::size_t i = 0; i < a.m.size(); ++i)
    check(std::fabs(a.m[i] - b.m[i]) < 3e-4f, "matrix mismatch");
}
void near(const SFVec3f &a, const SFVec3f &b) {
  check(std::fabs(a.x-b.x) < 3e-4f && std::fabs(a.y-b.y) < 3e-4f &&
        std::fabs(a.z-b.z) < 3e-4f, "position mismatch");
}

const char *declarations = R"(
<ExternProtoDeclare name='Tracked' url='"urn:x3d-cpp-gen:ext:TrackedDeviceSensor:1" "cave-fallbacks.x3d#TrackedDeviceSensor"'>
  <field name='metadata' type='SFNode' accessType='inputOutput'/>
  <field name='description' type='SFString' accessType='inputOutput'/>
  <field name='enabled' type='SFBool' accessType='inputOutput'/>
  <field name='device' type='SFString' accessType='initializeOnly'/>
  <field name='isActive' type='SFBool' accessType='outputOnly'/>
  <field name='position_changed' type='SFVec3f' accessType='outputOnly'/>
  <field name='orientation_changed' type='SFRotation' accessType='outputOnly'/>
  <field name='roomPosition_changed' type='SFVec3f' accessType='outputOnly'/>
  <field name='roomOrientation_changed' type='SFRotation' accessType='outputOnly'/>
  <field name='buttons_changed' type='MFBool' accessType='outputOnly'/>
  <field name='joystick_changed' type='SFVec2f' accessType='outputOnly'/>
</ExternProtoDeclare>
<ExternProtoDeclare name='RoomFrame' url='"urn:x3d-cpp-gen:ext:RoomFrameSensor:1" "cave-fallbacks.x3d#RoomFrameSensor"'>
  <field name='metadata' type='SFNode' accessType='inputOutput'/>
  <field name='description' type='SFString' accessType='inputOutput'/>
  <field name='enabled' type='SFBool' accessType='inputOutput'/>
  <field name='isActive' type='SFBool' accessType='outputOnly'/>
  <field name='position_changed' type='SFVec3f' accessType='outputOnly'/>
  <field name='orientation_changed' type='SFRotation' accessType='outputOnly'/>
  <field name='scale_changed' type='SFVec3f' accessType='outputOnly'/>
</ExternProtoDeclare>
<ExternProtoDeclare name='RoomGroup' url='"urn:x3d-cpp-gen:ext:RoomGroup:1" "cave-fallbacks.x3d#RoomGroup"'>
  <field name='metadata' type='SFNode' accessType='inputOutput'/>
  <field name='children' type='MFNode' accessType='inputOutput'/>
  <field name='addChildren' type='MFNode' accessType='inputOnly'/>
  <field name='removeChildren' type='MFNode' accessType='inputOnly'/>
  <field name='visible' type='SFBool' accessType='inputOutput'/>
  <field name='bboxDisplay' type='SFBool' accessType='inputOutput'/>
  <field name='bboxCenter' type='SFVec3f' accessType='initializeOnly'/>
  <field name='bboxSize' type='SFVec3f' accessType='initializeOnly'/>
</ExternProtoDeclare>
)";
std::string xml(const std::string &body) {
  return "<X3D profile='Full' version='4.0'><Scene>" + std::string(declarations) + body + "</Scene></X3D>";
}
X3DDocument document(const std::string &body) {
  auto doc = parseDocument(xml(body), Encoding::XML, "", install());
  for (const auto &warning : doc.protoWarnings) std::cerr << warning.detail << '\n';
  check(doc.protoWarnings.empty(), "native expansion generated PROTO warnings");
  return doc;
}
std::shared_ptr<CaveInputSystem> setup(Scene &scene, X3DExecutionContext &ctx) {
  check(buildRoutes(scene, ctx).rejected.empty(), "ROUTE rejected");
  ctx.buildSceneGraph(scene);
  return installCaveInput(scene, ctx);
}
RoomFrame frame() {
  RoomFrame f;
  f.valid = true;
  f.devices.push_back({"WAND", true, {{1, 2, -3}, {0, 1, 0, 0.4f}},
                       MFBool{true, false, true}, {0.5f, -0.75f}});
  return f;
}

void recursive_resolver() {
  const std::map<std::string, std::string> files{
      {"memory://cave/child.x3d", xml(R"(
        <ExternProtoDeclare name='Widget' url='"library.x3d#Widget"'/>
        <ProtoInstance name='Widget'/>)")},
      {"memory://cave/library.x3d", xml(R"(
        <ProtoDeclare name='Widget'><ProtoBody>
          <ProtoInstance name='Tracked' DEF='NestedWand'>
            <fieldValue name='device' value='WAND'/>
          </ProtoInstance>
        </ProtoBody></ProtoDeclare>)")}};
  std::vector<std::string> fetched;
  extract::AssetResolver assets = [&](const std::string &url, auto) {
    fetched.push_back(url);
    const auto it = files.find(url);
    if (it == files.end()) return extract::AssetResult::makeFailed();
    return extract::AssetResult::makeReady({it->second.begin(), it->second.end()});
  };
  auto resolvers = assetResolversFrom(assets, Encoding::Unknown,
      [](ProtoDeclarationResolver base) { return install(std::move(base)); });
  auto doc = parseDocument(xml("<Inline url='&quot;child.x3d&quot;'/>") ,
      Encoding::XML, "memory://cave", resolvers.proto, resolvers.inlineScene);
  check(doc.inlineWarnings.empty(), "decorated Inline did not load");
  std::size_t tracked = 0;
  x3d::runtime::detail::forEachNode(doc.scene, [&](X3DNode *node) {
    if (dynamic_cast<TrackedDeviceSensor *>(node)) ++tracked;
  });
  check(tracked == 1, "external PROTO body lost the recursive ext decorator");
  check(fetched == std::vector<std::string>{"memory://cave/child.x3d",
      "memory://cave/library.x3d"}, "ext URNs escaped to the confined byte loader");
}

void proto_routes() {
  auto doc = document(R"(
    <ProtoDeclare name='Relay'><ProtoInterface>
      <field name='pose' type='SFVec3f' accessType='outputOnly'/>
    </ProtoInterface><ProtoBody>
      <ProtoInstance name='Tracked'>
        <fieldValue name='device' value='WAND'/>
        <IS><connect nodeField='position_changed' protoField='pose'/></IS>
      </ProtoInstance>
    </ProtoBody></ProtoDeclare>
    <ProtoInstance name='Relay' DEF='Input'/>
    <Transform DEF='Destination'/>
    <ProtoInstance name='RoomFrame' DEF='Room'/>
    <Transform DEF='RoomDestination'/>
    <ROUTE fromNode='Input' fromField='pose' toNode='Destination' toField='set_translation'/>
    <ROUTE fromNode='Room' fromField='position_changed' toNode='RoomDestination' toField='set_translation'/>
  )");
  X3DExecutionContext ctx(doc.scene.authorFields);
  auto system = setup(doc.scene, ctx);
  auto sample = frame();
  sample.roomFromWorld = Mat4::translation({7, 0, 2}) * Mat4::rotation({0, 1, 0, 0.6f});
  system->submit(sample);
  ctx.tick(1);
  auto sensor = std::dynamic_pointer_cast<TrackedDeviceSensor>(doc.scene.resolve("Input"));
  auto dest = std::dynamic_pointer_cast<x3d::nodes::Transform>(doc.scene.resolve("Destination"));
  auto roomDest = std::dynamic_pointer_cast<x3d::nodes::Transform>(doc.scene.resolve("RoomDestination"));
  check(sensor && dest && roomDest, "native clone or ordinary destination missing");
  check(sensor->getIsActive(), "native sensor inactive");
  const auto inverse = sample.roomFromWorld.inverse();
  near(sensor->getPosition_changed(), inverse.transformPoint({1, 2, -3}));
  near(dest->getTranslation(), sensor->getPosition_changed());
  near(roomDest->getTranslation(), inverse.transformPoint({0, 0, 0}));
  near(Mat4::rotation(sensor->getOrientation_changed()),
       Mat4::rotation(rotationFromMatrix(inverse)) * Mat4::rotation({0, 1, 0, 0.4f}));
  near(sensor->getRoomPosition_changed(), {1, 2, -3});
  check(doc.scene.protoRedirects.at(sensor.get()).count("pose") == 1,
        "test must exercise an exposed renamed PROTO output");
  const auto encoded = XmlWriter{}.writeDocument(doc);
  check(encoded.find("ProtoInstance") != std::string::npos, "lost instance during round-trip");
  check(encoded.find("<TrackedDeviceSensor") == std::string::npos, "serialized native implementation");
  auto again = parseDocument(encoded, Encoding::XML, "", install());
  check(again.protoWarnings.empty(), "native round-trip cannot expand");
}

const char *roomChild = R"(
  <Group><ProtoInstance name='RoomGroup' DEF='RoomRoot'>
    <fieldValue name='children'>
      <Transform translation='0.5 1 -3' rotation='0 1 0 0.2'>
        <Shape><Box size='0.4 0.4 0.4'/></Shape>
      </Transform>
    </fieldValue>
  </ProtoInstance></Group>
)";
void room_same_tick() {
  static_assert(std::is_same_v<decltype(Mat4{}.m)::value_type, float>);
  auto doc = document(roomChild);
  check(doc.scene.resolvedProtoRoutes.size() == 3, "RoomGroup needs three standard ROUTEs");
  auto root = doc.scene.resolve("RoomRoot");
  check(root && root->nodeTypeName() == "Transform", "RoomGroup must have a standard Transform primary");
  X3DExecutionContext ctx(doc.scene.authorFields);
  auto system = setup(doc.scene, ctx);
  extract::SceneExtractor extractor(ctx, doc.scene);
  const Mat4 child = Mat4::translation({0.5f, 1, -3}) * Mat4::rotation({0, 1, 0, 0.2f});
  extract::RenderItemId id = 0;
  for (int i = 0; i < 4; ++i) {
    auto f = frame();
    const float scale = 1.0f + i * 0.25f;
    f.roomFromWorld = Mat4::translation({float(2 + i*4), float(i), float(-i*3)}) *
        Mat4::rotation({0, 1, 0, 0.3f * (i+1)}) * Mat4::scale({scale, scale, scale});
    system->submit(f);
    ctx.tick(i + 1); // Exactly ONE tick for this submission, before extraction/pick.
    const auto delta = i == 0 ? extractor.fullSnapshot() : extractor.delta();
    if (i == 0) {
      check(delta.added.size() == 1, "expected one RoomGroup child placement");
      id = delta.added.front();
    } else {
      check(delta.updatedTransform.size() == 1, "moving room frame must produce a transform delta");
      check(delta.added.empty() && delta.removed.empty(), "room motion changed placement identity");
    }
    const Mat4 inverse = f.roomFromWorld.inverse();
    const auto &item = extractor.item(id);
    near(item.worldTransform, inverse * child);
    near(f.roomFromWorld * item.worldTransform, child);
    // The host converts a ray at the fixed ROOM position into world coordinates.
    const auto hit = ctx.pick({inverse.transformPoint({0.5f, 1, 0}),
                              inverse.transformDirection({0, 0, -1})});
    check(hit.hit && hit.node == item.path.back(), "same-frame room-position pick missed child");
    const auto roomHit = f.roomFromWorld.transformPoint(hit.point);
    check(std::fabs(roomHit.x - 0.5f) < 3e-4f && std::fabs(roomHit.y - 1) < 3e-4f &&
          roomHit.z > -3 && roomHit.z < -2.7f, "pick returned the wrong room-space surface point");
    check(system->warnings().empty(), "root-level RoomGroup incorrectly diagnosed");
  }
}

void tracking_lifecycle() {
  auto doc = document(R"(
    <ProtoInstance name='Tracked' DEF='Wand'><fieldValue name='device' value='WAND'/></ProtoInstance>
    <ProtoInstance name='Tracked' DEF='Unknown'><fieldValue name='device' value='missing'/></ProtoInstance>
    <ProtoInstance name='Tracked' DEF='Disabled'><fieldValue name='enabled' value='false'/><fieldValue name='device' value='WAND'/></ProtoInstance>
    <ProtoInstance name='Tracked' DEF='Head'/>
    <ProtoInstance name='RoomFrame' DEF='Room'/>
    <ProtoInstance name='RoomFrame' DEF='DisabledRoom'><fieldValue name='enabled' value='false'/></ProtoInstance>
    <Transform DEF='Destination'/>
    <ROUTE fromNode='Wand' fromField='position_changed' toNode='Destination' toField='set_translation'/>
  )");
  X3DExecutionContext ctx(doc.scene.authorFields);
  auto system = setup(doc.scene, ctx);
  auto wand = std::dynamic_pointer_cast<TrackedDeviceSensor>(doc.scene.resolve("Wand"));
  auto unknown = std::dynamic_pointer_cast<TrackedDeviceSensor>(doc.scene.resolve("Unknown"));
  auto disabled = std::dynamic_pointer_cast<TrackedDeviceSensor>(doc.scene.resolve("Disabled"));
  auto head = std::dynamic_pointer_cast<TrackedDeviceSensor>(doc.scene.resolve("Head"));
  auto room = std::dynamic_pointer_cast<RoomFrameSensor>(doc.scene.resolve("Room"));
  auto disabledRoom = std::dynamic_pointer_cast<RoomFrameSensor>(doc.scene.resolve("DisabledRoom"));
  check(wand && unknown && disabled && head && room && disabledRoom, "sensor clone missing");
  auto f = frame();
  f.devices.push_back({"HEAD", true, {{0, 1.7f, 0}, {0, 1, 0, 0}}, {true}, {1, 1}});
  system->submit(f);
  ctx.tick(1);
  check(wand->getIsActive() && room->getIsActive(), "valid frame did not activate sensors");
  check(wand->getButtons_changed() == f.devices[0].buttons, "held buttons missing");
  check(wand->getJoystick_changed() == f.devices[0].joystick, "joystick missing");
  check(head->getIsActive() && head->getButtons_changed().empty() &&
        head->getJoystick_changed() == SFVec2f{0,0}, "default HEAD must not expose controller controls");
  check(!unknown->getIsActive() && !disabled->getIsActive() && !disabledRoom->getIsActive(),
        "unknown or disabled sensor activated");
  check(disabled->getButtons_changed().empty(), "disabled sensor exposed buttons");
  const auto lastPose = wand->getPosition_changed();
  f.devices[0].valid = false;
  system->submit(f);
  ctx.tick(2);
  check(!wand->getIsActive() && room->getIsActive(), "device loss must not deactivate the room");
  check(wand->getButtons_changed() == MFBool(3, false), "loss left held buttons");
  check(wand->getJoystick_changed() == SFVec2f{0, 0}, "loss left joystick motion");
  near(wand->getPosition_changed(), lastPose);
  auto destination = std::dynamic_pointer_cast<x3d::nodes::Transform>(doc.scene.resolve("Destination"));
  check(ctx.writeField(destination.get(), "translation", SFVec3f{99,99,99}) == FieldWriteResult::Ok,
        "could not seed recovery destination");
  f.devices[0].valid = true;
  system->submit(f);
  ctx.tick(3);
  check(wand->getIsActive() && wand->getButtons_changed() == f.devices[0].buttons,
        "recovery did not republish complete state");
  near(destination->getTranslation(), lastPose);
  ctx.postEvent(wand.get(), "enabled", SFBool{false});
  ctx.postEvent(room.get(), "enabled", SFBool{false});
  ctx.tick(4);
  check(!wand->getIsActive() && !room->getIsActive(), "enabled FALSE did not deactivate");
  check(wand->getButtons_changed() == MFBool(3, false) && wand->getJoystick_changed() == SFVec2f{0,0},
        "disabled input left controls held");
  ctx.postEvent(wand.get(), "enabled", SFBool{true});
  ctx.postEvent(room.get(), "enabled", SFBool{true});
  ctx.tick(5);
  check(wand->getIsActive() && room->getIsActive(), "reenabling failed");
  system->detach(wand.get(), ctx);
  f.devices[0].roomFromDevice.position = {9, 9, 9};
  system->submit(f);
  ctx.tick(6);
  near(wand->getPosition_changed(), lastPose);
  system->attach(wand.get(), ctx);
  ctx.tick(7);
  near(wand->getPosition_changed(), {9, 9, 9});
  check(!unknown->getIsActive(), "unknown name did not stay inactive");
}

void input_diagnostics() {
  auto doc = document(R"(
    <ProtoInstance name='Tracked' DEF='Wand'><fieldValue name='device' value='WAND'/></ProtoInstance>
    <ProtoInstance name='RoomFrame' DEF='Room'/>)");
  X3DExecutionContext ctx(doc.scene.authorFields);
  auto system = setup(doc.scene, ctx);
  auto f = frame();
  int tick = 0;
  auto expect = [&](const char *message, const X3DNode *node) {
    system->submit(f); ctx.tick(tick++);
    bool found = false;
    for (const auto &warning : system->warnings())
      if (warning.node == node && warning.message.find(message) != std::string::npos)
        found = true;
    check(found, message);
  };
  auto wand = doc.scene.resolve("Wand");
  f.roomFromWorld.m[4] = 0.5f; expect("shear", nullptr);
  f.roomFromWorld = Mat4::scale({1,2,1}); expect("non-uniform scale", nullptr);
  f.roomFromWorld = Mat4::identity();
  f.roomFromWorld.m[0] = std::numeric_limits<float>::quiet_NaN();
  expect("non-finite matrix", nullptr);
  f.roomFromWorld = Mat4::identity();
  f.devices.clear(); expect("unknown device", wand.get());
  f = frame(); f.devices[0].valid = false; expect("host marked sample invalid", wand.get());
  f = frame(); f.devices[0].roomFromDevice.position.x = std::numeric_limits<float>::infinity();
  expect("non-finite pose", wand.get());
  f = frame(); f.devices[0].roomFromDevice.orientation = {0,0,0,1};
  expect("zero rotation axis", wand.get());
  f = frame(); f.devices[0].joystick.x = std::numeric_limits<float>::infinity();
  expect("non-finite joystick", wand.get());
  f = frame(); system->submit(f); ctx.tick(tick++);
  check(system->warnings().empty(), "recovered input retained stale diagnostics");
  check(std::dynamic_pointer_cast<TrackedDeviceSensor>(wand)->getIsActive(),
        "diagnosed device did not recover");
}

void invalid_room() {
  auto doc = document("<ProtoInstance name='RoomFrame' DEF='Room'/>");
  X3DExecutionContext ctx(doc.scene.authorFields);
  auto system = setup(doc.scene, ctx);
  auto sensor = std::dynamic_pointer_cast<RoomFrameSensor>(doc.scene.resolve("Room"));
  check(bool(sensor), "room sensor missing");
  auto f = frame();
  ctx.tick(0);
  check(!sensor->getIsActive(), "missing submission activated sensor");
  Mat4 shear = Mat4::identity(); shear.m[4] = 0.5f;
  Mat4 perspective = Mat4::identity(); perspective.m[3] = 0.1f;
  Mat4 nan = Mat4::identity(); nan.m[12] = std::numeric_limits<float>::quiet_NaN();
  Mat4 infinity = Mat4::identity(); infinity.m[0] = std::numeric_limits<float>::infinity();
  int tick = 1;
  for (const auto &bad : {shear, Mat4::scale({1,2,1}), Mat4::scale({0,0,0}),
                          Mat4::scale({-1,1,1}), perspective, nan, infinity}) {
    f.roomFromWorld = Mat4::translation({2, 3, 4});
    system->submit(f); ctx.tick(tick++);
    check(sensor->getIsActive(), "valid frame failed to recover");
    auto lastPose = sensor->getPosition_changed();
    f.roomFromWorld = bad;
    system->submit(f); ctx.tick(tick++);
    check(!sensor->getIsActive(), "invalid room matrix accepted");
    near(sensor->getPosition_changed(), lastPose);
  }
  f.roomFromWorld = Mat4::identity(); f.valid = false;
  system->submit(f); ctx.tick(tick++);
  check(!sensor->getIsActive(), "host-invalid room accepted");
  f.valid = true; f.roomFromWorld = Mat4::scale({2,2,2});
  system->submit(f); ctx.tick(tick);
  check(sensor->getIsActive(), "positive uniform scale rejected");
  near(sensor->getScale_changed(), {0.5f,0.5f,0.5f});
}

void room_placement() {
  auto doc = document(R"(
    <ProtoDeclare name='RoomWrapper'><ProtoBody>
      <ProtoInstance name='RoomGroup'/>
    </ProtoBody></ProtoDeclare>
    <Group DEF='Structural'><ProtoInstance name='RoomGroup' DEF='Good'/></Group>
    <Transform DEF='Parent'><ProtoInstance name='RoomGroup' DEF='Bad'/></Transform>
    <Billboard><ProtoInstance name='RoomGroup' DEF='BillboardRoom'/></Billboard>
    <Transform><ProtoInstance name='RoomWrapper' DEF='WrappedBad'/></Transform>
    <Group><ProtoInstance USE='Good' name='RoomGroup'/></Group>
  )");
  X3DExecutionContext ctx(doc.scene.authorFields);
  auto system = setup(doc.scene, ctx);
  system->submit(frame()); ctx.tick(1);
  check(system->warnings().size() == 3, "Transform, Billboard, and wrapped RoomGroup must warn");
  for (const auto &warning : system->warnings())
    check(warning.node && warning.node->getDEF() != "Good" && !warning.message.empty(),
          "warning must identify the invalid RoomGroup");
  // A shared RoomGroup is invalid if ANY path has a transforming ancestor.
  auto parent = doc.scene.resolve("Parent");
  auto good = doc.scene.resolve("Good");
  ctx.postEvent(parent.get(), "children", MFNode{good});
  ctx.tick(2);
  bool found = false;
  for (const auto &warning : system->warnings()) if (warning.node == good.get()) found = true;
  check(found, "USE or edited ancestor path escaped placement diagnosis");
}

// Count reflection visits at a scene root, independently of wall-clock timing.
// Direct system passes isolate diagnostic traversal from transform/bounds work.
class CountedGroup : public x3d::nodes::Group {
public:
  mutable unsigned visits = 0;
  const FieldTable &fields() const override {
    ++visits;
    return x3d::nodes::Group::fields();
  }
};

void placement_cache() {
  auto doc = document(R"(
    <Collision DEF='Plain'><ProtoInstance name='RoomGroup' DEF='Room'/></Collision>
    <Transform DEF='Parent'/>
    <LOD DEF='Levels'><Group/><Group/></LOD>
    <ProtoInstance name='Tracked' DEF='Wand'><fieldValue name='device' value='WAND'/></ProtoInstance>
  )");
  auto root = std::make_shared<CountedGroup>();
  root->setChildren(doc.scene.rootNodes);
  doc.scene.rootNodes = {root};
  X3DExecutionContext ctx(doc.scene.authorFields);
  auto system = setup(doc.scene, ctx);
  system->submit(frame());
  root->visits = 0;
  system->update(0, ctx);
  check(root->visits == 1, "initial placement diagnosis must walk once");
  for (int i = 0; i < 6; ++i) system->update(i / 2, ctx);
  check(root->visits == 1, "static updates/cascade passes repeated ancestry walk");
  auto parent = doc.scene.resolve("Parent");
  auto plain = doc.scene.resolve("Plain");
  auto room = doc.scene.resolve("Room");
  auto revision = ctx.sceneTopologyRevision();
  check(ctx.writeField(parent.get(), "translation", SFVec3f{1, 0, 0}) == FieldWriteResult::Ok,
        "TRS write failed");
  system->update(3, ctx);
  check(root->visits == 1 && ctx.sceneTopologyRevision() == revision,
        "TRS animation invalidated authored containment");
  ctx.markActiveChildChanged(doc.scene.resolve("Levels").get());
  ctx.setHeadPose({1, 2, 3}, {0, 1, 0, 0});
  system->update(3, ctx);
  check(root->visits == 1 && ctx.sceneTopologyRevision() == revision,
        "LOD selection or viewer movement invalidated authored containment");
  auto writeChildren = [&](X3DNode *node, MFNode children) {
    check(ctx.writeField(node, "children", std::move(children)) == FieldWriteResult::Ok,
          "child replacement failed");
    system->update(3, ctx); // Same time and tick generation: no once-per-tick guard.
  };
  writeChildren(parent.get(), {room});
  check(root->visits == 2 && system->warnings().size() == 1,
        "shared transformed/untransformed paths must warn after replacement");
  writeChildren(plain.get(), {});
  check(root->visits == 3 && system->warnings().size() == 1,
        "reparented RoomGroup lost warning");
  writeChildren(parent.get(), {});
  writeChildren(plain.get(), {room});
  check(root->visits == 5 && system->warnings().empty(), "reparenting retained warning");
  auto proxyParent = std::make_shared<x3d::nodes::Transform>();
  proxyParent->setChildren(MFNode{room});
  check(ctx.writeField(plain.get(), "proxy", SFNode{proxyParent}) == FieldWriteResult::Ok,
        "SFNode replacement failed");
  system->update(3, ctx);
  // Collision.proxy is collision-only (never rendered, X3D 23.4.2), so a
  // transformed proxy is not a render placement and must not warn.
  check(system->warnings().empty(), "non-rendered Collision.proxy path was diagnosed");
  check(ctx.writeField(plain.get(), "proxy", SFNode{}) == FieldWriteResult::Ok,
        "SFNode removal failed");
  system->update(3, ctx);
  check(system->warnings().empty(), "SFNode removal produced a placement warning");

  // A write delivered between system passes must be visible in the same tick.
  writeChildren(parent.get(), {});
  ctx.postEvent(parent.get(), "children", MFNode{room});
  ctx.tick(4);
  check(system->warnings().size() == 1, "cascade edit was not diagnosed within tick");
  auto invalid = frame();
  invalid.devices[0].valid = false;
  system->submit(invalid);
  system->update(5, ctx);
  check(system->warnings().size() == 2, "input warning replaced cached placement warning");
  system->submit(frame());
  root->visits = 0;
  system->update(5, ctx);
  check(root->visits == 0 && system->warnings().size() == 1,
        "device recovery must preserve placement cache without traversing");

  auto routes = doc.scene.resolvedProtoRoutes;
  doc.scene.resolvedProtoRoutes.clear();
  check(buildRoutes(doc.scene, ctx).rejected.empty(), "route removal failed");
  root->visits = 0;
  system->update(5, ctx);
  check(root->visits == 0 && system->warnings().empty(), "empty anchors walked scene");
  doc.scene.resolvedProtoRoutes = routes;
  check(buildRoutes(doc.scene, ctx).rejected.empty(), "route restoration failed");
  system->update(5, ctx);
  check(root->visits == 1 && system->warnings().size() == 1,
        "private route replacement did not rebuild anchors");
  ctx.detachNodes({room.get()});
  check(system->warnings().empty(), "detach left borrowed warning pointers");
}

void placement_idle() {
  Scene scene;
  auto root = std::make_shared<CountedGroup>();
  root->setChildren(MFNode{std::make_shared<x3d::nodes::Transform>()});
  scene.rootNodes = {root};
  X3DExecutionContext ctx;
  auto system = installCaveInput(scene, ctx);
  // No graph systems/indexes: count only work caused by CaveInputSystem in tick.
  root->visits = 0;
  ctx.tick(0);
  ctx.tick(1);
  check(root->visits == 0, "idle scene without CAVE sensors traversed ancestry");
  check(system->warnings().empty(), "scene without CAVE sensors diagnosed input");
  auto sensor = std::make_shared<RoomFrameSensor>();
  system->attach(sensor.get(), ctx);
  system->submit(frame());
  ctx.tick(2);
  check(root->visits == 0, "sensor without RoomGroup anchors traversed ancestry");
  system->detach(sensor.get(), ctx);
  ctx.tick(3);
  check(root->visits == 0 && system->warnings().empty(), "last detach retained work");
}

void placement_inline() {
  auto doc = document("<Transform><Inline DEF='Deferred' load='false' url='&quot;room.x3d&quot;'/></Transform>");
  unsigned loads = 0;
  InlineResolver resolver = [&](const auto &, const auto &) {
    ++loads;
    auto child = document(R"(
      <ProtoDeclare name='Wrapper'><ProtoBody><ProtoInstance name='RoomGroup'/></ProtoBody></ProtoDeclare>
      <ProtoInstance name='Wrapper' DEF='LoadedRoom'/>)");
    return std::make_shared<Scene>(std::move(child.scene));
  };
  X3DExecutionContext ctx(doc.scene.authorFields);
  auto loader = std::make_shared<InlineRuntimeSystem>(doc.scene, resolver, "");
  auto inl = doc.scene.resolve("Deferred");
  loader->attach(inl.get(), ctx);
  ctx.addSystem(loader);
  auto system = setup(doc.scene, ctx);
  system->submit(frame());
  ctx.tick(0);
  check(loads == 0 && system->warnings().empty(), "deferred Inline loaded early");
  ctx.postEvent(inl.get(), "load", true);
  ctx.tick(1);
  check(loads == 1 && system->warnings().size() == 1,
        "deferred Inline/enclosing PROTO RoomGroup was not enrolled");
  std::weak_ptr<X3DNode> retired;
  for (const auto &route : doc.scene.resolvedInlineRoutes)
    if (route.to.get() == system->warnings().front().node) retired = route.to;
  check(!retired.expired(), "test did not locate hoisted RoomGroup route");
  check(ctx.writeField(inl.get(), "load", false) == FieldWriteResult::Ok, "unload failed");
  loader->update(1, ctx);
  check(system->warnings().empty(), "unload did not drop borrowed warnings immediately");
  system->update(1, ctx);
  check(retired.expired() && system->warnings().empty(), "unload retained stale placement");
  ctx.postEvent(inl.get(), "load", true);
  ctx.tick(2);
  check(loads == 2 && system->warnings().size() == 1, "reload did not diagnose new RoomGroup");
}

void placement_geolod() {
  auto doc = document(R"(
    <Transform><GeoLOD DEF='Tiles' geoSystem='"GC"' center='0 0 0'
        range='0' child1Url='"room.x3d"'>
      <Group containerField='rootNode'/>
    </GeoLOD></Transform>)");
  unsigned loads = 0;
  InlineResolver resolver = [&](const auto &, const auto &) {
    ++loads;
    auto child = document("<ProtoInstance name='RoomGroup'/>");
    return std::make_shared<Scene>(std::move(child.scene));
  };
  X3DExecutionContext ctx(doc.scene.authorFields);
  auto loader = std::make_shared<InlineRuntimeSystem>(doc.scene, resolver, "");
  auto lod = doc.scene.resolve("Tiles");
  loader->attach(lod.get(), ctx);
  ctx.addSystem(loader);
  auto system = setup(doc.scene, ctx);
  system->submit(frame());
  ctx.tick(0);
  check(loads == 0 && system->warnings().empty(), "far GeoLOD loaded child tile");
  ctx.postEvent(lod.get(), "range", SFFloat{1000000});
  ctx.tick(1);
  check(loads == 1 && system->warnings().size() == 1,
        "GeoLOD tile RoomGroup was not enrolled");
  std::weak_ptr<X3DNode> retired;
  for (const auto &route : doc.scene.resolvedInlineRoutes)
    if (route.to.get() == system->warnings().front().node) retired = route.to;
  check(!retired.expired(), "test did not locate GeoLOD RoomGroup");
  ctx.postEvent(lod.get(), "range", SFFloat{0});
  ctx.tick(2);
  check(retired.expired() && system->warnings().empty(), "GeoLOD retirement retained warning");
  ctx.postEvent(lod.get(), "range", SFFloat{1000000});
  ctx.tick(3);
  check(loads == 2 && system->warnings().size() == 1,
        "GeoLOD tile reload did not rebuild placement warnings");
}

// Room about X, device about Y: these rotations do not commute, so composing
// them in the wrong order produces a different world orientation.
void room_rotation_order() {
  auto doc = document("<ProtoInstance name='Tracked' DEF='Wand'><fieldValue name='device' value='WAND'/></ProtoInstance>");
  X3DExecutionContext ctx(doc.scene.authorFields);
  auto system = setup(doc.scene, ctx);
  auto f = frame();
  f.roomFromWorld = Mat4::translation({1, 2, 3}) * Mat4::rotation({1, 0, 0, 0.7f}) *
                    Mat4::scale({2, 2, 2});
  f.devices[0].roomFromDevice.orientation = {0, 1, 0, 0.9f};
  system->submit(f);
  ctx.tick(1);
  auto wand = std::dynamic_pointer_cast<TrackedDeviceSensor>(doc.scene.resolve("Wand"));
  check(wand && wand->getIsActive(), "wand inactive");
  const Mat4 worldFromRoom = f.roomFromWorld.inverse();
  const Mat4 roomRotation = Mat4::rotation({1, 0, 0, -0.7f}); // worldFromRoom's rotation.
  const Mat4 device = Mat4::rotation(f.devices[0].roomFromDevice.orientation);
  const SFVec3f forward{0, 0, -1};
  const SFVec3f expected = (roomRotation * device).transformDirection(forward);
  const SFVec3f reversed = (device * roomRotation).transformDirection(forward);
  check(std::fabs(expected.x - reversed.x) + std::fabs(expected.y - reversed.y) +
        std::fabs(expected.z - reversed.z) > 0.1f, "test rotations commute");
  near(Mat4::rotation(wand->getOrientation_changed()).transformDirection(forward), expected);
  near(wand->getPosition_changed(), worldFromRoom.transformPoint({1, 2, -3}));
}

// Float-accumulated navigation drifts past 1e-5 relative error; the frame is
// re-orthonormalized instead of freezing, while real shear is still rejected.
void room_drift() {
  auto doc = document(std::string(roomChild) + "<ProtoInstance name='RoomFrame' DEF='Room'/>");
  X3DExecutionContext ctx(doc.scene.authorFields);
  auto system = setup(doc.scene, ctx);
  extract::SceneExtractor extractor(ctx, doc.scene);
  auto room = std::dynamic_pointer_cast<RoomFrameSensor>(doc.scene.resolve("Room"));
  check(bool(room), "room sensor missing");
  // Accumulate many small float steps, then add a deliberate 3e-4 skew and a
  // 2e-4 column-length mismatch: both beyond 1e-5, both within 1e-3.
  Mat4 nav = Mat4::identity();
  for (int i = 0; i < 5000; ++i)
    nav = Mat4::rotation({0.3f, 1, 0.2f, 0.0021f}) * Mat4::translation({0.0002f, 0, -0.0001f}) * nav;
  nav.m[1] += 3e-4f;
  for (int r = 0; r < 3; ++r) nav.m[8 + r] *= 1.0002f;
  double dot = 0, a = 0, b = 0;
  for (int r = 0; r < 3; ++r) {
    dot += double(nav.m[r]) * nav.m[4 + r];
    a += double(nav.m[r]) * nav.m[r];
    b += double(nav.m[4 + r]) * nav.m[4 + r];
  }
  check(std::fabs(dot) > 1e-5 * std::sqrt(a * b), "drift fixture is too clean to test tolerance");
  auto f = frame();
  f.roomFromWorld = nav;
  system->submit(f);
  ctx.tick(1);
  check(room->getIsActive(), "float-drifted room frame froze the room");
  check(system->warnings().empty(), "drifted room frame reported a rejection");
  const auto delta = extractor.fullSnapshot();
  check(delta.added.size() == 1, "expected one RoomGroup child placement");
  const Mat4 child = Mat4::translation({0.5f, 1, -3}) * Mat4::rotation({0, 1, 0, 0.2f});
  const Mat4 roomSpace = nav * extractor.item(delta.added.front()).worldTransform;
  for (std::size_t i = 0; i < roomSpace.m.size(); ++i)
    check(std::fabs(roomSpace.m[i] - child.m[i]) < 5e-3f, "re-orthonormalized frame misplaced room child");
  const auto lastPose = room->getPosition_changed();
  Mat4 shear = nav;
  shear.m[4] += 5e-3f;
  f.roomFromWorld = shear;
  system->submit(f);
  ctx.tick(2);
  check(!room->getIsActive(), "real shear accepted");
  near(room->getPosition_changed(), lastPose);
  Mat4 stretch = nav;
  for (int r = 0; r < 3; ++r) stretch.m[r] *= 1.005f;
  f.roomFromWorld = stretch;
  system->submit(f);
  ctx.tick(3);
  check(!room->getIsActive(), "real non-uniform scale accepted");
}

// Only render-graph placement fields count; references and non-rendered slots
// do not place a RoomGroup under a transform. Layers warn separately.
void placement_fields() {
  auto doc = document(R"(
    <ProtoDeclare name='Impostor'><ProtoBody>
      <Group>
        <ProtoInstance name='RoomFrame' DEF='RoomGroupFrame'/>
        <Transform DEF='Fake'/>
      </Group>
      <ROUTE fromNode='RoomGroupFrame' fromField='position_changed' toNode='Fake' toField='translation'/>
    </ProtoBody></ProtoDeclare>
    <ProtoInstance name='RoomGroup' DEF='Root'/>
    <Transform>
      <TransformSensor size='1 1 1'>
        <ProtoInstance USE='Root' name='RoomGroup' containerField='targetObject'/>
      </TransformSensor>
      <ProtoInstance name='Impostor'/>
    </Transform>
    <Collision><Transform containerField='proxy'>
      <ProtoInstance name='RoomGroup' DEF='ProxyRoom'/>
    </Transform></Collision>
    <CADFace><Transform containerField='shape'>
      <ProtoInstance name='RoomGroup' DEF='CadRoom'/>
    </Transform></CADFace>
    <LayerSet order='0 1'>
      <Layer/>
      <Layer><ProtoInstance name='RoomGroup' DEF='LayerRoom'/></Layer>
    </LayerSet>
  )");
  bool impostor = false;
  for (const auto &route : doc.scene.resolvedProtoRoutes)
    if (dynamic_cast<RoomFrameSensor *>(route.from.get()) &&
        route.from->getDEF() == "RoomGroupFrame" && route.to->getDEF() == "Fake")
      impostor = true;
  check(impostor, "test must exercise an author sensor reusing RoomGroup's private DEF");
  X3DExecutionContext ctx(doc.scene.authorFields);
  auto system = setup(doc.scene, ctx);
  system->submit(frame());
  ctx.tick(1);
  const auto cad = doc.scene.resolve("CadRoom");
  const auto layer = doc.scene.resolve("LayerRoom");
  check(cad && layer, "RoomGroup primaries missing");
  bool cadWarned = false, layerWarned = false;
  for (const auto &warning : system->warnings()) {
    std::cerr << "warning: " << warning.message << '\n';
    if (warning.node == cad.get()) cadWarned = true;
    else if (warning.node == layer.get() && warning.message.find("Layer") != std::string::npos)
      layerWarned = true;
    else check(false, "reference, proxy, or impostor path produced a placement warning");
  }
  check(cadWarned, "CADFace.shape placement under a Transform did not warn");
  check(layerWarned, "RoomGroup inside a Layer did not warn");
  check(system->warnings().size() == 2, "expected exactly the CADFace and Layer warnings");
}

// With the RuntimeSession order (Inline loader first, CAVE input after it),
// sensors in a freshly loaded Inline report in the load tick itself.
void placement_inline_same_tick() {
  auto doc = document("<Inline DEF='Deferred' load='false' url='&quot;room.x3d&quot;'/>");
  InlineResolver resolver = [&](const auto &, const auto &) {
    auto child = document("<ProtoInstance name='Tracked' DEF='W'><fieldValue name='device' value='WAND'/></ProtoInstance>");
    return std::make_shared<Scene>(std::move(child.scene));
  };
  X3DExecutionContext ctx(doc.scene.authorFields);
  auto loader = std::make_shared<InlineRuntimeSystem>(doc.scene, resolver, "");
  auto inl = doc.scene.resolve("Deferred");
  loader->attach(inl.get(), ctx);
  ctx.addSystem(loader);
  auto system = setup(doc.scene, ctx);
  system->submit(frame());
  ctx.tick(0);
  ctx.postEvent(inl.get(), "load", true);
  ctx.tick(1);
  std::size_t sensors = 0, active = 0;
  x3d::runtime::detail::forEachNode(doc.scene, [&](X3DNode *node) {
    if (auto *sensor = dynamic_cast<TrackedDeviceSensor *>(node)) {
      ++sensors;
      if (sensor->getIsActive()) ++active;
    }
  });
  check(sensors == 1 && active == 1, "Inline-loaded sensor did not activate in its load tick");
}

void fallback() {
  const auto source = xml(std::string(roomChild) + R"(
    <ProtoInstance name='Tracked' DEF='Wand'><fieldValue name='device' value='WAND'/></ProtoInstance>
    <ProtoInstance name='RoomFrame' DEF='Room'/>
    <Transform DEF='Destination'/>
    <ROUTE fromNode='Wand' fromField='position_changed' toNode='Destination' toField='set_translation'/>
  )");
  // No ext resolver: skip URNs and actually read the second URL from disk.
  auto doc = parseDocument(source, Encoding::XML, std::string(X3D_EXT_PROTO_DIR) + "/");
  check(doc.protoWarnings.empty(), "portable fallback URL did not load");
  X3DExecutionContext ctx(doc.scene.authorFields);
  check(buildRoutes(doc.scene, ctx).rejected.empty(), "fallback output endpoint not routable");
  ctx.buildSceneGraph(doc.scene);
  ctx.tick(1);
  x3d::runtime::detail::forEachNode(doc.scene, [](X3DNode *node) {
    check(!dynamic_cast<TrackedDeviceSensor *>(node) && !dynamic_cast<RoomFrameSensor *>(node),
          "fallback unexpectedly instantiated a native extension");
  });
  check(doc.scene.resolve("Wand")->nodeTypeName() == "Group", "wrong inactive sensor fallback");
  check(doc.scene.resolve("Room")->nodeTypeName() == "Group", "wrong room sensor fallback");
  extract::SceneExtractor extractor(ctx, doc.scene);
  auto delta = extractor.fullSnapshot();
  check(delta.added.size() == 1, "fallback dropped RoomGroup children");
  near(extractor.item(delta.added.front()).worldTransform,
       Mat4::translation({0.5f,1,-3}) * Mat4::rotation({0,1,0,0.2f}));
  auto dest = std::dynamic_pointer_cast<x3d::nodes::Transform>(doc.scene.resolve("Destination"));
  near(dest->getTranslation(), {0,0,0});
  auto encoded = XmlWriter{}.writeDocument(doc);
  auto again = parseDocument(encoded, Encoding::XML, std::string(X3D_EXT_PROTO_DIR) + "/");
  check(again.protoWarnings.empty(), "fallback round-trip cannot load fallback URL");
}
} // namespace

int main(int argc, char **argv) {
  try {
    check(argc == 2, "expected one case name");
    const std::string name = argv[1];
    if (name == "recursive_resolver") recursive_resolver();
    else if (name == "input_diagnostics") input_diagnostics();
    else if (name == "proto_routes") proto_routes();
    else if (name == "room_same_tick") room_same_tick();
    else if (name == "tracking_lifecycle") tracking_lifecycle();
    else if (name == "invalid_room") invalid_room();
    else if (name == "room_placement") room_placement();
    else if (name == "placement_cache") placement_cache();
    else if (name == "placement_idle") placement_idle();
    else if (name == "placement_inline") placement_inline();
    else if (name == "placement_geolod") placement_geolod();
    else if (name == "fallback") fallback();
    else if (name == "room_rotation_order") room_rotation_order();
    else if (name == "room_drift") room_drift();
    else if (name == "placement_fields") placement_fields();
    else if (name == "placement_inline_same_tick") placement_inline_same_tick();
    else throw std::runtime_error("unknown case");
    std::cout << name << ": PASS\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
}
