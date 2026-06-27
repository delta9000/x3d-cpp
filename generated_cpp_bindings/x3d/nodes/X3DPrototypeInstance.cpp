// X3DPrototypeInstance.cpp
#include "x3d/nodes/X3DPrototypeInstance.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string X3DPrototypeInstance::nodeTypeName() const {
  return "X3DPrototypeInstance";
}

std::string X3DPrototypeInstance::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &X3DPrototypeInstance::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(
        FieldInfo{"IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const X3DPrototypeInstance &>(n).getIS());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<X3DPrototypeInstance &>(n).setIS(
                        std::any_cast<SFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DPrototypeInstance &>(n).getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DPrototypeInstance &>(n).setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void X3DPrototypeInstance::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

} // namespace x3d::nodes
