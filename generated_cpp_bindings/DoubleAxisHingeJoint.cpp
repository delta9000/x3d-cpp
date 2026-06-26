// DoubleAxisHingeJoint.cpp
#include "DoubleAxisHingeJoint.hpp"

std::string DoubleAxisHingeJoint::nodeTypeName() const {
  return "DoubleAxisHingeJoint";
}

std::string DoubleAxisHingeJoint::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &DoubleAxisHingeJoint::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "anchorPoint", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const DoubleAxisHingeJoint &>(n).getAnchorPoint());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DoubleAxisHingeJoint &>(n).setAnchorPoint(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "axis1", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const DoubleAxisHingeJoint &>(n).getAxis1());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DoubleAxisHingeJoint &>(n).setAxis1(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "axis2", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const DoubleAxisHingeJoint &>(n).getAxis2());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DoubleAxisHingeJoint &>(n).setAxis2(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "body1", X3DFieldType::SFNode, AccessType::InputOutput, "body1",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const DoubleAxisHingeJoint &>(n)
                              .X3DRigidJointNode::getBody1());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DoubleAxisHingeJoint &>(n).X3DRigidJointNode::setBody1(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "body1AnchorPoint", X3DFieldType::SFVec3f, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const DoubleAxisHingeJoint &>(n)
                              .getBody1AnchorPoint());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DoubleAxisHingeJoint &>(n).emitBody1AnchorPoint(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "body1Axis", X3DFieldType::SFVec3f, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const DoubleAxisHingeJoint &>(n).getBody1Axis());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DoubleAxisHingeJoint &>(n).emitBody1Axis(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "body2", X3DFieldType::SFNode, AccessType::InputOutput, "body2",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const DoubleAxisHingeJoint &>(n)
                              .X3DRigidJointNode::getBody2());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DoubleAxisHingeJoint &>(n).X3DRigidJointNode::setBody2(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "body2AnchorPoint", X3DFieldType::SFVec3f, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const DoubleAxisHingeJoint &>(n)
                              .getBody2AnchorPoint());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DoubleAxisHingeJoint &>(n).emitBody2AnchorPoint(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "body2Axis", X3DFieldType::SFVec3f, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const DoubleAxisHingeJoint &>(n).getBody2Axis());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DoubleAxisHingeJoint &>(n).emitBody2Axis(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "desiredAngularVelocity1", X3DFieldType::SFFloat,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const DoubleAxisHingeJoint &>(n)
                              .getDesiredAngularVelocity1());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DoubleAxisHingeJoint &>(n).setDesiredAngularVelocity1(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "desiredAngularVelocity2", X3DFieldType::SFFloat,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const DoubleAxisHingeJoint &>(n)
                              .getDesiredAngularVelocity2());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DoubleAxisHingeJoint &>(n).setDesiredAngularVelocity2(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "forceOutput", X3DFieldType::MFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const DoubleAxisHingeJoint &>(n)
                              .X3DRigidJointNode::getForceOutput());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DoubleAxisHingeJoint &>(n)
              .X3DRigidJointNode::setForceOutput(
                  std::any_cast<std::vector<ForceOutputValues>>(v));
        },

        [](const X3DNode &n) -> std::string {
          const auto &vec = dynamic_cast<const DoubleAxisHingeJoint &>(n)
                                .X3DRigidJointNode::getForceOutput();
          std::string out;
          for (std::size_t i = 0; i < vec.size(); ++i) {
            if (i)
              out += ' ';
            out += to_string(vec[i]);
          }
          return out;
        },

        [](X3DNode &n, const std::string &s) {
          std::vector<ForceOutputValues> vec;
          std::size_t i = 0;
          while (i < s.size()) {
            while (i < s.size() &&
                   (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' ||
                    s[i] == '\r' || s[i] == ','))
              ++i;
            std::size_t j = i;
            while (j < s.size() && s[j] != ' ' && s[j] != '\t' &&
                   s[j] != '\n' && s[j] != '\r' && s[j] != ',')
              ++j;
            if (j > i) {
              ForceOutputValues ev;
              if (from_string(s.substr(i, j - i), ev))
                vec.push_back(ev);
            }
            i = j;
          }
          dynamic_cast<DoubleAxisHingeJoint &>(n)
              .X3DRigidJointNode::setForceOutput(std::move(vec));
        }

    });

    t.push_back(FieldInfo{
        "hinge1Angle", X3DFieldType::SFFloat, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const DoubleAxisHingeJoint &>(n).getHinge1Angle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DoubleAxisHingeJoint &>(n).emitHinge1Angle(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "hinge1AngleRate", X3DFieldType::SFFloat, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const DoubleAxisHingeJoint &>(n)
                              .getHinge1AngleRate());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DoubleAxisHingeJoint &>(n).emitHinge1AngleRate(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "hinge2Angle", X3DFieldType::SFFloat, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const DoubleAxisHingeJoint &>(n).getHinge2Angle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DoubleAxisHingeJoint &>(n).emitHinge2Angle(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "hinge2AngleRate", X3DFieldType::SFFloat, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const DoubleAxisHingeJoint &>(n)
                              .getHinge2AngleRate());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DoubleAxisHingeJoint &>(n).emitHinge2AngleRate(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const DoubleAxisHingeJoint &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DoubleAxisHingeJoint &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "maxAngle1", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const DoubleAxisHingeJoint &>(n).getMaxAngle1());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DoubleAxisHingeJoint &>(n).setMaxAngle1(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "maxTorque1", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const DoubleAxisHingeJoint &>(n).getMaxTorque1());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DoubleAxisHingeJoint &>(n).setMaxTorque1(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "maxTorque2", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const DoubleAxisHingeJoint &>(n).getMaxTorque2());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DoubleAxisHingeJoint &>(n).setMaxTorque2(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const DoubleAxisHingeJoint &>(n)
                              .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DoubleAxisHingeJoint &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "minAngle1", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const DoubleAxisHingeJoint &>(n).getMinAngle1());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DoubleAxisHingeJoint &>(n).setMinAngle1(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "stop1Bounce", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const DoubleAxisHingeJoint &>(n).getStop1Bounce());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DoubleAxisHingeJoint &>(n).setStop1Bounce(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "stop1ConstantForceMix", X3DFieldType::SFFloat, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const DoubleAxisHingeJoint &>(n)
                              .getStop1ConstantForceMix());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DoubleAxisHingeJoint &>(n).setStop1ConstantForceMix(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "stop1ErrorCorrection", X3DFieldType::SFFloat, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const DoubleAxisHingeJoint &>(n)
                              .getStop1ErrorCorrection());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DoubleAxisHingeJoint &>(n).setStop1ErrorCorrection(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "suspensionErrorCorrection", X3DFieldType::SFFloat,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const DoubleAxisHingeJoint &>(n)
                              .getSuspensionErrorCorrection());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DoubleAxisHingeJoint &>(n).setSuspensionErrorCorrection(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "suspensionForce", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const DoubleAxisHingeJoint &>(n)
                              .getSuspensionForce());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DoubleAxisHingeJoint &>(n).setSuspensionForce(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const DoubleAxisHingeJoint &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DoubleAxisHingeJoint &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const DoubleAxisHingeJoint &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DoubleAxisHingeJoint &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const DoubleAxisHingeJoint &>(n)
                              .X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DoubleAxisHingeJoint &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const DoubleAxisHingeJoint &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DoubleAxisHingeJoint &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const DoubleAxisHingeJoint &>(n)
                              .X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<DoubleAxisHingeJoint &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void DoubleAxisHingeJoint::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}
