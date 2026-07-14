// CollidableOffset.cpp
#include "x3d/nodes/CollidableOffset.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string CollidableOffset::nodeTypeName() const {
  return "CollidableOffset";
}

std::string CollidableOffset::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &CollidableOffset::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "bboxCenter", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const CollidableOffset &>(n)
                              .X3DNBodyCollidableNode::getBboxCenter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollidableOffset &>(n)
              .X3DNBodyCollidableNode::setBboxCenterUnchecked(
                  std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxDisplay", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const CollidableOffset &>(n)
                              .X3DNBodyCollidableNode::getBboxDisplay());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollidableOffset &>(n)
              .X3DNBodyCollidableNode::setBboxDisplay(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxSize", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const CollidableOffset &>(n)
                              .X3DNBodyCollidableNode::getBboxSize());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollidableOffset &>(n)
              .X3DNBodyCollidableNode::setBboxSizeUnchecked(
                  std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "collidable", X3DFieldType::SFNode, AccessType::InitializeOnly,
        "collidable",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CollidableOffset &>(n).getCollidable());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollidableOffset &>(n).setCollidableUnchecked(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const CollidableOffset &>(n)
                              .X3DNBodyCollidableNode::getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollidableOffset &>(n)
              .X3DNBodyCollidableNode::setEnabled(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CollidableOffset &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollidableOffset &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CollidableOffset &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollidableOffset &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "rotation", X3DFieldType::SFRotation, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const CollidableOffset &>(n)
                              .X3DNBodyCollidableNode::getRotation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollidableOffset &>(n)
              .X3DNBodyCollidableNode::setRotation(
                  std::any_cast<SFRotation>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "translation", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const CollidableOffset &>(n)
                              .X3DNBodyCollidableNode::getTranslation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollidableOffset &>(n)
              .X3DNBodyCollidableNode::setTranslation(
                  std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "visible", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const CollidableOffset &>(n)
                              .X3DNBodyCollidableNode::getVisible());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollidableOffset &>(n)
              .X3DNBodyCollidableNode::setVisible(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CollidableOffset &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollidableOffset &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CollidableOffset &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollidableOffset &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CollidableOffset &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollidableOffset &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CollidableOffset &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollidableOffset &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CollidableOffset &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollidableOffset &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void CollidableOffset::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

namespace factory_detail {
std::shared_ptr<X3DNode> createCollidableOffset() {
  return std::make_shared<CollidableOffset>();
}
} // namespace factory_detail

} // namespace x3d::nodes
