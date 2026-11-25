#include "x3d/nodes/EnvironmentLight.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string EnvironmentLight::nodeTypeName() const {
  return "EnvironmentLight";
}

std::string EnvironmentLight::defaultContainerField() const {
  return getDefaultContainerField();
}

X3DVersion EnvironmentLight::introducedIn() const { return X3DVersion{4, 1}; }

const FieldTable &EnvironmentLight::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "ambientIntensity", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EnvironmentLight &>(n)
                              .X3DLightNode::getAmbientIntensity());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EnvironmentLight &>(n)
              .X3DLightNode::setAmbientIntensityUnchecked(
                  std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const EnvironmentLight &>(n)
                       .X3DLightNode::getAmbientIntensity(),
                  &typeid(SFFloat)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "color", X3DFieldType::SFColor, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EnvironmentLight &>(n)
                              .X3DLightNode::getColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EnvironmentLight &>(n).X3DLightNode::setColorUnchecked(
              std::any_cast<SFColor>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const EnvironmentLight &>(n)
                       .X3DLightNode::getColor(),
                  &typeid(SFColor)};
        },

        X3DVersion{4, 1}});

    t.push_back(
        FieldInfo{"diffuseCoefficients", X3DFieldType::MFFloat,
                  AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const EnvironmentLight &>(n)
                                        .getDiffuseCoefficients());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<EnvironmentLight &>(n).setDiffuseCoefficients(
                        std::any_cast<MFFloat>(v));
                  },

                  nullptr, nullptr

                  ,

                  [](const X3DNode &n) -> FieldView {
                    return {&dynamic_cast<const EnvironmentLight &>(n)
                                 .getDiffuseCoefficients(),
                            &typeid(MFFloat)};
                  },

                  X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "diffuseTexture", X3DFieldType::SFNode, AccessType::InputOutput,
        "diffuseTexture",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EnvironmentLight &>(n).getDiffuseTexture());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EnvironmentLight &>(n).setDiffuseTexture(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {
              &dynamic_cast<const EnvironmentLight &>(n).getDiffuseTexture(),
              &typeid(SFNode)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "global", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EnvironmentLight &>(n).getGlobal());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EnvironmentLight &>(n).setGlobal(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const EnvironmentLight &>(n).getGlobal(),
                  &typeid(SFBool)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "intensity", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EnvironmentLight &>(n)
                              .X3DLightNode::getIntensity());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EnvironmentLight &>(n)
              .X3DLightNode::setIntensityUnchecked(std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const EnvironmentLight &>(n)
                       .X3DLightNode::getIntensity(),
                  &typeid(SFFloat)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EnvironmentLight &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EnvironmentLight &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const EnvironmentLight &>(n).X3DNode::getIS(),
                  &typeid(SFNode)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EnvironmentLight &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EnvironmentLight &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {
              &dynamic_cast<const EnvironmentLight &>(n).X3DNode::getMetadata(),
              &typeid(SFNode)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "on", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EnvironmentLight &>(n).X3DLightNode::getOn());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EnvironmentLight &>(n).X3DLightNode::setOn(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {
              &dynamic_cast<const EnvironmentLight &>(n).X3DLightNode::getOn(),
              &typeid(SFBool)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "origin", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EnvironmentLight &>(n).getOrigin());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EnvironmentLight &>(n).setOrigin(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const EnvironmentLight &>(n).getOrigin(),
                  &typeid(SFVec3f)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "rotation", X3DFieldType::SFRotation, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EnvironmentLight &>(n).getRotation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EnvironmentLight &>(n).setRotation(
              std::any_cast<SFRotation>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const EnvironmentLight &>(n).getRotation(),
                  &typeid(SFRotation)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "shadowIntensity", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EnvironmentLight &>(n)
                              .X3DLightNode::getShadowIntensity());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EnvironmentLight &>(n)
              .X3DLightNode::setShadowIntensityUnchecked(
                  std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const EnvironmentLight &>(n)
                       .X3DLightNode::getShadowIntensity(),
                  &typeid(SFFloat)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "shadows", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EnvironmentLight &>(n)
                              .X3DLightNode::getShadows());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EnvironmentLight &>(n).X3DLightNode::setShadows(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const EnvironmentLight &>(n)
                       .X3DLightNode::getShadows(),
                  &typeid(SFBool)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "specularTexture", X3DFieldType::SFNode, AccessType::InputOutput,
        "specularTexture",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EnvironmentLight &>(n).getSpecularTexture());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EnvironmentLight &>(n).setSpecularTexture(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {
              &dynamic_cast<const EnvironmentLight &>(n).getSpecularTexture(),
              &typeid(SFNode)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EnvironmentLight &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EnvironmentLight &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const EnvironmentLight &>(n).X3DNode::getDEF(),
                  &typeid(SFString)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EnvironmentLight &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EnvironmentLight &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const EnvironmentLight &>(n).X3DNode::getUSE(),
                  &typeid(SFString)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EnvironmentLight &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EnvironmentLight &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {
              &dynamic_cast<const EnvironmentLight &>(n).X3DNode::getClass_(),
              &typeid(SFString)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EnvironmentLight &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EnvironmentLight &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const EnvironmentLight &>(n).X3DNode::getId(),
                  &typeid(SFString)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const EnvironmentLight &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EnvironmentLight &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {
              &dynamic_cast<const EnvironmentLight &>(n).X3DNode::getStyle(),
              &typeid(SFString)};
        },

        X3DVersion{4, 1}});

    return t;
  }();
  return table;
}

void EnvironmentLight::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void EnvironmentLight::validateRanges(std::vector<RangeDiagnostic> &out) const {

  X3DLightNode::checkRangesAmbientIntensity(X3DLightNode::getAmbientIntensity(),
                                            nodeTypeName(), "", out);

  X3DLightNode::checkRangesColor(X3DLightNode::getColor(), nodeTypeName(), "",
                                 out);

  X3DLightNode::checkRangesIntensity(X3DLightNode::getIntensity(),
                                     nodeTypeName(), "", out);

  X3DLightNode::checkRangesShadowIntensity(X3DLightNode::getShadowIntensity(),
                                           nodeTypeName(), "", out);
}

namespace factory_detail {
std::shared_ptr<X3DNode> createEnvironmentLight() {
  return std::make_shared<EnvironmentLight>();
}
} // namespace factory_detail

} // namespace x3d::nodes
