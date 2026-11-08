// Opt-in physical device and room-frame sensors. Core never includes this file.
#ifndef X3D_RUNTIME_EXT_CAVE_SENSORS_HPP
#define X3D_RUNTIME_EXT_CAVE_SENSORS_HPP

#include "x3d/nodes/X3DSensorNode.hpp"

#include <any>
#include <string>

namespace x3d::runtime::ext {
using namespace x3d::core;
using x3d::nodes::X3DNode;

namespace cave_detail {
// The generated sensor base supplies enabled, description, isActive, metadata,
// and the ordinary node fields. Only extension fields need new descriptors.
template <class Node, class Value>
FieldInfo field(const char *name, X3DFieldType type, AccessType access,
                const Value &(Node::*get)() const,
                void (Node::*set)(const Value &)) {
  return {name, type, access, "",
          [get](const X3DNode &node) -> std::any {
            return (dynamic_cast<const Node &>(node).*get)();
          },
          [set](X3DNode &node, const std::any &value) {
            (dynamic_cast<Node &>(node).*set)(std::any_cast<const Value &>(value));
          }, nullptr, nullptr};
}
} // namespace cave_detail

class TrackedDeviceSensor : public x3d::nodes::X3DSensorNode {
public:
  const SFString &getDevice() const { return device_; }
  void setDevice(const SFString &value) { device_ = value; }
  const SFVec3f &getPosition_changed() const { return position_; }
  void emitPosition_changed(const SFVec3f &value) { position_ = value; }
  const SFRotation &getOrientation_changed() const { return orientation_; }
  void emitOrientation_changed(const SFRotation &value) { orientation_ = value; }
  const SFVec3f &getRoomPosition_changed() const { return roomPosition_; }
  void emitRoomPosition_changed(const SFVec3f &value) { roomPosition_ = value; }
  const SFRotation &getRoomOrientation_changed() const { return roomOrientation_; }
  void emitRoomOrientation_changed(const SFRotation &value) { roomOrientation_ = value; }
  const MFBool &getButtons_changed() const { return buttons_; }
  void emitButtons_changed(const MFBool &value) { buttons_ = value; }
  const SFVec2f &getJoystick_changed() const { return joystick_; }
  void emitJoystick_changed(const SFVec2f &value) { joystick_ = value; }

  std::string nodeTypeName() const override { return "TrackedDeviceSensor"; }
  std::string defaultContainerField() const override { return "children"; }
  const FieldTable &fields() const override {
    static const FieldTable table = [this] {
      FieldTable t = X3DSensorNode::fields();
      using N = TrackedDeviceSensor;
      using cave_detail::field;
      t.push_back(field("device", X3DFieldType::SFString, AccessType::InitializeOnly,
                        &N::getDevice, &N::setDevice));
      t.push_back(field("position_changed", X3DFieldType::SFVec3f, AccessType::OutputOnly,
                        &N::getPosition_changed, &N::emitPosition_changed));
      t.push_back(field("orientation_changed", X3DFieldType::SFRotation, AccessType::OutputOnly,
                        &N::getOrientation_changed, &N::emitOrientation_changed));
      t.push_back(field("roomPosition_changed", X3DFieldType::SFVec3f, AccessType::OutputOnly,
                        &N::getRoomPosition_changed, &N::emitRoomPosition_changed));
      t.push_back(field("roomOrientation_changed", X3DFieldType::SFRotation, AccessType::OutputOnly,
                        &N::getRoomOrientation_changed, &N::emitRoomOrientation_changed));
      t.push_back(field("buttons_changed", X3DFieldType::MFBool, AccessType::OutputOnly,
                        &N::getButtons_changed, &N::emitButtons_changed));
      t.push_back(field("joystick_changed", X3DFieldType::SFVec2f, AccessType::OutputOnly,
                        &N::getJoystick_changed, &N::emitJoystick_changed));
      return t;
    }();
    return table;
  }
  void accept(NodeVisitor &visitor) const override {
    if (!visitor.enter(*this)) return;
    visitor.leave(*this);
  }

private:
  SFString device_{"HEAD"};
  SFVec3f position_{0, 0, 0}, roomPosition_{0, 0, 0};
  SFRotation orientation_{0, 1, 0, 0}, roomOrientation_{0, 1, 0, 0};
  MFBool buttons_;
  SFVec2f joystick_{0, 0};
};

class RoomFrameSensor : public x3d::nodes::X3DSensorNode {
public:
  const SFVec3f &getPosition_changed() const { return position_; }
  void emitPosition_changed(const SFVec3f &value) { position_ = value; }
  const SFRotation &getOrientation_changed() const { return orientation_; }
  void emitOrientation_changed(const SFRotation &value) { orientation_ = value; }
  const SFVec3f &getScale_changed() const { return scale_; }
  void emitScale_changed(const SFVec3f &value) { scale_ = value; }

  std::string nodeTypeName() const override { return "RoomFrameSensor"; }
  std::string defaultContainerField() const override { return "children"; }
  const FieldTable &fields() const override {
    static const FieldTable table = [this] {
      FieldTable t = X3DSensorNode::fields();
      using N = RoomFrameSensor;
      using cave_detail::field;
      t.push_back(field("position_changed", X3DFieldType::SFVec3f, AccessType::OutputOnly,
                        &N::getPosition_changed, &N::emitPosition_changed));
      t.push_back(field("orientation_changed", X3DFieldType::SFRotation, AccessType::OutputOnly,
                        &N::getOrientation_changed, &N::emitOrientation_changed));
      t.push_back(field("scale_changed", X3DFieldType::SFVec3f, AccessType::OutputOnly,
                        &N::getScale_changed, &N::emitScale_changed));
      return t;
    }();
    return table;
  }
  void accept(NodeVisitor &visitor) const override {
    if (!visitor.enter(*this)) return;
    visitor.leave(*this);
  }

private:
  SFVec3f position_{0, 0, 0}, scale_{1, 1, 1};
  SFRotation orientation_{0, 1, 0, 0};
};

namespace cave_detail {
// RoomGroup's private auxiliary sensor. CaveInputSystem identifies RoomGroup
// anchors by this type, not by a DEF name an author could reuse. Only the
// RoomGroup declaration's own clone factory creates it; no parser, generated
// factory, or other declaration knows the type name.
class RoomGroupFrameSensor final : public RoomFrameSensor {
public:
  std::string nodeTypeName() const override { return "RoomGroupFrameSensor"; }
};
} // namespace cave_detail

} // namespace x3d::runtime::ext
#endif
