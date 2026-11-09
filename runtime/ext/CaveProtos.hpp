#ifndef X3D_RUNTIME_EXT_CAVE_PROTOS_HPP
#define X3D_RUNTIME_EXT_CAVE_PROTOS_HPP

#include "CaveSensors.hpp"
#include "X3DProto.hpp"
#include "x3d/nodes/Transform.hpp"

#include <initializer_list>
#include <memory>
#include <utility>

namespace x3d::runtime::ext {
inline constexpr const char *kTrackedDeviceSensorUrn =
    "urn:x3d-cpp-gen:ext:TrackedDeviceSensor:1";
inline constexpr const char *kRoomFrameSensorUrn =
    "urn:x3d-cpp-gen:ext:RoomFrameSensor:1";
inline constexpr const char *kRoomGroupUrn = "urn:x3d-cpp-gen:ext:RoomGroup:1";

namespace cave_detail {
inline std::shared_ptr<X3DNode> createSensor(const std::string &type) {
  if (type == "TrackedDeviceSensor") return std::make_shared<TrackedDeviceSensor>();
  if (type == "RoomFrameSensor") return std::make_shared<RoomFrameSensor>();
  if (type == "RoomGroupFrameSensor") return std::make_shared<RoomGroupFrameSensor>();
  return nullptr;
}

inline void expose(ProtoDeclaration &decl, const std::shared_ptr<X3DNode> &body,
                   std::initializer_list<const char *> names) {
  for (const char *name : names)
    for (const FieldInfo &info : body->fields()) {
      if (info.x3dName != name) continue;
      ProtoField field;
      field.name = name;
      field.type = info.type;
      field.access = info.access;
      if (info.access == AccessType::InitializeOnly || info.access == AccessType::InputOutput) {
        if (info.type != X3DFieldType::SFNode && info.type != X3DFieldType::MFNode)
          field.value = info.get(*body);
      }
      decl.interface.push_back(std::move(field));
      decl.body.isConnections.push_back({body, name, name});
      break;
    }
}
} // namespace cave_detail

inline std::shared_ptr<ProtoDeclaration> makeTrackedDeviceSensorProto() {
  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "TrackedDeviceSensor";
  decl->createNode = cave_detail::createSensor;
  auto node = std::make_shared<TrackedDeviceSensor>();
  decl->body.nodes.push_back(node);
  cave_detail::expose(*decl, node, {"metadata", "description", "enabled", "device",
      "isActive", "position_changed", "orientation_changed", "roomPosition_changed",
      "roomOrientation_changed", "buttons_changed", "joystick_changed"});
  return decl;
}

inline std::shared_ptr<ProtoDeclaration> makeRoomFrameSensorProto() {
  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "RoomFrameSensor";
  decl->createNode = cave_detail::createSensor;
  auto node = std::make_shared<RoomFrameSensor>();
  decl->body.nodes.push_back(node);
  cave_detail::expose(*decl, node, {"metadata", "description", "enabled", "isActive",
      "position_changed", "orientation_changed", "scale_changed"});
  return decl;
}

// Standard Transform semantics throughout: this is a PROTO, not a RoomGroup
// native class. The auxiliary sensor is enrolled through scene.protoPeerNodes.
inline std::shared_ptr<ProtoDeclaration> makeRoomGroupProto() {
  auto decl = std::make_shared<ProtoDeclaration>();
  decl->name = "RoomGroup";
  decl->createNode = cave_detail::createSensor;
  auto transform = std::make_shared<x3d::nodes::Transform>();
  transform->setDEF("RoomTransform");
  // The private sensor type, not this DEF, marks the anchor (CaveInputSystem).
  auto sensor = std::make_shared<cave_detail::RoomGroupFrameSensor>();
  sensor->setDEF("RoomGroupFrame");
  decl->body.nodes = {transform, sensor};
  cave_detail::expose(*decl, transform, {"metadata", "children", "addChildren",
      "removeChildren", "visible", "bboxDisplay", "bboxCenter", "bboxSize"});
  for (const auto &[from, to] : {std::pair{"position_changed", "translation"},
                               std::pair{"orientation_changed", "rotation"},
                               std::pair{"scale_changed", "scale"}}) {
    Route route;
    route.fromNode = "RoomGroupFrame";
    route.fromField = from;
    route.toNode = "RoomTransform";
    route.toField = to;
    decl->body.routes.push_back(std::move(route));
  }
  return decl;
}
} // namespace x3d::runtime::ext
#endif
