// UnlitMaterial.cpp
#include "UnlitMaterial.hpp"

std::string UnlitMaterial::nodeTypeName() const { return "UnlitMaterial"; }

std::string UnlitMaterial::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &UnlitMaterial::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "emissiveColor", X3DFieldType::SFColor, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const UnlitMaterial &>(n).getEmissiveColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UnlitMaterial &>(n).setEmissiveColorUnchecked(
              std::any_cast<SFColor>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "emissiveTexture", X3DFieldType::SFNode, AccessType::InputOutput,
        "emissiveTexture",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const UnlitMaterial &>(n).getEmissiveTexture());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UnlitMaterial &>(n).setEmissiveTexture(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "emissiveTextureMapping", X3DFieldType::SFString,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const UnlitMaterial &>(n)
                  .X3DOneSidedMaterialNode::getEmissiveTextureMapping());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UnlitMaterial &>(n)
              .X3DOneSidedMaterialNode::setEmissiveTextureMapping(
                  std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const UnlitMaterial &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UnlitMaterial &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const UnlitMaterial &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UnlitMaterial &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "normalScale", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const UnlitMaterial &>(n)
                              .X3DOneSidedMaterialNode::getNormalScale());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UnlitMaterial &>(n)
              .X3DOneSidedMaterialNode::setNormalScaleUnchecked(
                  std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "normalTexture", X3DFieldType::SFNode, AccessType::InputOutput,
        "normalTexture",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const UnlitMaterial &>(n).getNormalTexture());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UnlitMaterial &>(n).setNormalTexture(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "normalTextureMapping", X3DFieldType::SFString, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const UnlitMaterial &>(n)
                  .X3DOneSidedMaterialNode::getNormalTextureMapping());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UnlitMaterial &>(n)
              .X3DOneSidedMaterialNode::setNormalTextureMapping(
                  std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "transparency", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const UnlitMaterial &>(n).getTransparency());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UnlitMaterial &>(n).setTransparencyUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const UnlitMaterial &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UnlitMaterial &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const UnlitMaterial &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UnlitMaterial &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const UnlitMaterial &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UnlitMaterial &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const UnlitMaterial &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UnlitMaterial &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const UnlitMaterial &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<UnlitMaterial &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void UnlitMaterial::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void UnlitMaterial::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesEmissiveColor(getEmissiveColor(), nodeTypeName(), "", out);

  X3DOneSidedMaterialNode::checkRangesNormalScale(
      X3DOneSidedMaterialNode::getNormalScale(), nodeTypeName(), "", out);

  checkRangesTransparency(getTransparency(), nodeTypeName(), "", out);
}

void UnlitMaterial::checkRangesEmissiveColor(
    const SFColor &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value.r < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "emissiveColor",
                                  "emissiveColor.r below minimum of 0"});
  if (value.r > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "emissiveColor",
                                  "emissiveColor.r above maximum of 1"});

  if (value.g < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "emissiveColor",
                                  "emissiveColor.g below minimum of 0"});
  if (value.g > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "emissiveColor",
                                  "emissiveColor.g above maximum of 1"});

  if (value.b < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "emissiveColor",
                                  "emissiveColor.b below minimum of 0"});
  if (value.b > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "emissiveColor",
                                  "emissiveColor.b above maximum of 1"});
}

void UnlitMaterial::checkRangesTransparency(const SFFloat &value,
                                            const std::string &nodeType,
                                            const std::string &defName,
                                            std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "transparency",
                                  "transparency below minimum of 0"});
  if (value > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "transparency",
                                  "transparency above maximum of 1"});
}
