// TouchSensor.cpp
#include "x3d/nodes/TouchSensor.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string TouchSensor::nodeTypeName() const { return "TouchSensor"; }

std::string TouchSensor::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &TouchSensor::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TouchSensor &>(n)
                              .X3DSensorNode::getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TouchSensor &>(n).X3DSensorNode::setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TouchSensor &>(n).X3DSensorNode::getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TouchSensor &>(n).X3DSensorNode::setEnabled(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "hitNormal_changed", X3DFieldType::SFVec3f, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TouchSensor &>(n).getHitNormal_changed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TouchSensor &>(n).emitHitNormal_changed(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "hitPoint_changed", X3DFieldType::SFVec3f, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TouchSensor &>(n).getHitPoint_changed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TouchSensor &>(n).emitHitPoint_changed(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "hitTexCoord_changed", X3DFieldType::SFVec2f, AccessType::OutputOnly,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TouchSensor &>(n).getHitTexCoord_changed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TouchSensor &>(n).emitHitTexCoord_changed(
              std::any_cast<SFVec2f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const TouchSensor &>(n).X3DNode::getIS());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<TouchSensor &>(n).X3DNode::setIS(
                        std::any_cast<SFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(
        FieldInfo{"isActive", X3DFieldType::SFBool, AccessType::OutputOnly, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const TouchSensor &>(n)
                                        .X3DSensorNode::getIsActive());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<TouchSensor &>(n).X3DSensorNode::emitIsActive(
                        std::any_cast<SFBool>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "isOver", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TouchSensor &>(n)
                              .X3DPointingDeviceSensorNode::getIsOver());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TouchSensor &>(n)
              .X3DPointingDeviceSensorNode::emitIsOver(
                  std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TouchSensor &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TouchSensor &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "touchTime", X3DFieldType::SFTime, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TouchSensor &>(n)
                              .X3DTouchSensorNode::getTouchTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TouchSensor &>(n).X3DTouchSensorNode::emitTouchTime(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const TouchSensor &>(n).X3DNode::getDEF());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<TouchSensor &>(n).X3DNode::setDEF(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(
        FieldInfo{"USE", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const TouchSensor &>(n).X3DNode::getUSE());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<TouchSensor &>(n).X3DNode::setUSE(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TouchSensor &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TouchSensor &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"id", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const TouchSensor &>(n).X3DNode::getId());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<TouchSensor &>(n).X3DNode::setId(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TouchSensor &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TouchSensor &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void TouchSensor::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

} // namespace x3d::nodes
