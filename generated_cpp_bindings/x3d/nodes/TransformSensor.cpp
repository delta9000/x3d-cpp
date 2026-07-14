// TransformSensor.cpp
#include "x3d/nodes/TransformSensor.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string TransformSensor::nodeTypeName() const { return "TransformSensor"; }

std::string TransformSensor::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &TransformSensor::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "center", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TransformSensor &>(n).getCenter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransformSensor &>(n).setCenter(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TransformSensor &>(n)
                              .X3DSensorNode::getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransformSensor &>(n).X3DSensorNode::setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TransformSensor &>(n)
                              .X3DSensorNode::getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransformSensor &>(n).X3DSensorNode::setEnabled(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enterTime", X3DFieldType::SFTime, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransformSensor &>(n).getEnterTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransformSensor &>(n).emitEnterTime(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"exitTime", X3DFieldType::SFTime, AccessType::OutputOnly, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const TransformSensor &>(n).getExitTime());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<TransformSensor &>(n).emitExitTime(
                        std::any_cast<SFTime>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransformSensor &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransformSensor &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isActive", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TransformSensor &>(n)
                              .X3DSensorNode::getIsActive());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransformSensor &>(n).X3DSensorNode::emitIsActive(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransformSensor &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransformSensor &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"orientation_changed", X3DFieldType::SFRotation,
                  AccessType::OutputOnly, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const TransformSensor &>(n)
                                        .getOrientation_changed());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<TransformSensor &>(n).emitOrientation_changed(
                        std::any_cast<SFRotation>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "position_changed", X3DFieldType::SFVec3f, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransformSensor &>(n).getPosition_changed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransformSensor &>(n).emitPosition_changed(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"size", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const TransformSensor &>(n)
                                        .X3DEnvironmentalSensorNode::getSize());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<TransformSensor &>(n)
                        .X3DEnvironmentalSensorNode::setSizeUnchecked(
                            std::any_cast<SFVec3f>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "targetObject", X3DFieldType::SFNode, AccessType::InputOutput,
        "targetObject",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransformSensor &>(n).getTargetObject());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransformSensor &>(n).setTargetObject(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransformSensor &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransformSensor &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransformSensor &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransformSensor &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransformSensor &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransformSensor &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransformSensor &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransformSensor &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TransformSensor &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TransformSensor &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void TransformSensor::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void TransformSensor::validateRanges(std::vector<RangeDiagnostic> &out) const {

  X3DEnvironmentalSensorNode::checkRangesSize(
      X3DEnvironmentalSensorNode::getSize(), nodeTypeName(), "", out);
}

namespace factory_detail {
std::shared_ptr<X3DNode> createTransformSensor() {
  return std::make_shared<TransformSensor>();
}
} // namespace factory_detail

} // namespace x3d::nodes
