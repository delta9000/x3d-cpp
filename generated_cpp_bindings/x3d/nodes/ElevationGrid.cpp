// ElevationGrid.cpp
#include "x3d/nodes/ElevationGrid.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string ElevationGrid::nodeTypeName() const { return "ElevationGrid"; }

std::string ElevationGrid::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &ElevationGrid::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "attrib", X3DFieldType::MFNode, AccessType::InputOutput, "attrib",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ElevationGrid &>(n).getAttrib());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ElevationGrid &>(n).setAttrib(std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "ccw", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ElevationGrid &>(n).getCcw());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ElevationGrid &>(n).setCcwUnchecked(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "color", X3DFieldType::SFNode, AccessType::InputOutput, "color",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ElevationGrid &>(n).getColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ElevationGrid &>(n).setColor(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "colorPerVertex", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ElevationGrid &>(n).getColorPerVertex());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ElevationGrid &>(n).setColorPerVertexUnchecked(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "creaseAngle", X3DFieldType::SFFloat, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ElevationGrid &>(n).getCreaseAngle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ElevationGrid &>(n).setCreaseAngleUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "fogCoord", X3DFieldType::SFNode, AccessType::InputOutput, "fogCoord",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ElevationGrid &>(n).getFogCoord());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ElevationGrid &>(n).setFogCoord(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "height", X3DFieldType::MFFloat, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ElevationGrid &>(n).getHeight());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ElevationGrid &>(n).setHeightUnchecked(
              std::any_cast<MFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ElevationGrid &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ElevationGrid &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ElevationGrid &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ElevationGrid &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "normal", X3DFieldType::SFNode, AccessType::InputOutput, "normal",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ElevationGrid &>(n).getNormal());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ElevationGrid &>(n).setNormal(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "normalPerVertex", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ElevationGrid &>(n).getNormalPerVertex());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ElevationGrid &>(n).setNormalPerVertexUnchecked(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{"set_height", X3DFieldType::MFFloat,
                          AccessType::InputOnly, "",

                          nullptr,

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<ElevationGrid &>(n).onSet_height(
                                std::any_cast<MFFloat>(v));
                          },

                          nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "solid", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ElevationGrid &>(n).getSolid());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ElevationGrid &>(n).setSolidUnchecked(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "texCoord", X3DFieldType::SFNode, AccessType::InputOutput, "texCoord",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ElevationGrid &>(n).getTexCoord());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ElevationGrid &>(n).setTexCoord(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "xDimension", X3DFieldType::SFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ElevationGrid &>(n).getXDimension());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ElevationGrid &>(n).setXDimensionUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "xSpacing", X3DFieldType::SFFloat, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ElevationGrid &>(n).getXSpacing());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ElevationGrid &>(n).setXSpacingUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "zDimension", X3DFieldType::SFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ElevationGrid &>(n).getZDimension());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ElevationGrid &>(n).setZDimensionUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "zSpacing", X3DFieldType::SFFloat, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ElevationGrid &>(n).getZSpacing());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ElevationGrid &>(n).setZSpacingUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ElevationGrid &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ElevationGrid &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ElevationGrid &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ElevationGrid &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ElevationGrid &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ElevationGrid &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ElevationGrid &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ElevationGrid &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ElevationGrid &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ElevationGrid &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void ElevationGrid::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void ElevationGrid::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesCreaseAngle(getCreaseAngle(), nodeTypeName(), "", out);

  checkRangesXDimension(getXDimension(), nodeTypeName(), "", out);

  checkRangesZDimension(getZDimension(), nodeTypeName(), "", out);
}

void ElevationGrid::checkRangesCreaseAngle(const SFFloat &value,
                                           const std::string &nodeType,
                                           const std::string &defName,
                                           std::vector<RangeDiagnostic> &out) {
  if (value < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "creaseAngle",
                                  "creaseAngle below minimum of 0"});
}

void ElevationGrid::checkRangesXDimension(const SFInt32 &value,
                                          const std::string &nodeType,
                                          const std::string &defName,
                                          std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "xDimension",
                                  "xDimension below minimum of 0"});
}

void ElevationGrid::checkRangesZDimension(const SFInt32 &value,
                                          const std::string &nodeType,
                                          const std::string &defName,
                                          std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "zDimension",
                                  "zDimension below minimum of 0"});
}

namespace factory_detail {
std::shared_ptr<X3DNode> createElevationGrid() {
  return std::make_shared<ElevationGrid>();
}
} // namespace factory_detail

} // namespace x3d::nodes
