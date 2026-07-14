// ColorInterpolator.cpp
#include "x3d/nodes/ColorInterpolator.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string ColorInterpolator::nodeTypeName() const {
  return "ColorInterpolator";
}

std::string ColorInterpolator::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &ColorInterpolator::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ColorInterpolator &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ColorInterpolator &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "key", X3DFieldType::MFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ColorInterpolator &>(n)
                              .X3DInterpolatorNode::getKey());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ColorInterpolator &>(n).X3DInterpolatorNode::setKey(
              std::any_cast<MFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "keyValue", X3DFieldType::MFColor, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ColorInterpolator &>(n).getKeyValue());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ColorInterpolator &>(n).setKeyValueUnchecked(
              std::any_cast<MFColor>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ColorInterpolator &>(n)
                              .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ColorInterpolator &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "set_fraction", X3DFieldType::SFFloat, AccessType::InputOnly, "",

        nullptr,

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ColorInterpolator &>(n)
              .X3DInterpolatorNode::onSet_fraction(std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "value_changed", X3DFieldType::SFColor, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ColorInterpolator &>(n).getValue_changed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ColorInterpolator &>(n).emitValue_changed(
              std::any_cast<SFColor>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ColorInterpolator &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ColorInterpolator &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ColorInterpolator &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ColorInterpolator &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ColorInterpolator &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ColorInterpolator &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ColorInterpolator &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ColorInterpolator &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ColorInterpolator &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ColorInterpolator &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void ColorInterpolator::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void ColorInterpolator::validateRanges(
    std::vector<RangeDiagnostic> &out) const {

  checkRangesKeyValue(getKeyValue(), nodeTypeName(), "", out);
}

void ColorInterpolator::checkRangesKeyValue(const MFColor &value,
                                            const std::string &nodeType,
                                            const std::string &defName,
                                            std::vector<RangeDiagnostic> &out) {
  for (const auto &v : value) {

    if (v.r < 0)
      out.push_back(RangeDiagnostic{nodeType, defName, "keyValue",
                                    "keyValue.r below minimum of 0"});
    if (v.r > 1)
      out.push_back(RangeDiagnostic{nodeType, defName, "keyValue",
                                    "keyValue.r above maximum of 1"});

    if (v.g < 0)
      out.push_back(RangeDiagnostic{nodeType, defName, "keyValue",
                                    "keyValue.g below minimum of 0"});
    if (v.g > 1)
      out.push_back(RangeDiagnostic{nodeType, defName, "keyValue",
                                    "keyValue.g above maximum of 1"});

    if (v.b < 0)
      out.push_back(RangeDiagnostic{nodeType, defName, "keyValue",
                                    "keyValue.b below minimum of 0"});
    if (v.b > 1)
      out.push_back(RangeDiagnostic{nodeType, defName, "keyValue",
                                    "keyValue.b above maximum of 1"});
  }
}

namespace factory_detail {
std::shared_ptr<X3DNode> createColorInterpolator() {
  return std::make_shared<ColorInterpolator>();
}
} // namespace factory_detail

} // namespace x3d::nodes
