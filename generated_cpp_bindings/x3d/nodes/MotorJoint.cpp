// MotorJoint.cpp
#include "x3d/nodes/MotorJoint.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string MotorJoint::nodeTypeName() const { return "MotorJoint"; }

std::string MotorJoint::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &MotorJoint::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "autoCalc", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const MotorJoint &>(n).getAutoCalc());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MotorJoint &>(n).setAutoCalcUnchecked(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "axis1Angle", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const MotorJoint &>(n).getAxis1Angle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MotorJoint &>(n).setAxis1Angle(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "axis1Torque", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const MotorJoint &>(n).getAxis1Torque());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MotorJoint &>(n).setAxis1Torque(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "axis2Angle", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const MotorJoint &>(n).getAxis2Angle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MotorJoint &>(n).setAxis2Angle(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "axis2Torque", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const MotorJoint &>(n).getAxis2Torque());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MotorJoint &>(n).setAxis2Torque(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "axis3Angle", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const MotorJoint &>(n).getAxis3Angle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MotorJoint &>(n).setAxis3Angle(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "axis3Torque", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const MotorJoint &>(n).getAxis3Torque());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MotorJoint &>(n).setAxis3Torque(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "body1", X3DFieldType::SFNode, AccessType::InputOutput, "body1",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const MotorJoint &>(n)
                              .X3DRigidJointNode::getBody1());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MotorJoint &>(n).X3DRigidJointNode::setBody1(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "body2", X3DFieldType::SFNode, AccessType::InputOutput, "body2",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const MotorJoint &>(n)
                              .X3DRigidJointNode::getBody2());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MotorJoint &>(n).X3DRigidJointNode::setBody2(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enabledAxes", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const MotorJoint &>(n).getEnabledAxes());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MotorJoint &>(n).setEnabledAxesUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "forceOutput", X3DFieldType::MFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const MotorJoint &>(n)
                              .X3DRigidJointNode::getForceOutput());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MotorJoint &>(n).X3DRigidJointNode::setForceOutput(
              std::any_cast<MFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const MotorJoint &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MotorJoint &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const MotorJoint &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MotorJoint &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "motor1Angle", X3DFieldType::SFFloat, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const MotorJoint &>(n).getMotor1Angle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MotorJoint &>(n).emitMotor1Angle(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "motor1AngleRate", X3DFieldType::SFFloat, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const MotorJoint &>(n).getMotor1AngleRate());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MotorJoint &>(n).emitMotor1AngleRate(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "motor1Axis", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const MotorJoint &>(n).getMotor1Axis());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MotorJoint &>(n).setMotor1Axis(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "motor2Angle", X3DFieldType::SFFloat, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const MotorJoint &>(n).getMotor2Angle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MotorJoint &>(n).emitMotor2Angle(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "motor2AngleRate", X3DFieldType::SFFloat, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const MotorJoint &>(n).getMotor2AngleRate());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MotorJoint &>(n).emitMotor2AngleRate(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "motor2Axis", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const MotorJoint &>(n).getMotor2Axis());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MotorJoint &>(n).setMotor2Axis(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "motor3Angle", X3DFieldType::SFFloat, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const MotorJoint &>(n).getMotor3Angle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MotorJoint &>(n).emitMotor3Angle(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "motor3AngleRate", X3DFieldType::SFFloat, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const MotorJoint &>(n).getMotor3AngleRate());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MotorJoint &>(n).emitMotor3AngleRate(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "motor3Axis", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const MotorJoint &>(n).getMotor3Axis());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MotorJoint &>(n).setMotor3Axis(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "stop1Bounce", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const MotorJoint &>(n).getStop1Bounce());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MotorJoint &>(n).setStop1Bounce(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "stop1ErrorCorrection", X3DFieldType::SFFloat, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const MotorJoint &>(n).getStop1ErrorCorrection());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MotorJoint &>(n).setStop1ErrorCorrection(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "stop2Bounce", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const MotorJoint &>(n).getStop2Bounce());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MotorJoint &>(n).setStop2Bounce(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "stop2ErrorCorrection", X3DFieldType::SFFloat, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const MotorJoint &>(n).getStop2ErrorCorrection());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MotorJoint &>(n).setStop2ErrorCorrection(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "stop3Bounce", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const MotorJoint &>(n).getStop3Bounce());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MotorJoint &>(n).setStop3Bounce(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "stop3ErrorCorrection", X3DFieldType::SFFloat, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const MotorJoint &>(n).getStop3ErrorCorrection());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MotorJoint &>(n).setStop3ErrorCorrection(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const MotorJoint &>(n).X3DNode::getDEF());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<MotorJoint &>(n).X3DNode::setDEF(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(
        FieldInfo{"USE", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const MotorJoint &>(n).X3DNode::getUSE());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<MotorJoint &>(n).X3DNode::setUSE(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const MotorJoint &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MotorJoint &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const MotorJoint &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MotorJoint &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const MotorJoint &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MotorJoint &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void MotorJoint::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void MotorJoint::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesEnabledAxes(getEnabledAxes(), nodeTypeName(), "", out);
}

void MotorJoint::checkRangesEnabledAxes(const SFInt32 &value,
                                        const std::string &nodeType,
                                        const std::string &defName,
                                        std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "enabledAxes",
                                  "enabledAxes below minimum of 0"});
  if (value > 3)
    out.push_back(RangeDiagnostic{nodeType, defName, "enabledAxes",
                                  "enabledAxes above maximum of 3"});
}

} // namespace x3d::nodes
