// X3DNurbsSurfaceGeometryNode.cpp
#include "X3DNurbsSurfaceGeometryNode.hpp"

std::string X3DNurbsSurfaceGeometryNode::nodeTypeName() const {
  return "X3DNurbsSurfaceGeometryNode";
}

std::string X3DNurbsSurfaceGeometryNode::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &X3DNurbsSurfaceGeometryNode::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "controlPoint", X3DFieldType::SFNode, AccessType::InputOutput,
        "controlPoint",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DNurbsSurfaceGeometryNode &>(n)
                              .getControlPoint());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DNurbsSurfaceGeometryNode &>(n).setControlPoint(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DNurbsSurfaceGeometryNode &>(n)
                              .X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DNurbsSurfaceGeometryNode &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DNurbsSurfaceGeometryNode &>(n)
                              .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DNurbsSurfaceGeometryNode &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "solid", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DNurbsSurfaceGeometryNode &>(n).getSolid());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DNurbsSurfaceGeometryNode &>(n).setSolidUnchecked(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "texCoord", X3DFieldType::SFNode, AccessType::InputOutput, "texCoord",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DNurbsSurfaceGeometryNode &>(n)
                              .getTexCoord());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DNurbsSurfaceGeometryNode &>(n).setTexCoord(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "uClosed", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DNurbsSurfaceGeometryNode &>(n)
                              .getUClosed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DNurbsSurfaceGeometryNode &>(n).setUClosedUnchecked(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "uDimension", X3DFieldType::SFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DNurbsSurfaceGeometryNode &>(n)
                              .getUDimension());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DNurbsSurfaceGeometryNode &>(n).setUDimensionUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "uKnot", X3DFieldType::MFDouble, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DNurbsSurfaceGeometryNode &>(n).getUKnot());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DNurbsSurfaceGeometryNode &>(n).setUKnotUnchecked(
              std::any_cast<MFDouble>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "uOrder", X3DFieldType::SFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DNurbsSurfaceGeometryNode &>(n).getUOrder());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DNurbsSurfaceGeometryNode &>(n).setUOrderUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "uTessellation", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DNurbsSurfaceGeometryNode &>(n)
                              .getUTessellation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DNurbsSurfaceGeometryNode &>(n).setUTessellation(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "vClosed", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DNurbsSurfaceGeometryNode &>(n)
                              .getVClosed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DNurbsSurfaceGeometryNode &>(n).setVClosedUnchecked(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "vDimension", X3DFieldType::SFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DNurbsSurfaceGeometryNode &>(n)
                              .getVDimension());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DNurbsSurfaceGeometryNode &>(n).setVDimensionUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "vKnot", X3DFieldType::MFDouble, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DNurbsSurfaceGeometryNode &>(n).getVKnot());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DNurbsSurfaceGeometryNode &>(n).setVKnotUnchecked(
              std::any_cast<MFDouble>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "vOrder", X3DFieldType::SFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DNurbsSurfaceGeometryNode &>(n).getVOrder());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DNurbsSurfaceGeometryNode &>(n).setVOrderUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "vTessellation", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DNurbsSurfaceGeometryNode &>(n)
                              .getVTessellation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DNurbsSurfaceGeometryNode &>(n).setVTessellation(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "weight", X3DFieldType::MFDouble, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DNurbsSurfaceGeometryNode &>(n).getWeight());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DNurbsSurfaceGeometryNode &>(n).setWeight(
              std::any_cast<MFDouble>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DNurbsSurfaceGeometryNode &>(n)
                              .X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DNurbsSurfaceGeometryNode &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DNurbsSurfaceGeometryNode &>(n)
                              .X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DNurbsSurfaceGeometryNode &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DNurbsSurfaceGeometryNode &>(n)
                              .X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DNurbsSurfaceGeometryNode &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DNurbsSurfaceGeometryNode &>(n)
                              .X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DNurbsSurfaceGeometryNode &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DNurbsSurfaceGeometryNode &>(n)
                              .X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DNurbsSurfaceGeometryNode &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void X3DNurbsSurfaceGeometryNode::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}
