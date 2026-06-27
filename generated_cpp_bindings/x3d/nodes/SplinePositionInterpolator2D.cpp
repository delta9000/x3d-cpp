// SplinePositionInterpolator2D.cpp
#include "x3d/nodes/SplinePositionInterpolator2D.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string SplinePositionInterpolator2D::nodeTypeName() const {
  return "SplinePositionInterpolator2D";
}

std::string SplinePositionInterpolator2D::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &SplinePositionInterpolator2D::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "closed", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SplinePositionInterpolator2D &>(n)
                              .getClosed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SplinePositionInterpolator2D &>(n).setClosed(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SplinePositionInterpolator2D &>(n)
                              .X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SplinePositionInterpolator2D &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "key", X3DFieldType::MFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SplinePositionInterpolator2D &>(n)
                              .X3DInterpolatorNode::getKey());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SplinePositionInterpolator2D &>(n)
              .X3DInterpolatorNode::setKey(std::any_cast<MFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "keyValue", X3DFieldType::MFVec2f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SplinePositionInterpolator2D &>(n)
                              .getKeyValue());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SplinePositionInterpolator2D &>(n).setKeyValue(
              std::any_cast<MFVec2f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "keyVelocity", X3DFieldType::MFVec2f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SplinePositionInterpolator2D &>(n)
                              .getKeyVelocity());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SplinePositionInterpolator2D &>(n).setKeyVelocity(
              std::any_cast<MFVec2f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SplinePositionInterpolator2D &>(n)
                              .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SplinePositionInterpolator2D &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "normalizeVelocity", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SplinePositionInterpolator2D &>(n)
                              .getNormalizeVelocity());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SplinePositionInterpolator2D &>(n).setNormalizeVelocity(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "set_fraction", X3DFieldType::SFFloat, AccessType::InputOnly, "",

        nullptr,

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SplinePositionInterpolator2D &>(n)
              .X3DInterpolatorNode::onSet_fraction(std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "value_changed", X3DFieldType::SFVec2f, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SplinePositionInterpolator2D &>(n)
                              .getValue_changed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SplinePositionInterpolator2D &>(n).emitValue_changed(
              std::any_cast<SFVec2f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SplinePositionInterpolator2D &>(n)
                              .X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SplinePositionInterpolator2D &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SplinePositionInterpolator2D &>(n)
                              .X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SplinePositionInterpolator2D &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SplinePositionInterpolator2D &>(n)
                              .X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SplinePositionInterpolator2D &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SplinePositionInterpolator2D &>(n)
                              .X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SplinePositionInterpolator2D &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const SplinePositionInterpolator2D &>(n)
                              .X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SplinePositionInterpolator2D &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void SplinePositionInterpolator2D::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

} // namespace x3d::nodes
