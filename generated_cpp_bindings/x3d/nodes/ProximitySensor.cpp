// ProximitySensor.cpp
#include "x3d/nodes/ProximitySensor.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string ProximitySensor::nodeTypeName() const { return "ProximitySensor"; }

std::string ProximitySensor::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &ProximitySensor::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "center", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ProximitySensor &>(n).getCenter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ProximitySensor &>(n).setCenter(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "centerOfRotation_changed", X3DFieldType::SFVec3f,
        AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ProximitySensor &>(n)
                              .getCenterOfRotation_changed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ProximitySensor &>(n).emitCenterOfRotation_changed(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ProximitySensor &>(n)
                              .X3DSensorNode::getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ProximitySensor &>(n).X3DSensorNode::setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ProximitySensor &>(n)
                              .X3DSensorNode::getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ProximitySensor &>(n).X3DSensorNode::setEnabled(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enterTime", X3DFieldType::SFTime, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ProximitySensor &>(n).getEnterTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ProximitySensor &>(n).emitEnterTime(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"exitTime", X3DFieldType::SFTime, AccessType::OutputOnly, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const ProximitySensor &>(n).getExitTime());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<ProximitySensor &>(n).emitExitTime(
                        std::any_cast<SFTime>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ProximitySensor &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ProximitySensor &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isActive", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ProximitySensor &>(n)
                              .X3DSensorNode::getIsActive());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ProximitySensor &>(n).X3DSensorNode::emitIsActive(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ProximitySensor &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ProximitySensor &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"orientation_changed", X3DFieldType::SFRotation,
                  AccessType::OutputOnly, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const ProximitySensor &>(n)
                                        .getOrientation_changed());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<ProximitySensor &>(n).emitOrientation_changed(
                        std::any_cast<SFRotation>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "position_changed", X3DFieldType::SFVec3f, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ProximitySensor &>(n).getPosition_changed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ProximitySensor &>(n).emitPosition_changed(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"size", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const ProximitySensor &>(n)
                                        .X3DEnvironmentalSensorNode::getSize());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<ProximitySensor &>(n)
                        .X3DEnvironmentalSensorNode::setSizeUnchecked(
                            std::any_cast<SFVec3f>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ProximitySensor &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ProximitySensor &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ProximitySensor &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ProximitySensor &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ProximitySensor &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ProximitySensor &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ProximitySensor &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ProximitySensor &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ProximitySensor &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ProximitySensor &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void ProximitySensor::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void ProximitySensor::validateRanges(std::vector<RangeDiagnostic> &out) const {

  X3DEnvironmentalSensorNode::checkRangesSize(
      X3DEnvironmentalSensorNode::getSize(), nodeTypeName(), "", out);
}

namespace factory_detail {
std::shared_ptr<X3DNode> createProximitySensor() {
  return std::make_shared<ProximitySensor>();
}
} // namespace factory_detail

} // namespace x3d::nodes
