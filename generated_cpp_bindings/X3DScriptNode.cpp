// X3DScriptNode.cpp
#include "X3DScriptNode.hpp"

std::string X3DScriptNode::nodeTypeName() const { return "X3DScriptNode"; }

std::string X3DScriptNode::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &X3DScriptNode::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "autoRefresh", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DScriptNode &>(n).getAutoRefresh());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DScriptNode &>(n).setAutoRefreshUnchecked(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "autoRefreshTimeLimit", X3DFieldType::SFTime, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DScriptNode &>(n).getAutoRefreshTimeLimit());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DScriptNode &>(n).setAutoRefreshTimeLimitUnchecked(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DScriptNode &>(n).getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DScriptNode &>(n).setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DScriptNode &>(n).getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DScriptNode &>(n).setIS(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "load", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DScriptNode &>(n).getLoad());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DScriptNode &>(n).setLoad(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DScriptNode &>(n).getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DScriptNode &>(n).setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "url", X3DFieldType::MFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DScriptNode &>(n).getUrl());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DScriptNode &>(n).setUrl(std::any_cast<MFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DScriptNode &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DScriptNode &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DScriptNode &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DScriptNode &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DScriptNode &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DScriptNode &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DScriptNode &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DScriptNode &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DScriptNode &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DScriptNode &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void X3DScriptNode::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void X3DScriptNode::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesAutoRefresh(getAutoRefresh(), nodeTypeName(), "", out);

  checkRangesAutoRefreshTimeLimit(getAutoRefreshTimeLimit(), nodeTypeName(), "",
                                  out);
}

void X3DScriptNode::checkRangesAutoRefresh(const SFTime &value,
                                           const std::string &nodeType,
                                           const std::string &defName,
                                           std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "autoRefresh",
                                  "autoRefresh below minimum of 0"});
}

void X3DScriptNode::checkRangesAutoRefreshTimeLimit(
    const SFTime &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "autoRefreshTimeLimit",
                                  "autoRefreshTimeLimit below minimum of 0"});
}
