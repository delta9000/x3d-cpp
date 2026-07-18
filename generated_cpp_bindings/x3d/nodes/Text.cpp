// Text.cpp
#include "x3d/nodes/Text.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string Text::nodeTypeName() const { return "Text"; }

std::string Text::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &Text::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "fontStyle", X3DFieldType::SFNode, AccessType::InputOutput, "fontStyle",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Text &>(n).getFontStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Text &>(n).setFontStyle(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Text &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Text &>(n).X3DNode::setIS(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "length", X3DFieldType::MFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Text &>(n).getLength());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Text &>(n).setLengthUnchecked(std::any_cast<MFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "lineBounds", X3DFieldType::MFVec2f, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Text &>(n).getLineBounds());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Text &>(n).emitLineBounds(std::any_cast<MFVec2f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "maxExtent", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Text &>(n).getMaxExtent());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Text &>(n).setMaxExtentUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Text &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Text &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "origin", X3DFieldType::SFVec3f, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Text &>(n).getOrigin());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Text &>(n).emitOrigin(std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "solid", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Text &>(n).getSolid());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Text &>(n).setSolidUnchecked(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "string", X3DFieldType::MFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Text &>(n).getString());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Text &>(n).setString(std::any_cast<MFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "textBounds", X3DFieldType::SFVec2f, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Text &>(n).getTextBounds());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Text &>(n).emitTextBounds(std::any_cast<SFVec2f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Text &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Text &>(n).X3DNode::setDEF(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Text &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Text &>(n).X3DNode::setUSE(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Text &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Text &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Text &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Text &>(n).X3DNode::setId(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Text &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Text &>(n).X3DNode::setStyle(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void Text::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void Text::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesLength(getLength(), nodeTypeName(), "", out);

  checkRangesMaxExtent(getMaxExtent(), nodeTypeName(), "", out);
}

void Text::checkRangesLength(const MFFloat &value, const std::string &nodeType,
                             const std::string &defName,
                             std::vector<RangeDiagnostic> &out) {
  for (const auto &v : value) {

    if (v < 0.0f)
      out.push_back(RangeDiagnostic{nodeType, defName, "length",
                                    "length below minimum of 0"});
  }
}

void Text::checkRangesMaxExtent(const SFFloat &value,
                                const std::string &nodeType,
                                const std::string &defName,
                                std::vector<RangeDiagnostic> &out) {
  if (value < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "maxExtent",
                                  "maxExtent below minimum of 0"});
}

namespace factory_detail {
std::shared_ptr<X3DNode> createText() { return std::make_shared<Text>(); }
} // namespace factory_detail

} // namespace x3d::nodes
