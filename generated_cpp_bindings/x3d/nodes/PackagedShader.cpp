// PackagedShader.cpp
#include "x3d/nodes/PackagedShader.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string PackagedShader::nodeTypeName() const { return "PackagedShader"; }

std::string PackagedShader::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &PackagedShader::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(
        FieldInfo{"activate", X3DFieldType::SFBool, AccessType::InputOnly, "",

                  nullptr,

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<PackagedShader &>(n).X3DShaderNode::onActivate(
                        std::any_cast<SFBool>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "autoRefresh", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PackagedShader &>(n)
                              .X3DUrlObject::getAutoRefresh());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PackagedShader &>(n)
              .X3DUrlObject::setAutoRefreshUnchecked(std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{"autoRefreshTimeLimit", X3DFieldType::SFTime,
                          AccessType::InputOutput, "",

                          [](const X3DNode &n) -> std::any {
                            return std::any(
                                dynamic_cast<const PackagedShader &>(n)
                                    .X3DUrlObject::getAutoRefreshTimeLimit());
                          },

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<PackagedShader &>(n)
                                .X3DUrlObject::setAutoRefreshTimeLimitUnchecked(
                                    std::any_cast<SFTime>(v));
                          },

                          nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PackagedShader &>(n)
                              .X3DUrlObject::getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PackagedShader &>(n).X3DUrlObject::setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "field", X3DFieldType::MFNode, AccessType::InputOutput, "field",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PackagedShader &>(n).getField());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PackagedShader &>(n).setField(std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PackagedShader &>(n).getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PackagedShader &>(n).setIS(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isSelected", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PackagedShader &>(n)
                              .X3DShaderNode::getIsSelected());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PackagedShader &>(n).X3DShaderNode::emitIsSelected(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isValid", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PackagedShader &>(n)
                              .X3DShaderNode::getIsValid());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PackagedShader &>(n).X3DShaderNode::emitIsValid(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "language", X3DFieldType::SFString, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PackagedShader &>(n)
                              .X3DShaderNode::getLanguage());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PackagedShader &>(n).X3DShaderNode::setLanguageUnchecked(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "load", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PackagedShader &>(n).X3DUrlObject::getLoad());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PackagedShader &>(n).X3DUrlObject::setLoad(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PackagedShader &>(n).getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PackagedShader &>(n).setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "url", X3DFieldType::MFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PackagedShader &>(n).X3DUrlObject::getUrl());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PackagedShader &>(n).X3DUrlObject::setUrl(
              std::any_cast<MFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PackagedShader &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PackagedShader &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PackagedShader &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PackagedShader &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PackagedShader &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PackagedShader &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PackagedShader &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PackagedShader &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PackagedShader &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PackagedShader &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void PackagedShader::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void PackagedShader::validateRanges(std::vector<RangeDiagnostic> &out) const {

  X3DUrlObject::checkRangesAutoRefresh(X3DUrlObject::getAutoRefresh(),
                                       nodeTypeName(), "", out);

  X3DUrlObject::checkRangesAutoRefreshTimeLimit(
      X3DUrlObject::getAutoRefreshTimeLimit(), nodeTypeName(), "", out);
}

} // namespace x3d::nodes
