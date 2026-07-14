// Color.cpp
#include "x3d/nodes/Color.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string Color::nodeTypeName() const { return "Color"; }

std::string Color::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &Color::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "color", X3DFieldType::MFColor, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Color &>(n).getColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Color &>(n).setColorUnchecked(std::any_cast<MFColor>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Color &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Color &>(n).X3DNode::setIS(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Color &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Color &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Color &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Color &>(n).X3DNode::setDEF(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Color &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Color &>(n).X3DNode::setUSE(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Color &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Color &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Color &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Color &>(n).X3DNode::setId(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Color &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Color &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void Color::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void Color::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesColor(getColor(), nodeTypeName(), "", out);
}

void Color::checkRangesColor(const MFColor &value, const std::string &nodeType,
                             const std::string &defName,
                             std::vector<RangeDiagnostic> &out) {
  for (const auto &v : value) {

    if (v.r < 0)
      out.push_back(RangeDiagnostic{nodeType, defName, "color",
                                    "color.r below minimum of 0"});
    if (v.r > 1)
      out.push_back(RangeDiagnostic{nodeType, defName, "color",
                                    "color.r above maximum of 1"});

    if (v.g < 0)
      out.push_back(RangeDiagnostic{nodeType, defName, "color",
                                    "color.g below minimum of 0"});
    if (v.g > 1)
      out.push_back(RangeDiagnostic{nodeType, defName, "color",
                                    "color.g above maximum of 1"});

    if (v.b < 0)
      out.push_back(RangeDiagnostic{nodeType, defName, "color",
                                    "color.b below minimum of 0"});
    if (v.b > 1)
      out.push_back(RangeDiagnostic{nodeType, defName, "color",
                                    "color.b above maximum of 1"});
  }
}

namespace factory_detail {
std::shared_ptr<X3DNode> createColor() { return std::make_shared<Color>(); }
} // namespace factory_detail

} // namespace x3d::nodes
