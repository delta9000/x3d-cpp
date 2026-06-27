// IndexedTriangleSet.cpp
#include "x3d/nodes/IndexedTriangleSet.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string IndexedTriangleSet::nodeTypeName() const {
  return "IndexedTriangleSet";
}

std::string IndexedTriangleSet::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &IndexedTriangleSet::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "attrib", X3DFieldType::MFNode, AccessType::InputOutput, "attrib",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const IndexedTriangleSet &>(n)
                              .X3DComposedGeometryNode::getAttrib());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedTriangleSet &>(n)
              .X3DComposedGeometryNode::setAttrib(std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"ccw", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const IndexedTriangleSet &>(n)
                                        .X3DComposedGeometryNode::getCcw());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<IndexedTriangleSet &>(n)
                        .X3DComposedGeometryNode::setCcwUnchecked(
                            std::any_cast<SFBool>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "color", X3DFieldType::SFNode, AccessType::InputOutput, "color",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const IndexedTriangleSet &>(n)
                              .X3DComposedGeometryNode::getColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedTriangleSet &>(n)
              .X3DComposedGeometryNode::setColor(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "colorPerVertex", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const IndexedTriangleSet &>(n)
                              .X3DComposedGeometryNode::getColorPerVertex());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedTriangleSet &>(n)
              .X3DComposedGeometryNode::setColorPerVertexUnchecked(
                  std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "coord", X3DFieldType::SFNode, AccessType::InputOutput, "coord",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const IndexedTriangleSet &>(n)
                              .X3DComposedGeometryNode::getCoord());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedTriangleSet &>(n)
              .X3DComposedGeometryNode::setCoord(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "fogCoord", X3DFieldType::SFNode, AccessType::InputOutput, "fogCoord",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const IndexedTriangleSet &>(n)
                              .X3DComposedGeometryNode::getFogCoord());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedTriangleSet &>(n)
              .X3DComposedGeometryNode::setFogCoord(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "index", X3DFieldType::MFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const IndexedTriangleSet &>(n).getIndex());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedTriangleSet &>(n).setIndexUnchecked(
              std::any_cast<MFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const IndexedTriangleSet &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedTriangleSet &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const IndexedTriangleSet &>(n)
                              .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedTriangleSet &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "normal", X3DFieldType::SFNode, AccessType::InputOutput, "normal",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const IndexedTriangleSet &>(n)
                              .X3DComposedGeometryNode::getNormal());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedTriangleSet &>(n)
              .X3DComposedGeometryNode::setNormal(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "normalPerVertex", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const IndexedTriangleSet &>(n)
                              .X3DComposedGeometryNode::getNormalPerVertex());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedTriangleSet &>(n)
              .X3DComposedGeometryNode::setNormalPerVertexUnchecked(
                  std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{"set_index", X3DFieldType::MFInt32,
                          AccessType::InputOnly, "",

                          nullptr,

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<IndexedTriangleSet &>(n).onSet_index(
                                std::any_cast<MFInt32>(v));
                          },

                          nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"solid", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const IndexedTriangleSet &>(n)
                                        .X3DComposedGeometryNode::getSolid());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<IndexedTriangleSet &>(n)
                        .X3DComposedGeometryNode::setSolidUnchecked(
                            std::any_cast<SFBool>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "texCoord", X3DFieldType::SFNode, AccessType::InputOutput, "texCoord",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const IndexedTriangleSet &>(n)
                              .X3DComposedGeometryNode::getTexCoord());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedTriangleSet &>(n)
              .X3DComposedGeometryNode::setTexCoord(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const IndexedTriangleSet &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedTriangleSet &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const IndexedTriangleSet &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedTriangleSet &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const IndexedTriangleSet &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedTriangleSet &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const IndexedTriangleSet &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedTriangleSet &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const IndexedTriangleSet &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedTriangleSet &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void IndexedTriangleSet::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

} // namespace x3d::nodes
