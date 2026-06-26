// ShadedVolumeStyle.cpp
#include "ShadedVolumeStyle.hpp"

std::string ShadedVolumeStyle::nodeTypeName() const {
  return "ShadedVolumeStyle";
}

std::string ShadedVolumeStyle::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &ShadedVolumeStyle::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ShadedVolumeStyle &>(n)
                              .X3DVolumeRenderStyleNode::getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ShadedVolumeStyle &>(n)
              .X3DVolumeRenderStyleNode::setEnabled(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ShadedVolumeStyle &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ShadedVolumeStyle &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "lighting", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ShadedVolumeStyle &>(n).getLighting());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ShadedVolumeStyle &>(n).setLighting(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "material", X3DFieldType::SFNode, AccessType::InputOutput, "material",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ShadedVolumeStyle &>(n).getMaterial());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ShadedVolumeStyle &>(n).setMaterial(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ShadedVolumeStyle &>(n)
                              .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ShadedVolumeStyle &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "phaseFunction", X3DFieldType::SFEnum, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ShadedVolumeStyle &>(n).getPhaseFunction());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ShadedVolumeStyle &>(n).setPhaseFunctionUnchecked(
              std::any_cast<PhaseFunctionValues>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(
              dynamic_cast<const ShadedVolumeStyle &>(n).getPhaseFunction());
        },

        [](X3DNode &n, const std::string &s) {
          PhaseFunctionValues ev;
          if (from_string(s, ev))
            dynamic_cast<ShadedVolumeStyle &>(n).setPhaseFunctionUnchecked(ev);
        }

    });

    t.push_back(FieldInfo{
        "shadows", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ShadedVolumeStyle &>(n).getShadows());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ShadedVolumeStyle &>(n).setShadows(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "surfaceNormals", X3DFieldType::SFNode, AccessType::InputOutput,
        "surfaceNormals",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ShadedVolumeStyle &>(n).getSurfaceNormals());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ShadedVolumeStyle &>(n).setSurfaceNormals(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ShadedVolumeStyle &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ShadedVolumeStyle &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ShadedVolumeStyle &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ShadedVolumeStyle &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ShadedVolumeStyle &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ShadedVolumeStyle &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ShadedVolumeStyle &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ShadedVolumeStyle &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ShadedVolumeStyle &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ShadedVolumeStyle &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void ShadedVolumeStyle::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}
