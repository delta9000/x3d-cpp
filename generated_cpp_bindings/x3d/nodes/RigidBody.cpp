// RigidBody.cpp
#include "x3d/nodes/RigidBody.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string RigidBody::nodeTypeName() const { return "RigidBody"; }

std::string RigidBody::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &RigidBody::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "angularDampingFactor", X3DFieldType::SFFloat, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RigidBody &>(n).getAngularDampingFactor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBody &>(n).setAngularDampingFactor(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "angularVelocity", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RigidBody &>(n).getAngularVelocity());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBody &>(n).setAngularVelocity(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "autoDamp", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RigidBody &>(n).getAutoDamp());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBody &>(n).setAutoDamp(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "autoDisable", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RigidBody &>(n).getAutoDisable());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBody &>(n).setAutoDisable(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxCenter", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RigidBody &>(n)
                              .X3DBoundedObject::getBboxCenter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBody &>(n).X3DBoundedObject::setBboxCenterUnchecked(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxDisplay", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RigidBody &>(n)
                              .X3DBoundedObject::getBboxDisplay());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBody &>(n).X3DBoundedObject::setBboxDisplay(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxSize", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RigidBody &>(n)
                              .X3DBoundedObject::getBboxSize());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBody &>(n).X3DBoundedObject::setBboxSizeUnchecked(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "centerOfMass", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RigidBody &>(n).getCenterOfMass());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBody &>(n).setCenterOfMass(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "disableAngularSpeed", X3DFieldType::SFFloat, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RigidBody &>(n).getDisableAngularSpeed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBody &>(n).setDisableAngularSpeed(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "disableLinearSpeed", X3DFieldType::SFFloat, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RigidBody &>(n).getDisableLinearSpeed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBody &>(n).setDisableLinearSpeed(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "disableTime", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RigidBody &>(n).getDisableTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBody &>(n).setDisableTimeUnchecked(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RigidBody &>(n).getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBody &>(n).setEnabled(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "finiteRotationAxis", X3DFieldType::SFVec3f, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RigidBody &>(n).getFiniteRotationAxis());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBody &>(n).setFiniteRotationAxis(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "fixed", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RigidBody &>(n).getFixed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBody &>(n).setFixed(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "forces", X3DFieldType::MFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RigidBody &>(n).getForces());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBody &>(n).setForces(std::any_cast<MFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "geometry", X3DFieldType::MFNode, AccessType::InputOutput, "geometry",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RigidBody &>(n).getGeometry());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBody &>(n).setGeometry(std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "inertia", X3DFieldType::SFMatrix3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RigidBody &>(n).getInertia());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBody &>(n).setInertia(std::any_cast<SFMatrix3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RigidBody &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBody &>(n).X3DNode::setIS(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "linearDampingFactor", X3DFieldType::SFFloat, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RigidBody &>(n).getLinearDampingFactor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBody &>(n).setLinearDampingFactor(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "linearVelocity", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RigidBody &>(n).getLinearVelocity());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBody &>(n).setLinearVelocity(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "mass", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RigidBody &>(n).getMass());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBody &>(n).setMass(std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "massDensityModel", X3DFieldType::SFNode, AccessType::InputOutput,
        "massDensityModel",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RigidBody &>(n).getMassDensityModel());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBody &>(n).setMassDensityModel(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RigidBody &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBody &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "orientation", X3DFieldType::SFRotation, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RigidBody &>(n).getOrientation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBody &>(n).setOrientation(
              std::any_cast<SFRotation>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "position", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RigidBody &>(n).getPosition());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBody &>(n).setPosition(std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "torques", X3DFieldType::MFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RigidBody &>(n).getTorques());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBody &>(n).setTorques(std::any_cast<MFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "useFiniteRotation", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RigidBody &>(n).getUseFiniteRotation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBody &>(n).setUseFiniteRotation(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "useGlobalGravity", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RigidBody &>(n).getUseGlobalGravity());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBody &>(n).setUseGlobalGravity(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"visible", X3DFieldType::SFBool, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const RigidBody &>(n)
                                        .X3DBoundedObject::getVisible());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<RigidBody &>(n).X3DBoundedObject::setVisible(
                        std::any_cast<SFBool>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RigidBody &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBody &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RigidBody &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBody &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const RigidBody &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBody &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const RigidBody &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<RigidBody &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"style", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const RigidBody &>(n).X3DNode::getStyle());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<RigidBody &>(n).X3DNode::setStyle(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    return t;
  }();
  return table;
}

void RigidBody::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void RigidBody::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesDisableTime(getDisableTime(), nodeTypeName(), "", out);
}

void RigidBody::checkRangesDisableTime(const SFTime &value,
                                       const std::string &nodeType,
                                       const std::string &defName,
                                       std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "disableTime",
                                  "disableTime below minimum of 0"});
}

namespace factory_detail {
std::shared_ptr<X3DNode> createRigidBody() {
  return std::make_shared<RigidBody>();
}
} // namespace factory_detail

} // namespace x3d::nodes
