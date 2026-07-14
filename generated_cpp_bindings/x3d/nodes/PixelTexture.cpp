// PixelTexture.cpp
#include "x3d/nodes/PixelTexture.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string PixelTexture::nodeTypeName() const { return "PixelTexture"; }

std::string PixelTexture::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &PixelTexture::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PixelTexture &>(n)
                              .X3DTextureNode::getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PixelTexture &>(n).X3DTextureNode::setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "image", X3DFieldType::SFImage, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PixelTexture &>(n).getImage());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PixelTexture &>(n).setImage(std::any_cast<SFImage>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const PixelTexture &>(n).X3DNode::getIS());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<PixelTexture &>(n).X3DNode::setIS(
                        std::any_cast<SFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PixelTexture &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PixelTexture &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "repeatS", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PixelTexture &>(n)
                              .X3DTexture2DNode::getRepeatS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PixelTexture &>(n).X3DTexture2DNode::setRepeatSUnchecked(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "repeatT", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PixelTexture &>(n)
                              .X3DTexture2DNode::getRepeatT());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PixelTexture &>(n).X3DTexture2DNode::setRepeatTUnchecked(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "textureProperties", X3DFieldType::SFNode, AccessType::InitializeOnly,
        "textureProperties",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PixelTexture &>(n)
                              .X3DTexture2DNode::getTextureProperties());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PixelTexture &>(n)
              .X3DTexture2DNode::setTexturePropertiesUnchecked(
                  std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PixelTexture &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PixelTexture &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PixelTexture &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PixelTexture &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PixelTexture &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PixelTexture &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"id", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const PixelTexture &>(n).X3DNode::getId());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<PixelTexture &>(n).X3DNode::setId(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PixelTexture &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PixelTexture &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void PixelTexture::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

namespace factory_detail {
std::shared_ptr<X3DNode> createPixelTexture() {
  return std::make_shared<PixelTexture>();
}
} // namespace factory_detail

} // namespace x3d::nodes
