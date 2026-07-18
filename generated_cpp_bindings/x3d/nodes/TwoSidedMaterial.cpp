// TwoSidedMaterial.cpp
#include "x3d/nodes/TwoSidedMaterial.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string TwoSidedMaterial::nodeTypeName() const {
  return "TwoSidedMaterial";
}

std::string TwoSidedMaterial::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &TwoSidedMaterial::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "ambientIntensity", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TwoSidedMaterial &>(n).getAmbientIntensity());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TwoSidedMaterial &>(n).setAmbientIntensityUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "backAmbientIntensity", X3DFieldType::SFFloat, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TwoSidedMaterial &>(n)
                              .getBackAmbientIntensity());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TwoSidedMaterial &>(n).setBackAmbientIntensityUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "backDiffuseColor", X3DFieldType::SFColor, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TwoSidedMaterial &>(n).getBackDiffuseColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TwoSidedMaterial &>(n).setBackDiffuseColorUnchecked(
              std::any_cast<SFColor>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "backEmissiveColor", X3DFieldType::SFColor, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TwoSidedMaterial &>(n).getBackEmissiveColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TwoSidedMaterial &>(n).setBackEmissiveColorUnchecked(
              std::any_cast<SFColor>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "backShininess", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TwoSidedMaterial &>(n).getBackShininess());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TwoSidedMaterial &>(n).setBackShininessUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "backSpecularColor", X3DFieldType::SFColor, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TwoSidedMaterial &>(n).getBackSpecularColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TwoSidedMaterial &>(n).setBackSpecularColorUnchecked(
              std::any_cast<SFColor>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "backTransparency", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TwoSidedMaterial &>(n).getBackTransparency());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TwoSidedMaterial &>(n).setBackTransparencyUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "diffuseColor", X3DFieldType::SFColor, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TwoSidedMaterial &>(n).getDiffuseColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TwoSidedMaterial &>(n).setDiffuseColorUnchecked(
              std::any_cast<SFColor>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "emissiveColor", X3DFieldType::SFColor, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TwoSidedMaterial &>(n).getEmissiveColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TwoSidedMaterial &>(n).setEmissiveColorUnchecked(
              std::any_cast<SFColor>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TwoSidedMaterial &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TwoSidedMaterial &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TwoSidedMaterial &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TwoSidedMaterial &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "separateBackColor", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TwoSidedMaterial &>(n).getSeparateBackColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TwoSidedMaterial &>(n).setSeparateBackColor(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "shininess", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TwoSidedMaterial &>(n).getShininess());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TwoSidedMaterial &>(n).setShininessUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "specularColor", X3DFieldType::SFColor, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TwoSidedMaterial &>(n).getSpecularColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TwoSidedMaterial &>(n).setSpecularColorUnchecked(
              std::any_cast<SFColor>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "transparency", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TwoSidedMaterial &>(n).getTransparency());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TwoSidedMaterial &>(n).setTransparencyUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TwoSidedMaterial &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TwoSidedMaterial &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TwoSidedMaterial &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TwoSidedMaterial &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TwoSidedMaterial &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TwoSidedMaterial &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TwoSidedMaterial &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TwoSidedMaterial &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TwoSidedMaterial &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TwoSidedMaterial &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void TwoSidedMaterial::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void TwoSidedMaterial::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesAmbientIntensity(getAmbientIntensity(), nodeTypeName(), "", out);

  checkRangesBackAmbientIntensity(getBackAmbientIntensity(), nodeTypeName(), "",
                                  out);

  checkRangesBackDiffuseColor(getBackDiffuseColor(), nodeTypeName(), "", out);

  checkRangesBackEmissiveColor(getBackEmissiveColor(), nodeTypeName(), "", out);

  checkRangesBackShininess(getBackShininess(), nodeTypeName(), "", out);

  checkRangesBackSpecularColor(getBackSpecularColor(), nodeTypeName(), "", out);

  checkRangesBackTransparency(getBackTransparency(), nodeTypeName(), "", out);

  checkRangesDiffuseColor(getDiffuseColor(), nodeTypeName(), "", out);

  checkRangesEmissiveColor(getEmissiveColor(), nodeTypeName(), "", out);

  checkRangesShininess(getShininess(), nodeTypeName(), "", out);

  checkRangesSpecularColor(getSpecularColor(), nodeTypeName(), "", out);

  checkRangesTransparency(getTransparency(), nodeTypeName(), "", out);
}

void TwoSidedMaterial::checkRangesAmbientIntensity(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "ambientIntensity",
                                  "ambientIntensity below minimum of 0"});
  if (value > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "ambientIntensity",
                                  "ambientIntensity above maximum of 1"});
}

void TwoSidedMaterial::checkRangesBackAmbientIntensity(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "backAmbientIntensity",
                                  "backAmbientIntensity below minimum of 0"});
  if (value > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "backAmbientIntensity",
                                  "backAmbientIntensity above maximum of 1"});
}

void TwoSidedMaterial::checkRangesBackDiffuseColor(
    const SFColor &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value.r < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "backDiffuseColor",
                                  "backDiffuseColor.r below minimum of 0"});
  if (value.r > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "backDiffuseColor",
                                  "backDiffuseColor.r above maximum of 1"});

  if (value.g < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "backDiffuseColor",
                                  "backDiffuseColor.g below minimum of 0"});
  if (value.g > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "backDiffuseColor",
                                  "backDiffuseColor.g above maximum of 1"});

  if (value.b < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "backDiffuseColor",
                                  "backDiffuseColor.b below minimum of 0"});
  if (value.b > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "backDiffuseColor",
                                  "backDiffuseColor.b above maximum of 1"});
}

void TwoSidedMaterial::checkRangesBackEmissiveColor(
    const SFColor &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value.r < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "backEmissiveColor",
                                  "backEmissiveColor.r below minimum of 0"});
  if (value.r > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "backEmissiveColor",
                                  "backEmissiveColor.r above maximum of 1"});

  if (value.g < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "backEmissiveColor",
                                  "backEmissiveColor.g below minimum of 0"});
  if (value.g > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "backEmissiveColor",
                                  "backEmissiveColor.g above maximum of 1"});

  if (value.b < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "backEmissiveColor",
                                  "backEmissiveColor.b below minimum of 0"});
  if (value.b > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "backEmissiveColor",
                                  "backEmissiveColor.b above maximum of 1"});
}

void TwoSidedMaterial::checkRangesBackShininess(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "backShininess",
                                  "backShininess below minimum of 0"});
  if (value > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "backShininess",
                                  "backShininess above maximum of 1"});
}

void TwoSidedMaterial::checkRangesBackSpecularColor(
    const SFColor &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value.r < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "backSpecularColor",
                                  "backSpecularColor.r below minimum of 0"});
  if (value.r > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "backSpecularColor",
                                  "backSpecularColor.r above maximum of 1"});

  if (value.g < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "backSpecularColor",
                                  "backSpecularColor.g below minimum of 0"});
  if (value.g > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "backSpecularColor",
                                  "backSpecularColor.g above maximum of 1"});

  if (value.b < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "backSpecularColor",
                                  "backSpecularColor.b below minimum of 0"});
  if (value.b > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "backSpecularColor",
                                  "backSpecularColor.b above maximum of 1"});
}

void TwoSidedMaterial::checkRangesBackTransparency(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "backTransparency",
                                  "backTransparency below minimum of 0"});
  if (value > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "backTransparency",
                                  "backTransparency above maximum of 1"});
}

void TwoSidedMaterial::checkRangesDiffuseColor(
    const SFColor &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value.r < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "diffuseColor",
                                  "diffuseColor.r below minimum of 0"});
  if (value.r > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "diffuseColor",
                                  "diffuseColor.r above maximum of 1"});

  if (value.g < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "diffuseColor",
                                  "diffuseColor.g below minimum of 0"});
  if (value.g > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "diffuseColor",
                                  "diffuseColor.g above maximum of 1"});

  if (value.b < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "diffuseColor",
                                  "diffuseColor.b below minimum of 0"});
  if (value.b > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "diffuseColor",
                                  "diffuseColor.b above maximum of 1"});
}

void TwoSidedMaterial::checkRangesEmissiveColor(
    const SFColor &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value.r < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "emissiveColor",
                                  "emissiveColor.r below minimum of 0"});
  if (value.r > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "emissiveColor",
                                  "emissiveColor.r above maximum of 1"});

  if (value.g < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "emissiveColor",
                                  "emissiveColor.g below minimum of 0"});
  if (value.g > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "emissiveColor",
                                  "emissiveColor.g above maximum of 1"});

  if (value.b < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "emissiveColor",
                                  "emissiveColor.b below minimum of 0"});
  if (value.b > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "emissiveColor",
                                  "emissiveColor.b above maximum of 1"});
}

void TwoSidedMaterial::checkRangesShininess(const SFFloat &value,
                                            const std::string &nodeType,
                                            const std::string &defName,
                                            std::vector<RangeDiagnostic> &out) {
  if (value < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "shininess",
                                  "shininess below minimum of 0"});
  if (value > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "shininess",
                                  "shininess above maximum of 1"});
}

void TwoSidedMaterial::checkRangesSpecularColor(
    const SFColor &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value.r < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "specularColor",
                                  "specularColor.r below minimum of 0"});
  if (value.r > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "specularColor",
                                  "specularColor.r above maximum of 1"});

  if (value.g < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "specularColor",
                                  "specularColor.g below minimum of 0"});
  if (value.g > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "specularColor",
                                  "specularColor.g above maximum of 1"});

  if (value.b < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "specularColor",
                                  "specularColor.b below minimum of 0"});
  if (value.b > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "specularColor",
                                  "specularColor.b above maximum of 1"});
}

void TwoSidedMaterial::checkRangesTransparency(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "transparency",
                                  "transparency below minimum of 0"});
  if (value > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "transparency",
                                  "transparency above maximum of 1"});
}

namespace factory_detail {
std::shared_ptr<X3DNode> createTwoSidedMaterial() {
  return std::make_shared<TwoSidedMaterial>();
}
} // namespace factory_detail

} // namespace x3d::nodes
