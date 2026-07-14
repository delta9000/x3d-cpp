// Script.cpp
#include "x3d/nodes/Script.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string Script::nodeTypeName() const { return "Script"; }

std::string Script::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &Script::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "autoRefresh", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Script &>(n).X3DScriptNode::getAutoRefresh());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Script &>(n).X3DScriptNode::setAutoRefreshUnchecked(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "autoRefreshTimeLimit", X3DFieldType::SFTime, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Script &>(n)
                              .X3DScriptNode::getAutoRefreshTimeLimit());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Script &>(n)
              .X3DScriptNode::setAutoRefreshTimeLimitUnchecked(
                  std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Script &>(n).X3DScriptNode::getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Script &>(n).X3DScriptNode::setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "directOutput", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Script &>(n).getDirectOutput());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Script &>(n).setDirectOutputUnchecked(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "field", X3DFieldType::MFNode, AccessType::InputOutput, "field",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Script &>(n).getField());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Script &>(n).setField(std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const Script &>(n).X3DScriptNode::getIS());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<Script &>(n).X3DScriptNode::setIS(
                        std::any_cast<SFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "load", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Script &>(n).X3DScriptNode::getLoad());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Script &>(n).X3DScriptNode::setLoad(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Script &>(n).X3DScriptNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Script &>(n).X3DScriptNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "mustEvaluate", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Script &>(n).getMustEvaluate());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Script &>(n).setMustEvaluateUnchecked(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "sourceCode", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Script &>(n).getSourceCode());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Script &>(n).setSourceCode(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "url", X3DFieldType::MFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Script &>(n).X3DScriptNode::getUrl());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Script &>(n).X3DScriptNode::setUrl(
              std::any_cast<MFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Script &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Script &>(n).X3DNode::setDEF(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Script &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Script &>(n).X3DNode::setUSE(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Script &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Script &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Script &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Script &>(n).X3DNode::setId(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Script &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Script &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void Script::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void Script::validateRanges(std::vector<RangeDiagnostic> &out) const {

  X3DScriptNode::checkRangesAutoRefresh(X3DScriptNode::getAutoRefresh(),
                                        nodeTypeName(), "", out);

  X3DScriptNode::checkRangesAutoRefreshTimeLimit(
      X3DScriptNode::getAutoRefreshTimeLimit(), nodeTypeName(), "", out);
}

namespace factory_detail {
std::shared_ptr<X3DNode> createScript() { return std::make_shared<Script>(); }
} // namespace factory_detail

} // namespace x3d::nodes
