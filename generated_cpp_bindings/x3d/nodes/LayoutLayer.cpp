// LayoutLayer.cpp
#include "x3d/nodes/LayoutLayer.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string LayoutLayer::nodeTypeName() const { return "LayoutLayer"; }

std::string LayoutLayer::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &LayoutLayer::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{"addChildren", X3DFieldType::MFNode,
                          AccessType::InputOnly, "addChildren",

                          nullptr,

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<LayoutLayer &>(n).onAddChildren(
                                std::any_cast<MFNode>(v));
                          },

                          nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "children", X3DFieldType::MFNode, AccessType::InputOutput, "children",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const LayoutLayer &>(n).getChildren());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<LayoutLayer &>(n).setChildren(std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const LayoutLayer &>(n).X3DNode::getIS());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<LayoutLayer &>(n).X3DNode::setIS(
                        std::any_cast<SFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "layout", X3DFieldType::SFNode, AccessType::InputOutput, "layout",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const LayoutLayer &>(n).getLayout());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<LayoutLayer &>(n).setLayout(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const LayoutLayer &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<LayoutLayer &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "objectType", X3DFieldType::MFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const LayoutLayer &>(n)
                              .X3DLayerNode::getObjectType());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<LayoutLayer &>(n).X3DLayerNode::setObjectType(
              std::any_cast<MFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "pickable", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const LayoutLayer &>(n).X3DLayerNode::getPickable());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<LayoutLayer &>(n).X3DLayerNode::setPickable(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{"removeChildren", X3DFieldType::MFNode,
                          AccessType::InputOnly, "removeChildren",

                          nullptr,

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<LayoutLayer &>(n).onRemoveChildren(
                                std::any_cast<MFNode>(v));
                          },

                          nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "viewport", X3DFieldType::SFNode, AccessType::InputOutput, "viewport",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const LayoutLayer &>(n).X3DLayerNode::getViewport());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<LayoutLayer &>(n).X3DLayerNode::setViewport(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "visible", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const LayoutLayer &>(n).X3DLayerNode::getVisible());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<LayoutLayer &>(n).X3DLayerNode::setVisible(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const LayoutLayer &>(n).X3DNode::getDEF());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<LayoutLayer &>(n).X3DNode::setDEF(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(
        FieldInfo{"USE", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const LayoutLayer &>(n).X3DNode::getUSE());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<LayoutLayer &>(n).X3DNode::setUSE(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const LayoutLayer &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<LayoutLayer &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"id", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const LayoutLayer &>(n).X3DNode::getId());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<LayoutLayer &>(n).X3DNode::setId(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const LayoutLayer &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<LayoutLayer &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void LayoutLayer::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

namespace factory_detail {
std::shared_ptr<X3DNode> createLayoutLayer() {
  return std::make_shared<LayoutLayer>();
}
} // namespace factory_detail

} // namespace x3d::nodes
