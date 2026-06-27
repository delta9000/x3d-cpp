// PixelTexture3D.cpp
#include "x3d/nodes/PixelTexture3D.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string PixelTexture3D::nodeTypeName() const { return "PixelTexture3D"; }

std::string PixelTexture3D::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &PixelTexture3D::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PixelTexture3D &>(n)
                              .X3DTextureNode::getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PixelTexture3D &>(n).X3DTextureNode::setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "image", X3DFieldType::MFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PixelTexture3D &>(n).getImage());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PixelTexture3D &>(n).setImage(std::any_cast<MFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PixelTexture3D &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PixelTexture3D &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PixelTexture3D &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PixelTexture3D &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "repeatR", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PixelTexture3D &>(n)
                              .X3DTexture3DNode::getRepeatR());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PixelTexture3D &>(n)
              .X3DTexture3DNode::setRepeatRUnchecked(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "repeatS", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PixelTexture3D &>(n)
                              .X3DTexture3DNode::getRepeatS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PixelTexture3D &>(n)
              .X3DTexture3DNode::setRepeatSUnchecked(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "repeatT", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PixelTexture3D &>(n)
                              .X3DTexture3DNode::getRepeatT());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PixelTexture3D &>(n)
              .X3DTexture3DNode::setRepeatTUnchecked(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "textureProperties", X3DFieldType::SFNode, AccessType::InitializeOnly,
        "textureProperties",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PixelTexture3D &>(n)
                              .X3DTexture3DNode::getTextureProperties());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PixelTexture3D &>(n)
              .X3DTexture3DNode::setTexturePropertiesUnchecked(
                  std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PixelTexture3D &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PixelTexture3D &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PixelTexture3D &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PixelTexture3D &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PixelTexture3D &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PixelTexture3D &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PixelTexture3D &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PixelTexture3D &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PixelTexture3D &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PixelTexture3D &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void PixelTexture3D::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

} // namespace x3d::nodes
