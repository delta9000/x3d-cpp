// PlaneSensor.cpp
#include "x3d/nodes/PlaneSensor.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string PlaneSensor::nodeTypeName() const { return "PlaneSensor"; }

std::string PlaneSensor::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &PlaneSensor::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "autoOffset", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PlaneSensor &>(n)
                              .X3DDragSensorNode::getAutoOffset());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PlaneSensor &>(n).X3DDragSensorNode::setAutoOffset(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "axisRotation", X3DFieldType::SFRotation, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PlaneSensor &>(n).getAxisRotation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PlaneSensor &>(n).setAxisRotation(
              std::any_cast<SFRotation>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PlaneSensor &>(n)
                              .X3DSensorNode::getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PlaneSensor &>(n).X3DSensorNode::setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PlaneSensor &>(n).X3DSensorNode::getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PlaneSensor &>(n).X3DSensorNode::setEnabled(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const PlaneSensor &>(n).X3DNode::getIS());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<PlaneSensor &>(n).X3DNode::setIS(
                        std::any_cast<SFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(
        FieldInfo{"isActive", X3DFieldType::SFBool, AccessType::OutputOnly, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const PlaneSensor &>(n)
                                        .X3DSensorNode::getIsActive());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<PlaneSensor &>(n).X3DSensorNode::emitIsActive(
                        std::any_cast<SFBool>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "isOver", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PlaneSensor &>(n)
                              .X3DPointingDeviceSensorNode::getIsOver());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PlaneSensor &>(n)
              .X3DPointingDeviceSensorNode::emitIsOver(
                  std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "maxPosition", X3DFieldType::SFVec2f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PlaneSensor &>(n).getMaxPosition());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PlaneSensor &>(n).setMaxPosition(
              std::any_cast<SFVec2f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PlaneSensor &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PlaneSensor &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "minPosition", X3DFieldType::SFVec2f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PlaneSensor &>(n).getMinPosition());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PlaneSensor &>(n).setMinPosition(
              std::any_cast<SFVec2f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "offset", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PlaneSensor &>(n).getOffset());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PlaneSensor &>(n).setOffset(std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "trackPoint_changed", X3DFieldType::SFVec3f, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PlaneSensor &>(n)
                              .X3DDragSensorNode::getTrackPoint_changed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PlaneSensor &>(n)
              .X3DDragSensorNode::emitTrackPoint_changed(
                  std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "translation_changed", X3DFieldType::SFVec3f, AccessType::OutputOnly,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PlaneSensor &>(n).getTranslation_changed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PlaneSensor &>(n).emitTranslation_changed(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const PlaneSensor &>(n).X3DNode::getDEF());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<PlaneSensor &>(n).X3DNode::setDEF(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(
        FieldInfo{"USE", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const PlaneSensor &>(n).X3DNode::getUSE());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<PlaneSensor &>(n).X3DNode::setUSE(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PlaneSensor &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PlaneSensor &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"id", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const PlaneSensor &>(n).X3DNode::getId());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<PlaneSensor &>(n).X3DNode::setId(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PlaneSensor &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PlaneSensor &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void PlaneSensor::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

namespace factory_detail {
std::shared_ptr<X3DNode> createPlaneSensor() {
  return std::make_shared<PlaneSensor>();
}
} // namespace factory_detail

} // namespace x3d::nodes
