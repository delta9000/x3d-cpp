// ProgramShader.cpp
#include "x3d/nodes/ProgramShader.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string ProgramShader::nodeTypeName() const { return "ProgramShader"; }

std::string ProgramShader::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &ProgramShader::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(
        FieldInfo{"activate", X3DFieldType::SFBool, AccessType::InputOnly, "",

                  nullptr,

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<ProgramShader &>(n).X3DShaderNode::onActivate(
                        std::any_cast<SFBool>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ProgramShader &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ProgramShader &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isSelected", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ProgramShader &>(n)
                              .X3DShaderNode::getIsSelected());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ProgramShader &>(n).X3DShaderNode::emitIsSelected(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"isValid", X3DFieldType::SFBool, AccessType::OutputOnly, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const ProgramShader &>(n)
                                        .X3DShaderNode::getIsValid());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<ProgramShader &>(n).X3DShaderNode::emitIsValid(
                        std::any_cast<SFBool>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "language", X3DFieldType::SFString, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ProgramShader &>(n)
                              .X3DShaderNode::getLanguage());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ProgramShader &>(n).X3DShaderNode::setLanguageUnchecked(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ProgramShader &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ProgramShader &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "programs", X3DFieldType::MFNode, AccessType::InputOutput, "programs",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ProgramShader &>(n).getPrograms());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ProgramShader &>(n).setPrograms(
              std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ProgramShader &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ProgramShader &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ProgramShader &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ProgramShader &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ProgramShader &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ProgramShader &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ProgramShader &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ProgramShader &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ProgramShader &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ProgramShader &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void ProgramShader::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

namespace factory_detail {
std::shared_ptr<X3DNode> createProgramShader() {
  return std::make_shared<ProgramShader>();
}
} // namespace factory_detail

} // namespace x3d::nodes
