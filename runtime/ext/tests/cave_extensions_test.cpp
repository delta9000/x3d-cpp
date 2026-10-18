// Standalone ext-gated cases. Each CTest case selects one function by argument.
// Checks remain active in Release builds; no assertions with side effects.
#include "x3d/ext.hpp"
#include "SceneExtractor.hpp"
#include "X3DCodecs.hpp"
#include "X3DParse.hpp"
#include "x3d/nodes/Transform.hpp"

#include <any>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
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
    if (name == "proto_routes") proto_routes();
    else if (name == "room_same_tick") room_same_tick();
    else if (name == "tracking_lifecycle") tracking_lifecycle();
    else if (name == "invalid_room") invalid_room();
    else if (name == "room_placement") room_placement();
    else if (name == "fallback") fallback();
    else throw std::runtime_error("unknown case");
    std::cout << name << ": PASS\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
}
