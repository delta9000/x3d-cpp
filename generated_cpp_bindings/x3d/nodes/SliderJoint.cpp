// SliderJoint.cpp
#include "x3d/nodes/SliderJoint.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string SliderJoint::nodeTypeName() const { return "SliderJoint"; }

std::string SliderJoint::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &SliderJoint::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "axis", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SliderJoint &>(n).getAxis());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SliderJoint &>(n).setAxis(std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "body1", X3DFieldType::SFNode, AccessType::InputOutput, "body1",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SliderJoint &>(n)
                              .X3DRigidJointNode::getBody1());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SliderJoint &>(n).X3DRigidJointNode::setBody1(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "body2", X3DFieldType::SFNode, AccessType::InputOutput, "body2",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SliderJoint &>(n)
                              .X3DRigidJointNode::getBody2());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SliderJoint &>(n).X3DRigidJointNode::setBody2(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "forceOutput", X3DFieldType::MFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SliderJoint &>(n)
                              .X3DRigidJointNode::getForceOutput());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SliderJoint &>(n).X3DRigidJointNode::setForceOutput(
              std::any_cast<MFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const SliderJoint &>(n).X3DNode::getIS());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<SliderJoint &>(n).X3DNode::setIS(
                        std::any_cast<SFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "maxSeparation", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SliderJoint &>(n).getMaxSeparation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SliderJoint &>(n).setMaxSeparation(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SliderJoint &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SliderJoint &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "minSeparation", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SliderJoint &>(n).getMinSeparation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SliderJoint &>(n).setMinSeparation(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "separation", X3DFieldType::SFFloat, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SliderJoint &>(n).getSeparation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SliderJoint &>(n).emitSeparation(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "separationRate", X3DFieldType::SFFloat, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SliderJoint &>(n).getSeparationRate());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SliderJoint &>(n).emitSeparationRate(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "sliderForce", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SliderJoint &>(n).getSliderForce());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SliderJoint &>(n).setSliderForce(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "stopBounce", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SliderJoint &>(n).getStopBounce());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SliderJoint &>(n).setStopBounce(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "stopErrorCorrection", X3DFieldType::SFFloat, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SliderJoint &>(n).getStopErrorCorrection());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SliderJoint &>(n).setStopErrorCorrection(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const SliderJoint &>(n).X3DNode::getDEF());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<SliderJoint &>(n).X3DNode::setDEF(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(
        FieldInfo{"USE", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const SliderJoint &>(n).X3DNode::getUSE());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<SliderJoint &>(n).X3DNode::setUSE(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SliderJoint &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SliderJoint &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"id", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const SliderJoint &>(n).X3DNode::getId());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<SliderJoint &>(n).X3DNode::setId(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SliderJoint &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SliderJoint &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void SliderJoint::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

} // namespace x3d::nodes
