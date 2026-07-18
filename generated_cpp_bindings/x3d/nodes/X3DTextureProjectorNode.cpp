// X3DTextureProjectorNode.cpp
#include "x3d/nodes/X3DTextureProjectorNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string X3DTextureProjectorNode::nodeTypeName() const {
  return "X3DTextureProjectorNode";
}

std::string X3DTextureProjectorNode::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &X3DTextureProjectorNode::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "ambientIntensity", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DTextureProjectorNode &>(n)
                              .X3DLightNode::getAmbientIntensity());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DTextureProjectorNode &>(n)
              .X3DLightNode::setAmbientIntensityUnchecked(
                  std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "aspectRatio", X3DFieldType::SFFloat, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DTextureProjectorNode &>(n)
                              .getAspectRatio());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DTextureProjectorNode &>(n).emitAspectRatio(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "color", X3DFieldType::SFColor, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DTextureProjectorNode &>(n)
                              .X3DLightNode::getColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DTextureProjectorNode &>(n)
              .X3DLightNode::setColorUnchecked(std::any_cast<SFColor>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DTextureProjectorNode &>(n)
                              .getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DTextureProjectorNode &>(n).setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "direction", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DTextureProjectorNode &>(n).getDirection());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DTextureProjectorNode &>(n).setDirection(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "farDistance", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DTextureProjectorNode &>(n)
                              .getFarDistance());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DTextureProjectorNode &>(n).setFarDistanceUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "global", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DTextureProjectorNode &>(n).getGlobal());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DTextureProjectorNode &>(n).setGlobal(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "intensity", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DTextureProjectorNode &>(n)
                              .X3DLightNode::getIntensity());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DTextureProjectorNode &>(n)
              .X3DLightNode::setIntensityUnchecked(std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DTextureProjectorNode &>(n)
                              .X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DTextureProjectorNode &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "location", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DTextureProjectorNode &>(n).getLocation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DTextureProjectorNode &>(n).setLocation(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DTextureProjectorNode &>(n)
                              .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DTextureProjectorNode &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "nearDistance", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DTextureProjectorNode &>(n)
                              .getNearDistance());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DTextureProjectorNode &>(n).setNearDistanceUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "on", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DTextureProjectorNode &>(n)
                              .X3DLightNode::getOn());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DTextureProjectorNode &>(n).X3DLightNode::setOn(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "shadowIntensity", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DTextureProjectorNode &>(n)
                              .X3DLightNode::getShadowIntensity());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DTextureProjectorNode &>(n)
              .X3DLightNode::setShadowIntensityUnchecked(
                  std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "shadows", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DTextureProjectorNode &>(n)
                              .X3DLightNode::getShadows());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DTextureProjectorNode &>(n).X3DLightNode::setShadows(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "texture", X3DFieldType::SFNode, AccessType::InputOutput, "texture",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DTextureProjectorNode &>(n).getTexture());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DTextureProjectorNode &>(n).setTexture(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DTextureProjectorNode &>(n)
                              .X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DTextureProjectorNode &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DTextureProjectorNode &>(n)
                              .X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DTextureProjectorNode &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DTextureProjectorNode &>(n)
                              .X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DTextureProjectorNode &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DTextureProjectorNode &>(n)
                              .X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DTextureProjectorNode &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DTextureProjectorNode &>(n)
                              .X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DTextureProjectorNode &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void X3DTextureProjectorNode::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void X3DTextureProjectorNode::validateRanges(
    std::vector<RangeDiagnostic> &out) const {

  X3DLightNode::checkRangesAmbientIntensity(X3DLightNode::getAmbientIntensity(),
                                            nodeTypeName(), "", out);

  X3DLightNode::checkRangesColor(X3DLightNode::getColor(), nodeTypeName(), "",
                                 out);

  checkRangesFarDistance(getFarDistance(), nodeTypeName(), "", out);

  X3DLightNode::checkRangesIntensity(X3DLightNode::getIntensity(),
                                     nodeTypeName(), "", out);

  checkRangesNearDistance(getNearDistance(), nodeTypeName(), "", out);

  X3DLightNode::checkRangesShadowIntensity(X3DLightNode::getShadowIntensity(),
                                           nodeTypeName(), "", out);
}

void X3DTextureProjectorNode::checkRangesFarDistance(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < -1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "farDistance",
                                  "farDistance below minimum of -1"});
}

void X3DTextureProjectorNode::checkRangesNearDistance(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < -1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "nearDistance",
                                  "nearDistance below minimum of -1"});
}

} // namespace x3d::nodes
