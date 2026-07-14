// ColorRGBA.cpp
#include "x3d/nodes/ColorRGBA.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string ColorRGBA::nodeTypeName() const { return "ColorRGBA"; }

std::string ColorRGBA::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &ColorRGBA::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "color", X3DFieldType::MFColorRGBA, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ColorRGBA &>(n).getColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ColorRGBA &>(n).setColorUnchecked(
              std::any_cast<MFColorRGBA>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ColorRGBA &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ColorRGBA &>(n).X3DNode::setIS(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ColorRGBA &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ColorRGBA &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ColorRGBA &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ColorRGBA &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ColorRGBA &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ColorRGBA &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ColorRGBA &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ColorRGBA &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ColorRGBA &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ColorRGBA &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"style", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const ColorRGBA &>(n).X3DNode::getStyle());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<ColorRGBA &>(n).X3DNode::setStyle(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    return t;
  }();
  return table;
}

void ColorRGBA::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void ColorRGBA::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesColor(getColor(), nodeTypeName(), "", out);
}

void ColorRGBA::checkRangesColor(const MFColorRGBA &value,
                                 const std::string &nodeType,
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

    if (v.a < 0)
      out.push_back(RangeDiagnostic{nodeType, defName, "color",
                                    "color.a below minimum of 0"});
    if (v.a > 1)
      out.push_back(RangeDiagnostic{nodeType, defName, "color",
                                    "color.a above maximum of 1"});
  }
}

namespace factory_detail {
std::shared_ptr<X3DNode> createColorRGBA() {
  return std::make_shared<ColorRGBA>();
}
} // namespace factory_detail

} // namespace x3d::nodes
