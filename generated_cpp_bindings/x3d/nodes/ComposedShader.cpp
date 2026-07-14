// ComposedShader.cpp
#include "x3d/nodes/ComposedShader.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string ComposedShader::nodeTypeName() const { return "ComposedShader"; }

std::string ComposedShader::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &ComposedShader::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(
        FieldInfo{"activate", X3DFieldType::SFBool, AccessType::InputOnly, "",

                  nullptr,

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<ComposedShader &>(n).X3DShaderNode::onActivate(
                        std::any_cast<SFBool>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "field", X3DFieldType::MFNode, AccessType::InputOutput, "field",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ComposedShader &>(n).getField());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ComposedShader &>(n).setField(std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ComposedShader &>(n).getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ComposedShader &>(n).setIS(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isSelected", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ComposedShader &>(n)
                              .X3DShaderNode::getIsSelected());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ComposedShader &>(n).X3DShaderNode::emitIsSelected(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isValid", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ComposedShader &>(n)
                              .X3DShaderNode::getIsValid());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ComposedShader &>(n).X3DShaderNode::emitIsValid(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "language", X3DFieldType::SFString, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ComposedShader &>(n)
                              .X3DShaderNode::getLanguage());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ComposedShader &>(n).X3DShaderNode::setLanguageUnchecked(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ComposedShader &>(n).getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ComposedShader &>(n).setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "parts", X3DFieldType::MFNode, AccessType::InputOutput, "parts",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ComposedShader &>(n).getParts());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ComposedShader &>(n).setParts(std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ComposedShader &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ComposedShader &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ComposedShader &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ComposedShader &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ComposedShader &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ComposedShader &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ComposedShader &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ComposedShader &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ComposedShader &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ComposedShader &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void ComposedShader::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

namespace factory_detail {
std::shared_ptr<X3DNode> createComposedShader() {
  return std::make_shared<ComposedShader>();
}
} // namespace factory_detail

} // namespace x3d::nodes
