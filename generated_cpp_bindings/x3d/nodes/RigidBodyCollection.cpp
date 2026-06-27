// RigidBodyCollection.cpp
#include "x3d/nodes/RigidBodyCollection.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string RigidBodyCollection::nodeTypeName() const {
  return "RigidBodyCollection";
}

std::string RigidBodyCollection::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &RigidBodyCollection::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "autoDisable", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RigidBodyCollection &>(n).getAutoDisable());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBodyCollection &>(n).setAutoDisable(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxCenter", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RigidBodyCollection &>(n)
                              .X3DBoundedObject::getBboxCenter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBodyCollection &>(n)
              .X3DBoundedObject::setBboxCenterUnchecked(
                  std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxDisplay", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RigidBodyCollection &>(n)
                              .X3DBoundedObject::getBboxDisplay());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBodyCollection &>(n)
              .X3DBoundedObject::setBboxDisplay(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxSize", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RigidBodyCollection &>(n)
                              .X3DBoundedObject::getBboxSize());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBodyCollection &>(n)
              .X3DBoundedObject::setBboxSizeUnchecked(
                  std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bodies", X3DFieldType::MFNode, AccessType::InputOutput, "bodies",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RigidBodyCollection &>(n).getBodies());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBodyCollection &>(n).setBodies(
              std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "collider", X3DFieldType::SFNode, AccessType::InitializeOnly,
        "collider",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RigidBodyCollection &>(n).getCollider());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBodyCollection &>(n).setColliderUnchecked(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "constantForceMix", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RigidBodyCollection &>(n)
                              .getConstantForceMix());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBodyCollection &>(n).setConstantForceMix(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "contactSurfaceThickness", X3DFieldType::SFFloat,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RigidBodyCollection &>(n)
                              .getContactSurfaceThickness());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBodyCollection &>(n).setContactSurfaceThickness(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "disableAngularSpeed", X3DFieldType::SFFloat, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RigidBodyCollection &>(n)
                              .getDisableAngularSpeed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBodyCollection &>(n).setDisableAngularSpeed(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "disableLinearSpeed", X3DFieldType::SFFloat, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RigidBodyCollection &>(n)
                              .getDisableLinearSpeed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBodyCollection &>(n).setDisableLinearSpeed(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "disableTime", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RigidBodyCollection &>(n).getDisableTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBodyCollection &>(n).setDisableTimeUnchecked(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RigidBodyCollection &>(n).getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBodyCollection &>(n).setEnabled(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "errorCorrection", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RigidBodyCollection &>(n)
                              .getErrorCorrection());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBodyCollection &>(n).setErrorCorrection(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "gravity", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RigidBodyCollection &>(n).getGravity());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBodyCollection &>(n).setGravity(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RigidBodyCollection &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBodyCollection &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "iterations", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RigidBodyCollection &>(n).getIterations());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBodyCollection &>(n).setIterations(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "joints", X3DFieldType::MFNode, AccessType::InputOutput, "joints",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RigidBodyCollection &>(n).getJoints());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBodyCollection &>(n).setJoints(
              std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "maxCorrectionSpeed", X3DFieldType::SFFloat, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RigidBodyCollection &>(n)
                              .getMaxCorrectionSpeed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBodyCollection &>(n).setMaxCorrectionSpeed(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RigidBodyCollection &>(n)
                              .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBodyCollection &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "preferAccuracy", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RigidBodyCollection &>(n).getPreferAccuracy());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBodyCollection &>(n).setPreferAccuracy(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"set_contacts", X3DFieldType::MFNode, AccessType::InputOnly,
                  "set_contacts",

                  nullptr,

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<RigidBodyCollection &>(n).onSet_contacts(
                        std::any_cast<MFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "visible", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RigidBodyCollection &>(n)
                              .X3DBoundedObject::getVisible());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBodyCollection &>(n).X3DBoundedObject::setVisible(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RigidBodyCollection &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBodyCollection &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RigidBodyCollection &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBodyCollection &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"class", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const RigidBodyCollection &>(n)
                                        .X3DNode::getClass_());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<RigidBodyCollection &>(n).X3DNode::setClass_(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RigidBodyCollection &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBodyCollection &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RigidBodyCollection &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBodyCollection &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void RigidBodyCollection::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void RigidBodyCollection::validateRanges(
    std::vector<RangeDiagnostic> &out) const {

  checkRangesDisableTime(getDisableTime(), nodeTypeName(), "", out);
}

void RigidBodyCollection::checkRangesDisableTime(
    const SFTime &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "disableTime",
                                  "disableTime below minimum of 0"});
}

} // namespace x3d::nodes
