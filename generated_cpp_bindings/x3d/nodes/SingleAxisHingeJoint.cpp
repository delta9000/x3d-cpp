// SingleAxisHingeJoint.cpp
#include "x3d/nodes/SingleAxisHingeJoint.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string SingleAxisHingeJoint::nodeTypeName() const {
  return "SingleAxisHingeJoint";
}

std::string SingleAxisHingeJoint::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &SingleAxisHingeJoint::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "anchorPoint", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SingleAxisHingeJoint &>(n).getAnchorPoint());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SingleAxisHingeJoint &>(n).setAnchorPoint(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "angle", X3DFieldType::SFFloat, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SingleAxisHingeJoint &>(n).getAngle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SingleAxisHingeJoint &>(n).emitAngle(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "angleRate", X3DFieldType::SFFloat, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SingleAxisHingeJoint &>(n).getAngleRate());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SingleAxisHingeJoint &>(n).emitAngleRate(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "axis", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SingleAxisHingeJoint &>(n).getAxis());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SingleAxisHingeJoint &>(n).setAxis(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "body1", X3DFieldType::SFNode, AccessType::InputOutput, "body1",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SingleAxisHingeJoint &>(n)
                              .X3DRigidJointNode::getBody1());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SingleAxisHingeJoint &>(n).X3DRigidJointNode::setBody1(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "body1AnchorPoint", X3DFieldType::SFVec3f, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SingleAxisHingeJoint &>(n)
                              .getBody1AnchorPoint());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SingleAxisHingeJoint &>(n).emitBody1AnchorPoint(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "body2", X3DFieldType::SFNode, AccessType::InputOutput, "body2",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SingleAxisHingeJoint &>(n)
                              .X3DRigidJointNode::getBody2());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SingleAxisHingeJoint &>(n).X3DRigidJointNode::setBody2(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "body2AnchorPoint", X3DFieldType::SFVec3f, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SingleAxisHingeJoint &>(n)
                              .getBody2AnchorPoint());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SingleAxisHingeJoint &>(n).emitBody2AnchorPoint(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "forceOutput", X3DFieldType::MFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SingleAxisHingeJoint &>(n)
                              .X3DRigidJointNode::getForceOutput());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SingleAxisHingeJoint &>(n)
              .X3DRigidJointNode::setForceOutput(std::any_cast<MFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SingleAxisHingeJoint &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SingleAxisHingeJoint &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "maxAngle", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SingleAxisHingeJoint &>(n).getMaxAngle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SingleAxisHingeJoint &>(n).setMaxAngle(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SingleAxisHingeJoint &>(n)
                              .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SingleAxisHingeJoint &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "minAngle", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SingleAxisHingeJoint &>(n).getMinAngle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SingleAxisHingeJoint &>(n).setMinAngle(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "stopBounce", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SingleAxisHingeJoint &>(n).getStopBounce());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SingleAxisHingeJoint &>(n).setStopBounce(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "stopErrorCorrection", X3DFieldType::SFFloat, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SingleAxisHingeJoint &>(n)
                              .getStopErrorCorrection());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SingleAxisHingeJoint &>(n).setStopErrorCorrection(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SingleAxisHingeJoint &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SingleAxisHingeJoint &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SingleAxisHingeJoint &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SingleAxisHingeJoint &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SingleAxisHingeJoint &>(n)
                              .X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SingleAxisHingeJoint &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SingleAxisHingeJoint &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SingleAxisHingeJoint &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SingleAxisHingeJoint &>(n)
                              .X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SingleAxisHingeJoint &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void SingleAxisHingeJoint::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

namespace factory_detail {
std::shared_ptr<X3DNode> createSingleAxisHingeJoint() {
  return std::make_shared<SingleAxisHingeJoint>();
}
} // namespace factory_detail

} // namespace x3d::nodes
