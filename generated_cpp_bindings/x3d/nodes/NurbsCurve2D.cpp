// NurbsCurve2D.cpp
#include "x3d/nodes/NurbsCurve2D.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string NurbsCurve2D::nodeTypeName() const { return "NurbsCurve2D"; }

std::string NurbsCurve2D::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &NurbsCurve2D::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "closed", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsCurve2D &>(n).getClosed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsCurve2D &>(n).setClosedUnchecked(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "controlPoint", X3DFieldType::MFVec2d, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsCurve2D &>(n)
                              .X3DNurbsControlCurveNode::getControlPoint());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsCurve2D &>(n)
              .X3DNurbsControlCurveNode::setControlPoint(
                  std::any_cast<MFVec2d>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const NurbsCurve2D &>(n).X3DNode::getIS());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<NurbsCurve2D &>(n).X3DNode::setIS(
                        std::any_cast<SFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "knot", X3DFieldType::MFDouble, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsCurve2D &>(n).getKnot());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsCurve2D &>(n).setKnotUnchecked(
              std::any_cast<MFDouble>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NurbsCurve2D &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsCurve2D &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "order", X3DFieldType::SFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsCurve2D &>(n).getOrder());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsCurve2D &>(n).setOrderUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "tessellation", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NurbsCurve2D &>(n).getTessellation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsCurve2D &>(n).setTessellation(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "weight", X3DFieldType::MFDouble, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsCurve2D &>(n).getWeight());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsCurve2D &>(n).setWeight(std::any_cast<MFDouble>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NurbsCurve2D &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsCurve2D &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NurbsCurve2D &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsCurve2D &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NurbsCurve2D &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsCurve2D &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"id", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const NurbsCurve2D &>(n).X3DNode::getId());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<NurbsCurve2D &>(n).X3DNode::setId(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NurbsCurve2D &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsCurve2D &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void NurbsCurve2D::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

} // namespace x3d::nodes
