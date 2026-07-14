// ComposedCubeMapTexture.cpp
#include "x3d/nodes/ComposedCubeMapTexture.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string ComposedCubeMapTexture::nodeTypeName() const {
  return "ComposedCubeMapTexture";
}

std::string ComposedCubeMapTexture::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &ComposedCubeMapTexture::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "backTexture", X3DFieldType::SFNode, AccessType::InputOutput,
        "backTexture",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ComposedCubeMapTexture &>(n).getBackTexture());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ComposedCubeMapTexture &>(n).setBackTexture(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bottomTexture", X3DFieldType::SFNode, AccessType::InputOutput,
        "bottomTexture",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ComposedCubeMapTexture &>(n)
                              .getBottomTexture());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ComposedCubeMapTexture &>(n).setBottomTexture(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ComposedCubeMapTexture &>(n)
                              .X3DTextureNode::getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ComposedCubeMapTexture &>(n)
              .X3DTextureNode::setDescription(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "frontTexture", X3DFieldType::SFNode, AccessType::InputOutput,
        "frontTexture",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ComposedCubeMapTexture &>(n)
                              .getFrontTexture());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ComposedCubeMapTexture &>(n).setFrontTexture(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ComposedCubeMapTexture &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ComposedCubeMapTexture &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "leftTexture", X3DFieldType::SFNode, AccessType::InputOutput,
        "leftTexture",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ComposedCubeMapTexture &>(n).getLeftTexture());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ComposedCubeMapTexture &>(n).setLeftTexture(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ComposedCubeMapTexture &>(n)
                              .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ComposedCubeMapTexture &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "rightTexture", X3DFieldType::SFNode, AccessType::InputOutput,
        "rightTexture",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ComposedCubeMapTexture &>(n)
                              .getRightTexture());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ComposedCubeMapTexture &>(n).setRightTexture(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "textureProperties", X3DFieldType::SFNode, AccessType::InitializeOnly,
        "textureProperties",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ComposedCubeMapTexture &>(n)
                              .getTextureProperties());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ComposedCubeMapTexture &>(n)
              .setTexturePropertiesUnchecked(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "topTexture", X3DFieldType::SFNode, AccessType::InputOutput,
        "topTexture",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ComposedCubeMapTexture &>(n).getTopTexture());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ComposedCubeMapTexture &>(n).setTopTexture(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ComposedCubeMapTexture &>(n)
                              .X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ComposedCubeMapTexture &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ComposedCubeMapTexture &>(n)
                              .X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ComposedCubeMapTexture &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ComposedCubeMapTexture &>(n)
                              .X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ComposedCubeMapTexture &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ComposedCubeMapTexture &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ComposedCubeMapTexture &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ComposedCubeMapTexture &>(n)
                              .X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ComposedCubeMapTexture &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void ComposedCubeMapTexture::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

namespace factory_detail {
std::shared_ptr<X3DNode> createComposedCubeMapTexture() {
  return std::make_shared<ComposedCubeMapTexture>();
}
} // namespace factory_detail

} // namespace x3d::nodes
