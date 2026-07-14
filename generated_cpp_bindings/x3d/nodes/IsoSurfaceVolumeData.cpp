// IsoSurfaceVolumeData.cpp
#include "x3d/nodes/IsoSurfaceVolumeData.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string IsoSurfaceVolumeData::nodeTypeName() const {
  return "IsoSurfaceVolumeData";
}

std::string IsoSurfaceVolumeData::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &IsoSurfaceVolumeData::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "bboxCenter", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const IsoSurfaceVolumeData &>(n)
                              .X3DVolumeDataNode::getBboxCenter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IsoSurfaceVolumeData &>(n)
              .X3DVolumeDataNode::setBboxCenterUnchecked(
                  std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxDisplay", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const IsoSurfaceVolumeData &>(n)
                              .X3DVolumeDataNode::getBboxDisplay());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IsoSurfaceVolumeData &>(n)
              .X3DVolumeDataNode::setBboxDisplay(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxSize", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const IsoSurfaceVolumeData &>(n)
                              .X3DVolumeDataNode::getBboxSize());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IsoSurfaceVolumeData &>(n)
              .X3DVolumeDataNode::setBboxSizeUnchecked(
                  std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "contourStepSize", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const IsoSurfaceVolumeData &>(n)
                              .getContourStepSize());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IsoSurfaceVolumeData &>(n).setContourStepSize(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "dimensions", X3DFieldType::SFVec3f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const IsoSurfaceVolumeData &>(n)
                              .X3DVolumeDataNode::getDimensions());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IsoSurfaceVolumeData &>(n)
              .X3DVolumeDataNode::setDimensions(std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "gradients", X3DFieldType::SFNode, AccessType::InputOutput, "gradients",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const IsoSurfaceVolumeData &>(n).getGradients());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IsoSurfaceVolumeData &>(n).setGradients(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const IsoSurfaceVolumeData &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IsoSurfaceVolumeData &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const IsoSurfaceVolumeData &>(n)
                              .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IsoSurfaceVolumeData &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "renderStyle", X3DFieldType::MFNode, AccessType::InputOutput,
        "renderStyle",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const IsoSurfaceVolumeData &>(n).getRenderStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IsoSurfaceVolumeData &>(n).setRenderStyle(
              std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "surfaceTolerance", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const IsoSurfaceVolumeData &>(n)
                              .getSurfaceTolerance());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IsoSurfaceVolumeData &>(n).setSurfaceToleranceUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "surfaceValues", X3DFieldType::MFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const IsoSurfaceVolumeData &>(n).getSurfaceValues());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IsoSurfaceVolumeData &>(n).setSurfaceValues(
              std::any_cast<MFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "visible", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const IsoSurfaceVolumeData &>(n)
                              .X3DVolumeDataNode::getVisible());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IsoSurfaceVolumeData &>(n).X3DVolumeDataNode::setVisible(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "voxels", X3DFieldType::SFNode, AccessType::InputOutput, "voxels",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const IsoSurfaceVolumeData &>(n).getVoxels());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IsoSurfaceVolumeData &>(n).setVoxels(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const IsoSurfaceVolumeData &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IsoSurfaceVolumeData &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const IsoSurfaceVolumeData &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IsoSurfaceVolumeData &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const IsoSurfaceVolumeData &>(n)
                              .X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IsoSurfaceVolumeData &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const IsoSurfaceVolumeData &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IsoSurfaceVolumeData &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const IsoSurfaceVolumeData &>(n)
                              .X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IsoSurfaceVolumeData &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void IsoSurfaceVolumeData::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void IsoSurfaceVolumeData::validateRanges(
    std::vector<RangeDiagnostic> &out) const {

  checkRangesSurfaceTolerance(getSurfaceTolerance(), nodeTypeName(), "", out);
}

void IsoSurfaceVolumeData::checkRangesSurfaceTolerance(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "surfaceTolerance",
                                  "surfaceTolerance below minimum of 0"});
}

namespace factory_detail {
std::shared_ptr<X3DNode> createIsoSurfaceVolumeData() {
  return std::make_shared<IsoSurfaceVolumeData>();
}
} // namespace factory_detail

} // namespace x3d::nodes
