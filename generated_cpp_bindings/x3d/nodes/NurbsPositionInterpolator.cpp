// NurbsPositionInterpolator.cpp
#include "x3d/nodes/NurbsPositionInterpolator.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string NurbsPositionInterpolator::nodeTypeName() const {
  return "NurbsPositionInterpolator";
}

std::string NurbsPositionInterpolator::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &NurbsPositionInterpolator::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "controlPoint", X3DFieldType::SFNode, AccessType::InputOutput,
        "controlPoint",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsPositionInterpolator &>(n)
                              .getControlPoint());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPositionInterpolator &>(n).setControlPoint(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsPositionInterpolator &>(n)
                              .X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPositionInterpolator &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "knot", X3DFieldType::MFDouble, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NurbsPositionInterpolator &>(n).getKnot());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPositionInterpolator &>(n).setKnot(
              std::any_cast<MFDouble>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsPositionInterpolator &>(n)
                              .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPositionInterpolator &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "order", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NurbsPositionInterpolator &>(n).getOrder());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPositionInterpolator &>(n).setOrderUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "set_fraction", X3DFieldType::SFFloat, AccessType::InputOnly, "",

        nullptr,

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPositionInterpolator &>(n).onSet_fraction(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "value_changed", X3DFieldType::SFVec3f, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsPositionInterpolator &>(n)
                              .getValue_changed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPositionInterpolator &>(n).emitValue_changed(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "weight", X3DFieldType::MFDouble, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NurbsPositionInterpolator &>(n).getWeight());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPositionInterpolator &>(n).setWeight(
              std::any_cast<MFDouble>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsPositionInterpolator &>(n)
                              .X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPositionInterpolator &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsPositionInterpolator &>(n)
                              .X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPositionInterpolator &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsPositionInterpolator &>(n)
                              .X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPositionInterpolator &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsPositionInterpolator &>(n)
                              .X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPositionInterpolator &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsPositionInterpolator &>(n)
                              .X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsPositionInterpolator &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void NurbsPositionInterpolator::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void NurbsPositionInterpolator::validateRanges(
    std::vector<RangeDiagnostic> &out) const {

  checkRangesOrder(getOrder(), nodeTypeName(), "", out);
}

void NurbsPositionInterpolator::checkRangesOrder(
    const SFInt32 &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 2)
    out.push_back(RangeDiagnostic{nodeType, defName, "order",
                                  "order below minimum of 2"});
}

} // namespace x3d::nodes
