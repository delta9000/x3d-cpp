// Shape.cpp
#include "x3d/nodes/Shape.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string Shape::nodeTypeName() const { return "Shape"; }

std::string Shape::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &Shape::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "appearance", X3DFieldType::SFNode, AccessType::InputOutput,
        "appearance",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Shape &>(n).X3DShapeNode::getAppearance());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Shape &>(n).X3DShapeNode::setAppearance(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxCenter", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Shape &>(n).X3DShapeNode::getBboxCenter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Shape &>(n).X3DShapeNode::setBboxCenterUnchecked(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxDisplay", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Shape &>(n).X3DShapeNode::getBboxDisplay());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Shape &>(n).X3DShapeNode::setBboxDisplay(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxSize", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Shape &>(n).X3DShapeNode::getBboxSize());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Shape &>(n).X3DShapeNode::setBboxSizeUnchecked(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "castShadow", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Shape &>(n).X3DShapeNode::getCastShadow());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Shape &>(n).X3DShapeNode::setCastShadow(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "geometry", X3DFieldType::SFNode, AccessType::InputOutput, "geometry",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Shape &>(n).X3DShapeNode::getGeometry());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Shape &>(n).X3DShapeNode::setGeometry(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Shape &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Shape &>(n).X3DNode::setIS(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Shape &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Shape &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "visible", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Shape &>(n).X3DShapeNode::getVisible());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Shape &>(n).X3DShapeNode::setVisible(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Shape &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Shape &>(n).X3DNode::setDEF(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Shape &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Shape &>(n).X3DNode::setUSE(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Shape &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Shape &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Shape &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Shape &>(n).X3DNode::setId(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Shape &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Shape &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void Shape::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

namespace factory_detail {
std::shared_ptr<X3DNode> createShape() { return std::make_shared<Shape>(); }
} // namespace factory_detail

} // namespace x3d::nodes
