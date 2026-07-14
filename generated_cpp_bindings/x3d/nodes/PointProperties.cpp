// PointProperties.cpp
#include "x3d/nodes/PointProperties.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string PointProperties::nodeTypeName() const { return "PointProperties"; }

std::string PointProperties::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &PointProperties::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "attenuation", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PointProperties &>(n).getAttenuation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointProperties &>(n).setAttenuation(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PointProperties &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointProperties &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PointProperties &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointProperties &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "pointSizeMaxValue", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PointProperties &>(n).getPointSizeMaxValue());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointProperties &>(n).setPointSizeMaxValueUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "pointSizeMinValue", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PointProperties &>(n).getPointSizeMinValue());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointProperties &>(n).setPointSizeMinValueUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "pointSizeScaleFactor", X3DFieldType::SFFloat, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PointProperties &>(n)
                              .getPointSizeScaleFactor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointProperties &>(n).setPointSizeScaleFactorUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PointProperties &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointProperties &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PointProperties &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointProperties &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PointProperties &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointProperties &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PointProperties &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointProperties &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PointProperties &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointProperties &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void PointProperties::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void PointProperties::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesPointSizeMaxValue(getPointSizeMaxValue(), nodeTypeName(), "", out);

  checkRangesPointSizeMinValue(getPointSizeMinValue(), nodeTypeName(), "", out);

  checkRangesPointSizeScaleFactor(getPointSizeScaleFactor(), nodeTypeName(), "",
                                  out);
}

void PointProperties::checkRangesPointSizeMaxValue(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "pointSizeMaxValue",
                                  "pointSizeMaxValue below minimum of 0"});
}

void PointProperties::checkRangesPointSizeMinValue(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "pointSizeMinValue",
                                  "pointSizeMinValue below minimum of 0"});
}

void PointProperties::checkRangesPointSizeScaleFactor(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "pointSizeScaleFactor",
                                  "pointSizeScaleFactor below minimum of 1"});
}

namespace factory_detail {
std::shared_ptr<X3DNode> createPointProperties() {
  return std::make_shared<PointProperties>();
}
} // namespace factory_detail

} // namespace x3d::nodes
