// PhysicalMaterial.cpp
#include "PhysicalMaterial.hpp"

std::string PhysicalMaterial::nodeTypeName() const {
  return "PhysicalMaterial";
}

std::string PhysicalMaterial::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &PhysicalMaterial::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "baseColor", X3DFieldType::SFColor, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PhysicalMaterial &>(n).getBaseColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PhysicalMaterial &>(n).setBaseColorUnchecked(
              std::any_cast<SFColor>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "baseTexture", X3DFieldType::SFNode, AccessType::InputOutput,
        "baseTexture",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PhysicalMaterial &>(n).getBaseTexture());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PhysicalMaterial &>(n).setBaseTexture(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"baseTextureMapping", X3DFieldType::SFString,
                  AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const PhysicalMaterial &>(n)
                                        .getBaseTextureMapping());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<PhysicalMaterial &>(n).setBaseTextureMapping(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "emissiveColor", X3DFieldType::SFColor, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PhysicalMaterial &>(n).getEmissiveColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PhysicalMaterial &>(n).setEmissiveColorUnchecked(
              std::any_cast<SFColor>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "emissiveTexture", X3DFieldType::SFNode, AccessType::InputOutput,
        "emissiveTexture",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PhysicalMaterial &>(n).getEmissiveTexture());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PhysicalMaterial &>(n).setEmissiveTexture(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "emissiveTextureMapping", X3DFieldType::SFString,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PhysicalMaterial &>(n)
                  .X3DOneSidedMaterialNode::getEmissiveTextureMapping());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PhysicalMaterial &>(n)
              .X3DOneSidedMaterialNode::setEmissiveTextureMapping(
                  std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PhysicalMaterial &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PhysicalMaterial &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PhysicalMaterial &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PhysicalMaterial &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metallic", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PhysicalMaterial &>(n).getMetallic());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PhysicalMaterial &>(n).setMetallicUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metallicRoughnessTexture", X3DFieldType::SFNode,
        AccessType::InputOutput, "metallicRoughnessTexture",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PhysicalMaterial &>(n)
                              .getMetallicRoughnessTexture());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PhysicalMaterial &>(n).setMetallicRoughnessTexture(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metallicRoughnessTextureMapping", X3DFieldType::SFString,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PhysicalMaterial &>(n)
                              .getMetallicRoughnessTextureMapping());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PhysicalMaterial &>(n)
              .setMetallicRoughnessTextureMapping(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "normalScale", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PhysicalMaterial &>(n)
                              .X3DOneSidedMaterialNode::getNormalScale());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PhysicalMaterial &>(n)
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
              dynamic_cast<const PhysicalMaterial &>(n).getNormalTexture());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PhysicalMaterial &>(n).setNormalTexture(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "normalTextureMapping", X3DFieldType::SFString, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PhysicalMaterial &>(n)
                  .X3DOneSidedMaterialNode::getNormalTextureMapping());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PhysicalMaterial &>(n)
              .X3DOneSidedMaterialNode::setNormalTextureMapping(
                  std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "occlusionStrength", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PhysicalMaterial &>(n).getOcclusionStrength());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PhysicalMaterial &>(n).setOcclusionStrengthUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "occlusionTexture", X3DFieldType::SFNode, AccessType::InputOutput,
        "occlusionTexture",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PhysicalMaterial &>(n).getOcclusionTexture());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PhysicalMaterial &>(n).setOcclusionTexture(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "occlusionTextureMapping", X3DFieldType::SFString,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PhysicalMaterial &>(n)
                              .getOcclusionTextureMapping());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PhysicalMaterial &>(n).setOcclusionTextureMapping(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "roughness", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PhysicalMaterial &>(n).getRoughness());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PhysicalMaterial &>(n).setRoughnessUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "transparency", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PhysicalMaterial &>(n).getTransparency());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PhysicalMaterial &>(n).setTransparencyUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PhysicalMaterial &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PhysicalMaterial &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PhysicalMaterial &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PhysicalMaterial &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PhysicalMaterial &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PhysicalMaterial &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PhysicalMaterial &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PhysicalMaterial &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PhysicalMaterial &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PhysicalMaterial &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void PhysicalMaterial::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void PhysicalMaterial::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesBaseColor(getBaseColor(), nodeTypeName(), "", out);

  checkRangesEmissiveColor(getEmissiveColor(), nodeTypeName(), "", out);

  checkRangesMetallic(getMetallic(), nodeTypeName(), "", out);

  X3DOneSidedMaterialNode::checkRangesNormalScale(
      X3DOneSidedMaterialNode::getNormalScale(), nodeTypeName(), "", out);

  checkRangesOcclusionStrength(getOcclusionStrength(), nodeTypeName(), "", out);

  checkRangesRoughness(getRoughness(), nodeTypeName(), "", out);

  checkRangesTransparency(getTransparency(), nodeTypeName(), "", out);
}

void PhysicalMaterial::checkRangesBaseColor(const SFColor &value,
                                            const std::string &nodeType,
                                            const std::string &defName,
                                            std::vector<RangeDiagnostic> &out) {
  if (value.r < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "baseColor",
                                  "baseColor.r below minimum of 0"});
  if (value.r > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "baseColor",
                                  "baseColor.r above maximum of 1"});

  if (value.g < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "baseColor",
                                  "baseColor.g below minimum of 0"});
  if (value.g > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "baseColor",
                                  "baseColor.g above maximum of 1"});

  if (value.b < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "baseColor",
                                  "baseColor.b below minimum of 0"});
  if (value.b > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "baseColor",
                                  "baseColor.b above maximum of 1"});
}

void PhysicalMaterial::checkRangesEmissiveColor(
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

void PhysicalMaterial::checkRangesMetallic(const SFFloat &value,
                                           const std::string &nodeType,
                                           const std::string &defName,
                                           std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "metallic",
                                  "metallic below minimum of 0"});
  if (value > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "metallic",
                                  "metallic above maximum of 1"});
}

void PhysicalMaterial::checkRangesOcclusionStrength(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "occlusionStrength",
                                  "occlusionStrength below minimum of 0"});
  if (value > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "occlusionStrength",
                                  "occlusionStrength above maximum of 1"});
}

void PhysicalMaterial::checkRangesRoughness(const SFFloat &value,
                                            const std::string &nodeType,
                                            const std::string &defName,
                                            std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "roughness",
                                  "roughness below minimum of 0"});
  if (value > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "roughness",
                                  "roughness above maximum of 1"});
}

void PhysicalMaterial::checkRangesTransparency(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "transparency",
                                  "transparency below minimum of 0"});
  if (value > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "transparency",
                                  "transparency above maximum of 1"});
}
