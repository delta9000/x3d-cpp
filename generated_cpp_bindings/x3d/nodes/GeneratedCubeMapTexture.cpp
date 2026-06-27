// GeneratedCubeMapTexture.cpp
#include "x3d/nodes/GeneratedCubeMapTexture.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string GeneratedCubeMapTexture::nodeTypeName() const {
  return "GeneratedCubeMapTexture";
}

std::string GeneratedCubeMapTexture::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &GeneratedCubeMapTexture::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeneratedCubeMapTexture &>(n)
                              .X3DTextureNode::getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeneratedCubeMapTexture &>(n)
              .X3DTextureNode::setDescription(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeneratedCubeMapTexture &>(n)
                              .X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeneratedCubeMapTexture &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeneratedCubeMapTexture &>(n)
                              .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeneratedCubeMapTexture &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "size", X3DFieldType::SFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const GeneratedCubeMapTexture &>(n).getSize());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeneratedCubeMapTexture &>(n).setSizeUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "textureProperties", X3DFieldType::SFNode, AccessType::InitializeOnly,
        "textureProperties",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeneratedCubeMapTexture &>(n)
                              .getTextureProperties());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeneratedCubeMapTexture &>(n)
              .setTexturePropertiesUnchecked(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "update", X3DFieldType::SFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const GeneratedCubeMapTexture &>(n).getUpdate());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeneratedCubeMapTexture &>(n).setUpdate(
              std::any_cast<GeneratedCubeMapTextureUpdateChoices>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(
              dynamic_cast<const GeneratedCubeMapTexture &>(n).getUpdate());
        },

        [](X3DNode &n, const std::string &s) {
          GeneratedCubeMapTextureUpdateChoices ev;
          if (from_string(s, ev))
            dynamic_cast<GeneratedCubeMapTexture &>(n).setUpdate(ev);
        }

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeneratedCubeMapTexture &>(n)
                              .X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeneratedCubeMapTexture &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeneratedCubeMapTexture &>(n)
                              .X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeneratedCubeMapTexture &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeneratedCubeMapTexture &>(n)
                              .X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeneratedCubeMapTexture &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeneratedCubeMapTexture &>(n)
                              .X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeneratedCubeMapTexture &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeneratedCubeMapTexture &>(n)
                              .X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeneratedCubeMapTexture &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void GeneratedCubeMapTexture::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

} // namespace x3d::nodes
