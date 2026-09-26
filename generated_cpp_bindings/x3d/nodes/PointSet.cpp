// PointSet.cpp
#include "x3d/nodes/PointSet.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string PointSet::nodeTypeName() const { return "PointSet"; }

std::string PointSet::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &PointSet::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "attrib", X3DFieldType::MFNode, AccessType::InputOutput, "attrib",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PointSet &>(n).getAttrib());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointSet &>(n).setAttrib(std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const PointSet &>(n).getAttrib(),
                  &typeid(MFNode)};
        }

    });

    t.push_back(FieldInfo{
        "color", X3DFieldType::SFNode, AccessType::InputOutput, "color",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PointSet &>(n).getColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointSet &>(n).setColor(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const PointSet &>(n).getColor(),
                  &typeid(SFNode)};
        }

    });

    t.push_back(FieldInfo{
        "coord", X3DFieldType::SFNode, AccessType::InputOutput, "coord",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PointSet &>(n).getCoord());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointSet &>(n).setCoord(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const PointSet &>(n).getCoord(),
                  &typeid(SFNode)};
        }

    });

    t.push_back(FieldInfo{
        "fogCoord", X3DFieldType::SFNode, AccessType::InputOutput, "fogCoord",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PointSet &>(n).getFogCoord());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointSet &>(n).setFogCoord(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const PointSet &>(n).getFogCoord(),
                  &typeid(SFNode)};
        }

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PointSet &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointSet &>(n).X3DNode::setIS(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const PointSet &>(n).X3DNode::getIS(),
                  &typeid(SFNode)};
        }

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PointSet &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointSet &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const PointSet &>(n).X3DNode::getMetadata(),
                  &typeid(SFNode)};
        }

    });

    t.push_back(FieldInfo{
        "normal", X3DFieldType::SFNode, AccessType::InputOutput, "normal",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PointSet &>(n).getNormal());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointSet &>(n).setNormal(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const PointSet &>(n).getNormal(),
                  &typeid(SFNode)};
        }

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PointSet &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointSet &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const PointSet &>(n).X3DNode::getDEF(),
                  &typeid(SFString)};
        }

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PointSet &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointSet &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const PointSet &>(n).X3DNode::getUSE(),
                  &typeid(SFString)};
        }

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PointSet &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointSet &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const PointSet &>(n).X3DNode::getClass_(),
                  &typeid(SFString)};
        }

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PointSet &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointSet &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const PointSet &>(n).X3DNode::getId(),
                  &typeid(SFString)};
        }

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PointSet &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PointSet &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const PointSet &>(n).X3DNode::getStyle(),
                  &typeid(SFString)};
        }

    });

    return t;
  }();
  return table;
}

void PointSet::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

namespace factory_detail {
std::shared_ptr<X3DNode> createPointSet() {
  return std::make_shared<PointSet>();
}
} // namespace factory_detail

} // namespace x3d::nodes
