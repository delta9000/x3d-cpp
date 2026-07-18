// TextureProperties.cpp
#include "x3d/nodes/TextureProperties.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string TextureProperties::nodeTypeName() const {
  return "TextureProperties";
}

std::string TextureProperties::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &TextureProperties::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "anisotropicDegree", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TextureProperties &>(n)
                              .getAnisotropicDegree());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureProperties &>(n).setAnisotropicDegreeUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "borderColor", X3DFieldType::SFColorRGBA, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TextureProperties &>(n).getBorderColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureProperties &>(n).setBorderColorUnchecked(
              std::any_cast<SFColorRGBA>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "borderWidth", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TextureProperties &>(n).getBorderWidth());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureProperties &>(n).setBorderWidthUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "boundaryModeR", X3DFieldType::SFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TextureProperties &>(n).getBoundaryModeR());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureProperties &>(n).setBoundaryModeR(
              std::any_cast<TextureBoundaryModeChoices>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(
              dynamic_cast<const TextureProperties &>(n).getBoundaryModeR());
        },

        [](X3DNode &n, const std::string &s) {
          TextureBoundaryModeChoices ev;
          if (from_string(s, ev))
            dynamic_cast<TextureProperties &>(n).setBoundaryModeR(ev);
        }

    });

    t.push_back(FieldInfo{
        "boundaryModeS", X3DFieldType::SFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TextureProperties &>(n).getBoundaryModeS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureProperties &>(n).setBoundaryModeS(
              std::any_cast<TextureBoundaryModeChoices>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(
              dynamic_cast<const TextureProperties &>(n).getBoundaryModeS());
        },

        [](X3DNode &n, const std::string &s) {
          TextureBoundaryModeChoices ev;
          if (from_string(s, ev))
            dynamic_cast<TextureProperties &>(n).setBoundaryModeS(ev);
        }

    });

    t.push_back(FieldInfo{
        "boundaryModeT", X3DFieldType::SFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TextureProperties &>(n).getBoundaryModeT());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureProperties &>(n).setBoundaryModeT(
              std::any_cast<TextureBoundaryModeChoices>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(
              dynamic_cast<const TextureProperties &>(n).getBoundaryModeT());
        },

        [](X3DNode &n, const std::string &s) {
          TextureBoundaryModeChoices ev;
          if (from_string(s, ev))
            dynamic_cast<TextureProperties &>(n).setBoundaryModeT(ev);
        }

    });

    t.push_back(FieldInfo{
        "generateMipMaps", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TextureProperties &>(n).getGenerateMipMaps());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureProperties &>(n).setGenerateMipMapsUnchecked(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TextureProperties &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureProperties &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "magnificationFilter", X3DFieldType::SFEnum, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TextureProperties &>(n)
                              .getMagnificationFilter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureProperties &>(n).setMagnificationFilter(
              std::any_cast<TextureMagnificationModeChoices>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(dynamic_cast<const TextureProperties &>(n)
                               .getMagnificationFilter());
        },

        [](X3DNode &n, const std::string &s) {
          TextureMagnificationModeChoices ev;
          if (from_string(s, ev))
            dynamic_cast<TextureProperties &>(n).setMagnificationFilter(ev);
        }

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TextureProperties &>(n)
                              .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureProperties &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "minificationFilter", X3DFieldType::SFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TextureProperties &>(n)
                              .getMinificationFilter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureProperties &>(n).setMinificationFilter(
              std::any_cast<TextureMinificationModeChoices>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(dynamic_cast<const TextureProperties &>(n)
                               .getMinificationFilter());
        },

        [](X3DNode &n, const std::string &s) {
          TextureMinificationModeChoices ev;
          if (from_string(s, ev))
            dynamic_cast<TextureProperties &>(n).setMinificationFilter(ev);
        }

    });

    t.push_back(FieldInfo{
        "textureCompression", X3DFieldType::SFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TextureProperties &>(n)
                              .getTextureCompression());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureProperties &>(n).setTextureCompression(
              std::any_cast<TextureCompressionModeChoices>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(dynamic_cast<const TextureProperties &>(n)
                               .getTextureCompression());
        },

        [](X3DNode &n, const std::string &s) {
          TextureCompressionModeChoices ev;
          if (from_string(s, ev))
            dynamic_cast<TextureProperties &>(n).setTextureCompression(ev);
        }

    });

    t.push_back(FieldInfo{
        "texturePriority", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TextureProperties &>(n).getTexturePriority());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureProperties &>(n).setTexturePriorityUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TextureProperties &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureProperties &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TextureProperties &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureProperties &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TextureProperties &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureProperties &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TextureProperties &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureProperties &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TextureProperties &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureProperties &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void TextureProperties::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void TextureProperties::validateRanges(
    std::vector<RangeDiagnostic> &out) const {

  checkRangesAnisotropicDegree(getAnisotropicDegree(), nodeTypeName(), "", out);

  checkRangesBorderColor(getBorderColor(), nodeTypeName(), "", out);

  checkRangesBorderWidth(getBorderWidth(), nodeTypeName(), "", out);

  checkRangesTexturePriority(getTexturePriority(), nodeTypeName(), "", out);
}

void TextureProperties::checkRangesAnisotropicDegree(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "anisotropicDegree",
                                  "anisotropicDegree below minimum of 1"});
}

void TextureProperties::checkRangesBorderColor(
    const SFColorRGBA &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value.r < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "borderColor",
                                  "borderColor.r below minimum of 0"});
  if (value.r > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "borderColor",
                                  "borderColor.r above maximum of 1"});

  if (value.g < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "borderColor",
                                  "borderColor.g below minimum of 0"});
  if (value.g > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "borderColor",
                                  "borderColor.g above maximum of 1"});

  if (value.b < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "borderColor",
                                  "borderColor.b below minimum of 0"});
  if (value.b > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "borderColor",
                                  "borderColor.b above maximum of 1"});

  if (value.a < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "borderColor",
                                  "borderColor.a below minimum of 0"});
  if (value.a > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "borderColor",
                                  "borderColor.a above maximum of 1"});
}

void TextureProperties::checkRangesBorderWidth(
    const SFInt32 &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "borderWidth",
                                  "borderWidth below minimum of 0"});
}

void TextureProperties::checkRangesTexturePriority(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "texturePriority",
                                  "texturePriority below minimum of 0"});
  if (value > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "texturePriority",
                                  "texturePriority above maximum of 1"});
}

namespace factory_detail {
std::shared_ptr<X3DNode> createTextureProperties() {
  return std::make_shared<TextureProperties>();
}
} // namespace factory_detail

} // namespace x3d::nodes
