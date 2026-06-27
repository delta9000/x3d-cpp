// X3DLightNode.cpp
#include "x3d/nodes/X3DLightNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string X3DLightNode::nodeTypeName() const { return "X3DLightNode"; }

std::string X3DLightNode::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &X3DLightNode::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "ambientIntensity", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DLightNode &>(n).getAmbientIntensity());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DLightNode &>(n).setAmbientIntensityUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "color", X3DFieldType::SFColor, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DLightNode &>(n).getColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DLightNode &>(n).setColorUnchecked(
              std::any_cast<SFColor>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "intensity", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DLightNode &>(n).getIntensity());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DLightNode &>(n).setIntensityUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const X3DLightNode &>(n).X3DNode::getIS());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<X3DLightNode &>(n).X3DNode::setIS(
                        std::any_cast<SFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DLightNode &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DLightNode &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "on", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DLightNode &>(n).getOn());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DLightNode &>(n).setOn(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "shadowIntensity", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DLightNode &>(n).getShadowIntensity());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DLightNode &>(n).setShadowIntensityUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "shadows", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DLightNode &>(n).getShadows());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DLightNode &>(n).setShadows(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DLightNode &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DLightNode &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DLightNode &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DLightNode &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DLightNode &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DLightNode &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"id", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const X3DLightNode &>(n).X3DNode::getId());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<X3DLightNode &>(n).X3DNode::setId(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DLightNode &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DLightNode &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void X3DLightNode::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void X3DLightNode::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesAmbientIntensity(getAmbientIntensity(), nodeTypeName(), "", out);

  checkRangesColor(getColor(), nodeTypeName(), "", out);

  checkRangesIntensity(getIntensity(), nodeTypeName(), "", out);

  checkRangesShadowIntensity(getShadowIntensity(), nodeTypeName(), "", out);
}

void X3DLightNode::checkRangesAmbientIntensity(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "ambientIntensity",
                                  "ambientIntensity below minimum of 0"});
  if (value > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "ambientIntensity",
                                  "ambientIntensity above maximum of 1"});
}

void X3DLightNode::checkRangesColor(const SFColor &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out) {
  if (value.r < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "color",
                                  "color.r below minimum of 0"});
  if (value.r > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "color",
                                  "color.r above maximum of 1"});

  if (value.g < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "color",
                                  "color.g below minimum of 0"});
  if (value.g > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "color",
                                  "color.g above maximum of 1"});

  if (value.b < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "color",
                                  "color.b below minimum of 0"});
  if (value.b > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "color",
                                  "color.b above maximum of 1"});
}

void X3DLightNode::checkRangesIntensity(const SFFloat &value,
                                        const std::string &nodeType,
                                        const std::string &defName,
                                        std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "intensity",
                                  "intensity below minimum of 0"});
}

void X3DLightNode::checkRangesShadowIntensity(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "shadowIntensity",
                                  "shadowIntensity below minimum of 0"});
  if (value > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "shadowIntensity",
                                  "shadowIntensity above maximum of 1"});
}

} // namespace x3d::nodes
