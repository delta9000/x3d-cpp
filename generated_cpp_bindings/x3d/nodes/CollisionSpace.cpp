// CollisionSpace.cpp
#include "x3d/nodes/CollisionSpace.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string CollisionSpace::nodeTypeName() const { return "CollisionSpace"; }

std::string CollisionSpace::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &CollisionSpace::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "bboxCenter", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const CollisionSpace &>(n)
                              .X3DNBodyCollisionSpaceNode::getBboxCenter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionSpace &>(n)
              .X3DNBodyCollisionSpaceNode::setBboxCenterUnchecked(
                  std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxDisplay", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const CollisionSpace &>(n)
                              .X3DNBodyCollisionSpaceNode::getBboxDisplay());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionSpace &>(n)
              .X3DNBodyCollisionSpaceNode::setBboxDisplay(
                  std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxSize", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const CollisionSpace &>(n)
                              .X3DNBodyCollisionSpaceNode::getBboxSize());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionSpace &>(n)
              .X3DNBodyCollisionSpaceNode::setBboxSizeUnchecked(
                  std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "collidables", X3DFieldType::MFNode, AccessType::InputOutput,
        "collidables",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CollisionSpace &>(n).getCollidables());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionSpace &>(n).setCollidables(
              std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const CollisionSpace &>(n)
                              .X3DNBodyCollisionSpaceNode::getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionSpace &>(n)
              .X3DNBodyCollisionSpaceNode::setEnabled(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CollisionSpace &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionSpace &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CollisionSpace &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionSpace &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "useGeometry", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CollisionSpace &>(n).getUseGeometry());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionSpace &>(n).setUseGeometry(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "visible", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const CollisionSpace &>(n)
                              .X3DNBodyCollisionSpaceNode::getVisible());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionSpace &>(n)
              .X3DNBodyCollisionSpaceNode::setVisible(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CollisionSpace &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionSpace &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CollisionSpace &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionSpace &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CollisionSpace &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionSpace &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CollisionSpace &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionSpace &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CollisionSpace &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CollisionSpace &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void CollisionSpace::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

} // namespace x3d::nodes
