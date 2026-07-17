// NurbsSurfaceInterpolator.cpp
#include "x3d/nodes/NurbsSurfaceInterpolator.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string NurbsSurfaceInterpolator::nodeTypeName() const {
  return "NurbsSurfaceInterpolator";
}

std::string NurbsSurfaceInterpolator::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &NurbsSurfaceInterpolator::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "controlPoint", X3DFieldType::SFNode, AccessType::InputOutput,
        "controlPoint",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsSurfaceInterpolator &>(n)
                              .getControlPoint());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSurfaceInterpolator &>(n).setControlPoint(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsSurfaceInterpolator &>(n)
                              .X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSurfaceInterpolator &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsSurfaceInterpolator &>(n)
                              .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSurfaceInterpolator &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "normal_changed", X3DFieldType::SFVec3f, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsSurfaceInterpolator &>(n)
                              .getNormal_changed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSurfaceInterpolator &>(n).emitNormal_changed(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "position_changed", X3DFieldType::SFVec3f, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsSurfaceInterpolator &>(n)
                              .getPosition_changed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSurfaceInterpolator &>(n).emitPosition_changed(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "set_fraction", X3DFieldType::SFVec2f, AccessType::InputOnly, "",

        nullptr,

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSurfaceInterpolator &>(n).onSet_fraction(
              std::any_cast<SFVec2f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "uDimension", X3DFieldType::SFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsSurfaceInterpolator &>(n)
                              .getUDimension());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSurfaceInterpolator &>(n).setUDimensionUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "uKnot", X3DFieldType::MFDouble, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NurbsSurfaceInterpolator &>(n).getUKnot());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSurfaceInterpolator &>(n).setUKnotUnchecked(
              std::any_cast<MFDouble>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "uOrder", X3DFieldType::SFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NurbsSurfaceInterpolator &>(n).getUOrder());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSurfaceInterpolator &>(n).setUOrderUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "vDimension", X3DFieldType::SFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsSurfaceInterpolator &>(n)
                              .getVDimension());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSurfaceInterpolator &>(n).setVDimensionUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "vKnot", X3DFieldType::MFDouble, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NurbsSurfaceInterpolator &>(n).getVKnot());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSurfaceInterpolator &>(n).setVKnotUnchecked(
              std::any_cast<MFDouble>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "vOrder", X3DFieldType::SFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NurbsSurfaceInterpolator &>(n).getVOrder());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSurfaceInterpolator &>(n).setVOrderUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "weight", X3DFieldType::MFDouble, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NurbsSurfaceInterpolator &>(n).getWeight());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSurfaceInterpolator &>(n).setWeight(
              std::any_cast<MFDouble>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsSurfaceInterpolator &>(n)
                              .X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSurfaceInterpolator &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsSurfaceInterpolator &>(n)
                              .X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSurfaceInterpolator &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsSurfaceInterpolator &>(n)
                              .X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSurfaceInterpolator &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsSurfaceInterpolator &>(n)
                              .X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSurfaceInterpolator &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsSurfaceInterpolator &>(n)
                              .X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSurfaceInterpolator &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void NurbsSurfaceInterpolator::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void NurbsSurfaceInterpolator::validateRanges(
    std::vector<RangeDiagnostic> &out) const {

  checkRangesUDimension(getUDimension(), nodeTypeName(), "", out);

  checkRangesUOrder(getUOrder(), nodeTypeName(), "", out);

  checkRangesVDimension(getVDimension(), nodeTypeName(), "", out);

  checkRangesVOrder(getVOrder(), nodeTypeName(), "", out);
}

void NurbsSurfaceInterpolator::checkRangesUDimension(
    const SFInt32 &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "uDimension",
                                  "uDimension below minimum of 0"});
}

void NurbsSurfaceInterpolator::checkRangesUOrder(
    const SFInt32 &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 2)
    out.push_back(RangeDiagnostic{nodeType, defName, "uOrder",
                                  "uOrder below minimum of 2"});
}

void NurbsSurfaceInterpolator::checkRangesVDimension(
    const SFInt32 &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "vDimension",
                                  "vDimension below minimum of 0"});
}

void NurbsSurfaceInterpolator::checkRangesVOrder(
    const SFInt32 &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 2)
    out.push_back(RangeDiagnostic{nodeType, defName, "vOrder",
                                  "vOrder below minimum of 2"});
}

namespace factory_detail {
std::shared_ptr<X3DNode> createNurbsSurfaceInterpolator() {
  return std::make_shared<NurbsSurfaceInterpolator>();
}
} // namespace factory_detail

} // namespace x3d::nodes
