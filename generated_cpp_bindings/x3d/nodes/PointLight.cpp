// PointLight.cpp
#include "x3d/nodes/PointLight.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string PointLight::nodeTypeName() const { return "PointLight"; }

std::string PointLight::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &PointLight::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "ambientIntensity", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PointLight &>(n)
                              .X3DLightNode::getAmbientIntensity());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointLight &>(n)
              .X3DLightNode::setAmbientIntensityUnchecked(
                  std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "attenuation", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PointLight &>(n).getAttenuation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointLight &>(n).setAttenuationUnchecked(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "color", X3DFieldType::SFColor, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PointLight &>(n).X3DLightNode::getColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointLight &>(n).X3DLightNode::setColorUnchecked(
              std::any_cast<SFColor>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "global", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PointLight &>(n).getGlobal());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointLight &>(n).setGlobal(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "intensity", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PointLight &>(n).X3DLightNode::getIntensity());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointLight &>(n).X3DLightNode::setIntensityUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PointLight &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointLight &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "location", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PointLight &>(n).getLocation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointLight &>(n).setLocation(std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PointLight &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointLight &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "on", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PointLight &>(n).X3DLightNode::getOn());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointLight &>(n).X3DLightNode::setOn(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "radius", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PointLight &>(n).getRadius());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointLight &>(n).setRadiusUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "shadowIntensity", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PointLight &>(n)
                              .X3DLightNode::getShadowIntensity());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointLight &>(n)
              .X3DLightNode::setShadowIntensityUnchecked(
                  std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "shadows", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PointLight &>(n).X3DLightNode::getShadows());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointLight &>(n).X3DLightNode::setShadows(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const PointLight &>(n).X3DNode::getDEF());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<PointLight &>(n).X3DNode::setDEF(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(
        FieldInfo{"USE", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const PointLight &>(n).X3DNode::getUSE());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<PointLight &>(n).X3DNode::setUSE(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PointLight &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointLight &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PointLight &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointLight &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PointLight &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointLight &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void PointLight::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void PointLight::validateRanges(std::vector<RangeDiagnostic> &out) const {

  X3DLightNode::checkRangesAmbientIntensity(X3DLightNode::getAmbientIntensity(),
                                            nodeTypeName(), "", out);

  checkRangesAttenuation(getAttenuation(), nodeTypeName(), "", out);

  X3DLightNode::checkRangesColor(X3DLightNode::getColor(), nodeTypeName(), "",
                                 out);

  X3DLightNode::checkRangesIntensity(X3DLightNode::getIntensity(),
                                     nodeTypeName(), "", out);

  checkRangesRadius(getRadius(), nodeTypeName(), "", out);

  X3DLightNode::checkRangesShadowIntensity(X3DLightNode::getShadowIntensity(),
                                           nodeTypeName(), "", out);
}

void PointLight::checkRangesAttenuation(const SFVec3f &value,
                                        const std::string &nodeType,
                                        const std::string &defName,
                                        std::vector<RangeDiagnostic> &out) {
  if (value.x < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "attenuation",
                                  "attenuation.x below minimum of 0"});

  if (value.y < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "attenuation",
                                  "attenuation.y below minimum of 0"});

  if (value.z < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "attenuation",
                                  "attenuation.z below minimum of 0"});
}

void PointLight::checkRangesRadius(const SFFloat &value,
                                   const std::string &nodeType,
                                   const std::string &defName,
                                   std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "radius",
                                  "radius below minimum of 0"});
}

namespace factory_detail {
std::shared_ptr<X3DNode> createPointLight() {
  return std::make_shared<PointLight>();
}
} // namespace factory_detail

} // namespace x3d::nodes
