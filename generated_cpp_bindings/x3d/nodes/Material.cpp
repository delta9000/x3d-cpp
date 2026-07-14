// Material.cpp
#include "x3d/nodes/Material.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string Material::nodeTypeName() const { return "Material"; }

std::string Material::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &Material::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "ambientIntensity", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Material &>(n).getAmbientIntensity());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Material &>(n).setAmbientIntensityUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"ambientTexture", X3DFieldType::SFNode,
                  AccessType::InputOutput, "ambientTexture",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const Material &>(n).getAmbientTexture());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<Material &>(n).setAmbientTexture(
                        std::any_cast<SFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "ambientTextureMapping", X3DFieldType::SFString,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Material &>(n).getAmbientTextureMapping());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Material &>(n).setAmbientTextureMapping(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "diffuseColor", X3DFieldType::SFColor, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Material &>(n).getDiffuseColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Material &>(n).setDiffuseColorUnchecked(
              std::any_cast<SFColor>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"diffuseTexture", X3DFieldType::SFNode,
                  AccessType::InputOutput, "diffuseTexture",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const Material &>(n).getDiffuseTexture());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<Material &>(n).setDiffuseTexture(
                        std::any_cast<SFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "diffuseTextureMapping", X3DFieldType::SFString,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Material &>(n).getDiffuseTextureMapping());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Material &>(n).setDiffuseTextureMapping(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "emissiveColor", X3DFieldType::SFColor, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Material &>(n).getEmissiveColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Material &>(n).setEmissiveColorUnchecked(
              std::any_cast<SFColor>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"emissiveTexture", X3DFieldType::SFNode,
                  AccessType::InputOutput, "emissiveTexture",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const Material &>(n).getEmissiveTexture());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<Material &>(n).setEmissiveTexture(
                        std::any_cast<SFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "emissiveTextureMapping", X3DFieldType::SFString,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Material &>(n)
                  .X3DOneSidedMaterialNode::getEmissiveTextureMapping());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Material &>(n)
              .X3DOneSidedMaterialNode::setEmissiveTextureMapping(
                  std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Material &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Material &>(n).X3DNode::setIS(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Material &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Material &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "normalScale", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Material &>(n)
                              .X3DOneSidedMaterialNode::getNormalScale());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Material &>(n)
              .X3DOneSidedMaterialNode::setNormalScaleUnchecked(
                  std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "normalTexture", X3DFieldType::SFNode, AccessType::InputOutput,
        "normalTexture",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Material &>(n).getNormalTexture());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Material &>(n).setNormalTexture(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "normalTextureMapping", X3DFieldType::SFString, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Material &>(n)
                  .X3DOneSidedMaterialNode::getNormalTextureMapping());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Material &>(n)
              .X3DOneSidedMaterialNode::setNormalTextureMapping(
                  std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "occlusionStrength", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Material &>(n).getOcclusionStrength());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Material &>(n).setOcclusionStrengthUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "occlusionTexture", X3DFieldType::SFNode, AccessType::InputOutput,
        "occlusionTexture",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Material &>(n).getOcclusionTexture());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Material &>(n).setOcclusionTexture(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "occlusionTextureMapping", X3DFieldType::SFString,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Material &>(n).getOcclusionTextureMapping());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Material &>(n).setOcclusionTextureMapping(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "shininess", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Material &>(n).getShininess());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Material &>(n).setShininessUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "shininessTexture", X3DFieldType::SFNode, AccessType::InputOutput,
        "shininessTexture",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Material &>(n).getShininessTexture());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Material &>(n).setShininessTexture(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "shininessTextureMapping", X3DFieldType::SFString,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Material &>(n).getShininessTextureMapping());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Material &>(n).setShininessTextureMapping(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "specularColor", X3DFieldType::SFColor, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Material &>(n).getSpecularColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Material &>(n).setSpecularColorUnchecked(
              std::any_cast<SFColor>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"specularTexture", X3DFieldType::SFNode,
                  AccessType::InputOutput, "specularTexture",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const Material &>(n).getSpecularTexture());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<Material &>(n).setSpecularTexture(
                        std::any_cast<SFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "specularTextureMapping", X3DFieldType::SFString,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Material &>(n).getSpecularTextureMapping());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Material &>(n).setSpecularTextureMapping(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "transparency", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Material &>(n).getTransparency());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Material &>(n).setTransparencyUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Material &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Material &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Material &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Material &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"class", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const Material &>(n).X3DNode::getClass_());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<Material &>(n).X3DNode::setClass_(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Material &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Material &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"style", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const Material &>(n).X3DNode::getStyle());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<Material &>(n).X3DNode::setStyle(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    return t;
  }();
  return table;
}

void Material::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void Material::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesAmbientIntensity(getAmbientIntensity(), nodeTypeName(), "", out);

  checkRangesDiffuseColor(getDiffuseColor(), nodeTypeName(), "", out);

  checkRangesEmissiveColor(getEmissiveColor(), nodeTypeName(), "", out);

  X3DOneSidedMaterialNode::checkRangesNormalScale(
      X3DOneSidedMaterialNode::getNormalScale(), nodeTypeName(), "", out);

  checkRangesOcclusionStrength(getOcclusionStrength(), nodeTypeName(), "", out);

  checkRangesShininess(getShininess(), nodeTypeName(), "", out);

  checkRangesSpecularColor(getSpecularColor(), nodeTypeName(), "", out);

  checkRangesTransparency(getTransparency(), nodeTypeName(), "", out);
}

void Material::checkRangesAmbientIntensity(const SFFloat &value,
                                           const std::string &nodeType,
                                           const std::string &defName,
                                           std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "ambientIntensity",
                                  "ambientIntensity below minimum of 0"});
  if (value > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "ambientIntensity",
                                  "ambientIntensity above maximum of 1"});
}

void Material::checkRangesDiffuseColor(const SFColor &value,
                                       const std::string &nodeType,
                                       const std::string &defName,
                                       std::vector<RangeDiagnostic> &out) {
  if (value.r < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "diffuseColor",
                                  "diffuseColor.r below minimum of 0"});
  if (value.r > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "diffuseColor",
                                  "diffuseColor.r above maximum of 1"});

  if (value.g < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "diffuseColor",
                                  "diffuseColor.g below minimum of 0"});
  if (value.g > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "diffuseColor",
                                  "diffuseColor.g above maximum of 1"});

  if (value.b < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "diffuseColor",
                                  "diffuseColor.b below minimum of 0"});
  if (value.b > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "diffuseColor",
                                  "diffuseColor.b above maximum of 1"});
}

void Material::checkRangesEmissiveColor(const SFColor &value,
                                        const std::string &nodeType,
                                        const std::string &defName,
                                        std::vector<RangeDiagnostic> &out) {
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

void Material::checkRangesOcclusionStrength(const SFFloat &value,
                                            const std::string &nodeType,
                                            const std::string &defName,
                                            std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "occlusionStrength",
                                  "occlusionStrength below minimum of 0"});
  if (value > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "occlusionStrength",
                                  "occlusionStrength above maximum of 1"});
}

void Material::checkRangesShininess(const SFFloat &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "shininess",
                                  "shininess below minimum of 0"});
  if (value > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "shininess",
                                  "shininess above maximum of 1"});
}

void Material::checkRangesSpecularColor(const SFColor &value,
                                        const std::string &nodeType,
                                        const std::string &defName,
                                        std::vector<RangeDiagnostic> &out) {
  if (value.r < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "specularColor",
                                  "specularColor.r below minimum of 0"});
  if (value.r > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "specularColor",
                                  "specularColor.r above maximum of 1"});

  if (value.g < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "specularColor",
                                  "specularColor.g below minimum of 0"});
  if (value.g > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "specularColor",
                                  "specularColor.g above maximum of 1"});

  if (value.b < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "specularColor",
                                  "specularColor.b below minimum of 0"});
  if (value.b > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "specularColor",
                                  "specularColor.b above maximum of 1"});
}

void Material::checkRangesTransparency(const SFFloat &value,
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

namespace factory_detail {
std::shared_ptr<X3DNode> createMaterial() {
  return std::make_shared<Material>();
}
} // namespace factory_detail

} // namespace x3d::nodes
