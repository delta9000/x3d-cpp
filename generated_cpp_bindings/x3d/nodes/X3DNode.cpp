// X3DNode.cpp
#include "x3d/nodes/X3DNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string X3DNode::nodeTypeName() const { return "X3DNode"; }

std::string X3DNode::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &X3DNode::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(
        FieldInfo{"IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const X3DNode &>(n).getIS());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<X3DNode &>(n).setIS(std::any_cast<SFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DNode &>(n).getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DNode &>(n).setMetadata(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DNode &>(n).getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DNode &>(n).setDEF(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DNode &>(n).getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DNode &>(n).setUSE(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DNode &>(n).getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DNode &>(n).setClass_(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DNode &>(n).getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DNode &>(n).setId(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DNode &>(n).getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DNode &>(n).setStyle(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void X3DNode::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void X3DNode::validateRanges(std::vector<RangeDiagnostic> & /*out*/) const {}

} // namespace x3d::nodes
