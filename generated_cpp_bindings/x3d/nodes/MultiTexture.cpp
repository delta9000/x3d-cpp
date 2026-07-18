// MultiTexture.cpp
#include "x3d/nodes/MultiTexture.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string MultiTexture::nodeTypeName() const { return "MultiTexture"; }

std::string MultiTexture::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &MultiTexture::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "alpha", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const MultiTexture &>(n).getAlpha());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MultiTexture &>(n).setAlphaUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "color", X3DFieldType::SFColor, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const MultiTexture &>(n).getColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MultiTexture &>(n).setColorUnchecked(
              std::any_cast<SFColor>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const MultiTexture &>(n)
                              .X3DTextureNode::getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MultiTexture &>(n).X3DTextureNode::setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "function", X3DFieldType::MFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const MultiTexture &>(n).getFunction());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MultiTexture &>(n).setFunction(
              std::any_cast<MFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const MultiTexture &>(n).X3DNode::getIS());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<MultiTexture &>(n).X3DNode::setIS(
                        std::any_cast<SFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const MultiTexture &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MultiTexture &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "mode", X3DFieldType::MFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const MultiTexture &>(n).getMode());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MultiTexture &>(n).setMode(std::any_cast<MFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "source", X3DFieldType::MFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const MultiTexture &>(n).getSource());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MultiTexture &>(n).setSource(std::any_cast<MFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "texture", X3DFieldType::MFNode, AccessType::InputOutput, "texture",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const MultiTexture &>(n).getTexture());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MultiTexture &>(n).setTexture(std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const MultiTexture &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MultiTexture &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const MultiTexture &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MultiTexture &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const MultiTexture &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MultiTexture &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"id", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const MultiTexture &>(n).X3DNode::getId());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<MultiTexture &>(n).X3DNode::setId(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const MultiTexture &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<MultiTexture &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void MultiTexture::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void MultiTexture::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesAlpha(getAlpha(), nodeTypeName(), "", out);

  checkRangesColor(getColor(), nodeTypeName(), "", out);
}

void MultiTexture::checkRangesAlpha(const SFFloat &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out) {
  if (value < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "alpha",
                                  "alpha below minimum of 0"});
  if (value > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "alpha",
                                  "alpha above maximum of 1"});
}

void MultiTexture::checkRangesColor(const SFColor &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out) {
  if (value.r < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "color",
                                  "color.r below minimum of 0"});
  if (value.r > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "color",
                                  "color.r above maximum of 1"});

  if (value.g < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "color",
                                  "color.g below minimum of 0"});
  if (value.g > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "color",
                                  "color.g above maximum of 1"});

  if (value.b < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "color",
                                  "color.b below minimum of 0"});
  if (value.b > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "color",
                                  "color.b above maximum of 1"});
}

namespace factory_detail {
std::shared_ptr<X3DNode> createMultiTexture() {
  return std::make_shared<MultiTexture>();
}
} // namespace factory_detail

} // namespace x3d::nodes
