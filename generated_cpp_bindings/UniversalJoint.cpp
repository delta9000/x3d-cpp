// UniversalJoint.cpp
#include "UniversalJoint.hpp"

std::string UniversalJoint::nodeTypeName() const { return "UniversalJoint"; }

std::string UniversalJoint::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &UniversalJoint::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "anchorPoint", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const UniversalJoint &>(n).getAnchorPoint());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UniversalJoint &>(n).setAnchorPoint(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "axis1", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const UniversalJoint &>(n).getAxis1());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UniversalJoint &>(n).setAxis1(std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "axis2", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const UniversalJoint &>(n).getAxis2());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UniversalJoint &>(n).setAxis2(std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "body1", X3DFieldType::SFNode, AccessType::InputOutput, "body1",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const UniversalJoint &>(n)
                              .X3DRigidJointNode::getBody1());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UniversalJoint &>(n).X3DRigidJointNode::setBody1(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "body1AnchorPoint", X3DFieldType::SFVec3f, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const UniversalJoint &>(n).getBody1AnchorPoint());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UniversalJoint &>(n).emitBody1AnchorPoint(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "body1Axis", X3DFieldType::SFVec3f, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const UniversalJoint &>(n).getBody1Axis());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UniversalJoint &>(n).emitBody1Axis(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "body2", X3DFieldType::SFNode, AccessType::InputOutput, "body2",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const UniversalJoint &>(n)
                              .X3DRigidJointNode::getBody2());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UniversalJoint &>(n).X3DRigidJointNode::setBody2(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "body2AnchorPoint", X3DFieldType::SFVec3f, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const UniversalJoint &>(n).getBody2AnchorPoint());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UniversalJoint &>(n).emitBody2AnchorPoint(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "body2Axis", X3DFieldType::SFVec3f, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const UniversalJoint &>(n).getBody2Axis());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UniversalJoint &>(n).emitBody2Axis(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "forceOutput", X3DFieldType::MFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const UniversalJoint &>(n)
                              .X3DRigidJointNode::getForceOutput());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UniversalJoint &>(n).X3DRigidJointNode::setForceOutput(
              std::any_cast<MFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const UniversalJoint &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UniversalJoint &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const UniversalJoint &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UniversalJoint &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "stop1Bounce", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const UniversalJoint &>(n).getStop1Bounce());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UniversalJoint &>(n).setStop1BounceUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "stop1ErrorCorrection", X3DFieldType::SFFloat, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const UniversalJoint &>(n)
                              .getStop1ErrorCorrection());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UniversalJoint &>(n).setStop1ErrorCorrectionUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "stop2Bounce", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const UniversalJoint &>(n).getStop2Bounce());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UniversalJoint &>(n).setStop2BounceUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "stop2ErrorCorrection", X3DFieldType::SFFloat, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const UniversalJoint &>(n)
                              .getStop2ErrorCorrection());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UniversalJoint &>(n).setStop2ErrorCorrectionUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const UniversalJoint &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UniversalJoint &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const UniversalJoint &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UniversalJoint &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const UniversalJoint &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UniversalJoint &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const UniversalJoint &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UniversalJoint &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const UniversalJoint &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UniversalJoint &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void UniversalJoint::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void UniversalJoint::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesStop1Bounce(getStop1Bounce(), nodeTypeName(), "", out);

  checkRangesStop1ErrorCorrection(getStop1ErrorCorrection(), nodeTypeName(), "",
                                  out);

  checkRangesStop2Bounce(getStop2Bounce(), nodeTypeName(), "", out);

  checkRangesStop2ErrorCorrection(getStop2ErrorCorrection(), nodeTypeName(), "",
                                  out);
}

void UniversalJoint::checkRangesStop1Bounce(const SFFloat &value,
                                            const std::string &nodeType,
                                            const std::string &defName,
                                            std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "stop1Bounce",
                                  "stop1Bounce below minimum of 0"});
  if (value > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "stop1Bounce",
                                  "stop1Bounce above maximum of 1"});
}

void UniversalJoint::checkRangesStop1ErrorCorrection(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "stop1ErrorCorrection",
                                  "stop1ErrorCorrection below minimum of 0"});
  if (value > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "stop1ErrorCorrection",
                                  "stop1ErrorCorrection above maximum of 1"});
}

void UniversalJoint::checkRangesStop2Bounce(const SFFloat &value,
                                            const std::string &nodeType,
                                            const std::string &defName,
                                            std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "stop2Bounce",
                                  "stop2Bounce below minimum of 0"});
  if (value > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "stop2Bounce",
                                  "stop2Bounce above maximum of 1"});
}

void UniversalJoint::checkRangesStop2ErrorCorrection(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "stop2ErrorCorrection",
                                  "stop2ErrorCorrection below minimum of 0"});
  if (value > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "stop2ErrorCorrection",
                                  "stop2ErrorCorrection above maximum of 1"});
}
