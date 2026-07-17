// FloatVertexAttribute.cpp
#include "x3d/nodes/FloatVertexAttribute.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string FloatVertexAttribute::nodeTypeName() const {
  return "FloatVertexAttribute";
}

std::string FloatVertexAttribute::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &FloatVertexAttribute::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const FloatVertexAttribute &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<FloatVertexAttribute &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const FloatVertexAttribute &>(n)
                              .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<FloatVertexAttribute &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "name", X3DFieldType::SFString, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const FloatVertexAttribute &>(n)
                              .X3DVertexAttributeNode::getName());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<FloatVertexAttribute &>(n)
              .X3DVertexAttributeNode::setNameUnchecked(
                  std::any_cast<xs_nmtoken>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "numComponents", X3DFieldType::SFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const FloatVertexAttribute &>(n).getNumComponents());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<FloatVertexAttribute &>(n).setNumComponentsUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "value", X3DFieldType::MFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const FloatVertexAttribute &>(n).getValue());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<FloatVertexAttribute &>(n).setValue(
              std::any_cast<MFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const FloatVertexAttribute &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<FloatVertexAttribute &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const FloatVertexAttribute &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<FloatVertexAttribute &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const FloatVertexAttribute &>(n)
                              .X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<FloatVertexAttribute &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const FloatVertexAttribute &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<FloatVertexAttribute &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const FloatVertexAttribute &>(n)
                              .X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<FloatVertexAttribute &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void FloatVertexAttribute::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void FloatVertexAttribute::validateRanges(
    std::vector<RangeDiagnostic> &out) const {

  checkRangesNumComponents(getNumComponents(), nodeTypeName(), "", out);
}

void FloatVertexAttribute::checkRangesNumComponents(
    const SFInt32 &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "numComponents",
                                  "numComponents below minimum of 1"});
  if (value > 4)
    out.push_back(RangeDiagnostic{nodeType, defName, "numComponents",
                                  "numComponents above maximum of 4"});
}

namespace factory_detail {
std::shared_ptr<X3DNode> createFloatVertexAttribute() {
  return std::make_shared<FloatVertexAttribute>();
}
} // namespace factory_detail

} // namespace x3d::nodes
