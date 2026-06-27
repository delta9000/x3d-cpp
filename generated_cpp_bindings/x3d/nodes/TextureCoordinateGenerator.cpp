// TextureCoordinateGenerator.cpp
#include "x3d/nodes/TextureCoordinateGenerator.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string TextureCoordinateGenerator::nodeTypeName() const {
  return "TextureCoordinateGenerator";
}

std::string TextureCoordinateGenerator::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &TextureCoordinateGenerator::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TextureCoordinateGenerator &>(n)
                              .X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureCoordinateGenerator &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "mapping", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TextureCoordinateGenerator &>(n)
                              .X3DSingleTextureCoordinateNode::getMapping());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureCoordinateGenerator &>(n)
              .X3DSingleTextureCoordinateNode::setMapping(
                  std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TextureCoordinateGenerator &>(n)
                              .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureCoordinateGenerator &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "mode", X3DFieldType::SFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TextureCoordinateGenerator &>(n).getMode());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureCoordinateGenerator &>(n).setMode(
              std::any_cast<TextureCoordinateGeneratorModeChoices>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(
              dynamic_cast<const TextureCoordinateGenerator &>(n).getMode());
        },

        [](X3DNode &n, const std::string &s) {
          TextureCoordinateGeneratorModeChoices ev;
          if (from_string(s, ev))
            dynamic_cast<TextureCoordinateGenerator &>(n).setMode(ev);
        }

    });

    t.push_back(FieldInfo{
        "parameter", X3DFieldType::MFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TextureCoordinateGenerator &>(n)
                              .getParameter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureCoordinateGenerator &>(n).setParameter(
              std::any_cast<MFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TextureCoordinateGenerator &>(n)
                              .X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureCoordinateGenerator &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TextureCoordinateGenerator &>(n)
                              .X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureCoordinateGenerator &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TextureCoordinateGenerator &>(n)
                              .X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureCoordinateGenerator &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TextureCoordinateGenerator &>(n)
                              .X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureCoordinateGenerator &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TextureCoordinateGenerator &>(n)
                              .X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TextureCoordinateGenerator &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void TextureCoordinateGenerator::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

} // namespace x3d::nodes
