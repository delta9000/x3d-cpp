// BlendedVolumeStyle.cpp
#include "x3d/nodes/BlendedVolumeStyle.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string BlendedVolumeStyle::nodeTypeName() const {
  return "BlendedVolumeStyle";
}

std::string BlendedVolumeStyle::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &BlendedVolumeStyle::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const BlendedVolumeStyle &>(n)
                              .X3DVolumeRenderStyleNode::getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BlendedVolumeStyle &>(n)
              .X3DVolumeRenderStyleNode::setEnabled(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BlendedVolumeStyle &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BlendedVolumeStyle &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const BlendedVolumeStyle &>(n)
                              .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BlendedVolumeStyle &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "renderStyle", X3DFieldType::SFNode, AccessType::InputOutput,
        "renderStyle",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BlendedVolumeStyle &>(n).getRenderStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BlendedVolumeStyle &>(n).setRenderStyle(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "voxels", X3DFieldType::SFNode, AccessType::InputOutput, "voxels",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BlendedVolumeStyle &>(n).getVoxels());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BlendedVolumeStyle &>(n).setVoxels(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "weightConstant1", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BlendedVolumeStyle &>(n).getWeightConstant1());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BlendedVolumeStyle &>(n).setWeightConstant1Unchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "weightConstant2", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BlendedVolumeStyle &>(n).getWeightConstant2());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BlendedVolumeStyle &>(n).setWeightConstant2Unchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "weightFunction1", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BlendedVolumeStyle &>(n).getWeightFunction1());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BlendedVolumeStyle &>(n).setWeightFunction1(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "weightFunction2", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BlendedVolumeStyle &>(n).getWeightFunction2());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BlendedVolumeStyle &>(n).setWeightFunction2(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "weightTransferFunction1", X3DFieldType::SFNode,
        AccessType::InputOutput, "weightTransferFunction1",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const BlendedVolumeStyle &>(n)
                              .getWeightTransferFunction1());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BlendedVolumeStyle &>(n).setWeightTransferFunction1(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "weightTransferFunction2", X3DFieldType::SFNode,
        AccessType::InputOutput, "weightTransferFunction2",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const BlendedVolumeStyle &>(n)
                              .getWeightTransferFunction2());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BlendedVolumeStyle &>(n).setWeightTransferFunction2(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BlendedVolumeStyle &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BlendedVolumeStyle &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BlendedVolumeStyle &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BlendedVolumeStyle &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BlendedVolumeStyle &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BlendedVolumeStyle &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BlendedVolumeStyle &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BlendedVolumeStyle &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BlendedVolumeStyle &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BlendedVolumeStyle &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void BlendedVolumeStyle::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void BlendedVolumeStyle::validateRanges(
    std::vector<RangeDiagnostic> &out) const {

  checkRangesWeightConstant1(getWeightConstant1(), nodeTypeName(), "", out);

  checkRangesWeightConstant2(getWeightConstant2(), nodeTypeName(), "", out);
}

void BlendedVolumeStyle::checkRangesWeightConstant1(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "weightConstant1",
                                  "weightConstant1 below minimum of 0"});
  if (value > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "weightConstant1",
                                  "weightConstant1 above maximum of 1"});
}

void BlendedVolumeStyle::checkRangesWeightConstant2(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "weightConstant2",
                                  "weightConstant2 below minimum of 0"});
  if (value > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "weightConstant2",
                                  "weightConstant2 above maximum of 1"});
}

namespace factory_detail {
std::shared_ptr<X3DNode> createBlendedVolumeStyle() {
  return std::make_shared<BlendedVolumeStyle>();
}
} // namespace factory_detail

} // namespace x3d::nodes
