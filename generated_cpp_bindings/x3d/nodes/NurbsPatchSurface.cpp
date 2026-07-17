// NurbsPatchSurface.cpp
#include "x3d/nodes/NurbsPatchSurface.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string NurbsPatchSurface::nodeTypeName() const {
  return "NurbsPatchSurface";
}

std::string NurbsPatchSurface::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &NurbsPatchSurface::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "controlPoint", X3DFieldType::SFNode, AccessType::InputOutput,
        "controlPoint",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsPatchSurface &>(n)
                              .X3DNurbsSurfaceGeometryNode::getControlPoint());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPatchSurface &>(n)
              .X3DNurbsSurfaceGeometryNode::setControlPoint(
                  std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NurbsPatchSurface &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPatchSurface &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsPatchSurface &>(n)
                              .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPatchSurface &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "solid", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsPatchSurface &>(n)
                              .X3DNurbsSurfaceGeometryNode::getSolid());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPatchSurface &>(n)
              .X3DNurbsSurfaceGeometryNode::setSolidUnchecked(
                  std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "texCoord", X3DFieldType::SFNode, AccessType::InputOutput, "texCoord",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsPatchSurface &>(n)
                              .X3DNurbsSurfaceGeometryNode::getTexCoord());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPatchSurface &>(n)
              .X3DNurbsSurfaceGeometryNode::setTexCoord(
                  std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "uClosed", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsPatchSurface &>(n)
                              .X3DNurbsSurfaceGeometryNode::getUClosed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPatchSurface &>(n)
              .X3DNurbsSurfaceGeometryNode::setUClosedUnchecked(
                  std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "uDimension", X3DFieldType::SFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsPatchSurface &>(n)
                              .X3DNurbsSurfaceGeometryNode::getUDimension());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPatchSurface &>(n)
              .X3DNurbsSurfaceGeometryNode::setUDimensionUnchecked(
                  std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "uKnot", X3DFieldType::MFDouble, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsPatchSurface &>(n)
                              .X3DNurbsSurfaceGeometryNode::getUKnot());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPatchSurface &>(n)
              .X3DNurbsSurfaceGeometryNode::setUKnotUnchecked(
                  std::any_cast<MFDouble>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "uOrder", X3DFieldType::SFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsPatchSurface &>(n)
                              .X3DNurbsSurfaceGeometryNode::getUOrder());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPatchSurface &>(n)
              .X3DNurbsSurfaceGeometryNode::setUOrderUnchecked(
                  std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "uTessellation", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsPatchSurface &>(n)
                              .X3DNurbsSurfaceGeometryNode::getUTessellation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPatchSurface &>(n)
              .X3DNurbsSurfaceGeometryNode::setUTessellation(
                  std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "vClosed", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsPatchSurface &>(n)
                              .X3DNurbsSurfaceGeometryNode::getVClosed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPatchSurface &>(n)
              .X3DNurbsSurfaceGeometryNode::setVClosedUnchecked(
                  std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "vDimension", X3DFieldType::SFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsPatchSurface &>(n)
                              .X3DNurbsSurfaceGeometryNode::getVDimension());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPatchSurface &>(n)
              .X3DNurbsSurfaceGeometryNode::setVDimensionUnchecked(
                  std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "vKnot", X3DFieldType::MFDouble, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsPatchSurface &>(n)
                              .X3DNurbsSurfaceGeometryNode::getVKnot());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPatchSurface &>(n)
              .X3DNurbsSurfaceGeometryNode::setVKnotUnchecked(
                  std::any_cast<MFDouble>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "vOrder", X3DFieldType::SFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsPatchSurface &>(n)
                              .X3DNurbsSurfaceGeometryNode::getVOrder());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPatchSurface &>(n)
              .X3DNurbsSurfaceGeometryNode::setVOrderUnchecked(
                  std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "vTessellation", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsPatchSurface &>(n)
                              .X3DNurbsSurfaceGeometryNode::getVTessellation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPatchSurface &>(n)
              .X3DNurbsSurfaceGeometryNode::setVTessellation(
                  std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "weight", X3DFieldType::MFDouble, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsPatchSurface &>(n)
                              .X3DNurbsSurfaceGeometryNode::getWeight());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPatchSurface &>(n)
              .X3DNurbsSurfaceGeometryNode::setWeight(
                  std::any_cast<MFDouble>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NurbsPatchSurface &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPatchSurface &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NurbsPatchSurface &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPatchSurface &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NurbsPatchSurface &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPatchSurface &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NurbsPatchSurface &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPatchSurface &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NurbsPatchSurface &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPatchSurface &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void NurbsPatchSurface::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void NurbsPatchSurface::validateRanges(
    std::vector<RangeDiagnostic> &out) const {

  X3DNurbsSurfaceGeometryNode::checkRangesUDimension(
      X3DNurbsSurfaceGeometryNode::getUDimension(), nodeTypeName(), "", out);

  X3DNurbsSurfaceGeometryNode::checkRangesUOrder(
      X3DNurbsSurfaceGeometryNode::getUOrder(), nodeTypeName(), "", out);

  X3DNurbsSurfaceGeometryNode::checkRangesVDimension(
      X3DNurbsSurfaceGeometryNode::getVDimension(), nodeTypeName(), "", out);

  X3DNurbsSurfaceGeometryNode::checkRangesVOrder(
      X3DNurbsSurfaceGeometryNode::getVOrder(), nodeTypeName(), "", out);
}

namespace factory_detail {
std::shared_ptr<X3DNode> createNurbsPatchSurface() {
  return std::make_shared<NurbsPatchSurface>();
}
} // namespace factory_detail

} // namespace x3d::nodes
