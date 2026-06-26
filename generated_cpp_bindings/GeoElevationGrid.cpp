// GeoElevationGrid.cpp
#include "GeoElevationGrid.hpp"

std::string GeoElevationGrid::nodeTypeName() const {
  return "GeoElevationGrid";
}

std::string GeoElevationGrid::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &GeoElevationGrid::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "ccw", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeoElevationGrid &>(n).getCcw());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoElevationGrid &>(n).setCcwUnchecked(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "color", X3DFieldType::SFNode, AccessType::InputOutput, "color",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeoElevationGrid &>(n).getColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoElevationGrid &>(n).setColor(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "colorPerVertex", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const GeoElevationGrid &>(n).getColorPerVertex());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoElevationGrid &>(n).setColorPerVertexUnchecked(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "creaseAngle", X3DFieldType::SFDouble, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const GeoElevationGrid &>(n).getCreaseAngle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoElevationGrid &>(n).setCreaseAngleUnchecked(
              std::any_cast<SFDouble>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "geoGridOrigin", X3DFieldType::SFVec3d, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const GeoElevationGrid &>(n).getGeoGridOrigin());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoElevationGrid &>(n).setGeoGridOriginUnchecked(
              std::any_cast<SFVec3d>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "geoOrigin", X3DFieldType::SFNode, AccessType::InitializeOnly,
        "geoOrigin",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const GeoElevationGrid &>(n).getGeoOrigin());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoElevationGrid &>(n).setGeoOriginUnchecked(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "geoSystem", X3DFieldType::MFString, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const GeoElevationGrid &>(n).getGeoSystem());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoElevationGrid &>(n).setGeoSystemUnchecked(
              std::any_cast<MFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "height", X3DFieldType::MFDouble, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const GeoElevationGrid &>(n).getHeight());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoElevationGrid &>(n).setHeightUnchecked(
              std::any_cast<MFDouble>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const GeoElevationGrid &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoElevationGrid &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const GeoElevationGrid &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoElevationGrid &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "normal", X3DFieldType::SFNode, AccessType::InputOutput, "normal",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const GeoElevationGrid &>(n).getNormal());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoElevationGrid &>(n).setNormal(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "normalPerVertex", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const GeoElevationGrid &>(n).getNormalPerVertex());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoElevationGrid &>(n).setNormalPerVertexUnchecked(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{"set_height", X3DFieldType::MFDouble,
                          AccessType::InputOnly, "",

                          nullptr,

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<GeoElevationGrid &>(n).onSet_height(
                                std::any_cast<MFDouble>(v));
                          },

                          nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "solid", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeoElevationGrid &>(n).getSolid());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoElevationGrid &>(n).setSolidUnchecked(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "texCoord", X3DFieldType::SFNode, AccessType::InputOutput, "texCoord",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const GeoElevationGrid &>(n).getTexCoord());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoElevationGrid &>(n).setTexCoord(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "xDimension", X3DFieldType::SFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const GeoElevationGrid &>(n).getXDimension());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoElevationGrid &>(n).setXDimensionUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "xSpacing", X3DFieldType::SFDouble, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const GeoElevationGrid &>(n).getXSpacing());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoElevationGrid &>(n).setXSpacingUnchecked(
              std::any_cast<SFDouble>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"yScale", X3DFieldType::SFFloat, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const GeoElevationGrid &>(n).getYScale());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<GeoElevationGrid &>(n).setYScaleUnchecked(
                        std::any_cast<SFFloat>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "zDimension", X3DFieldType::SFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const GeoElevationGrid &>(n).getZDimension());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoElevationGrid &>(n).setZDimensionUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "zSpacing", X3DFieldType::SFDouble, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const GeoElevationGrid &>(n).getZSpacing());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoElevationGrid &>(n).setZSpacingUnchecked(
              std::any_cast<SFDouble>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const GeoElevationGrid &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoElevationGrid &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const GeoElevationGrid &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoElevationGrid &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const GeoElevationGrid &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoElevationGrid &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const GeoElevationGrid &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoElevationGrid &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const GeoElevationGrid &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoElevationGrid &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void GeoElevationGrid::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void GeoElevationGrid::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesYScale(getYScale(), nodeTypeName(), "", out);
}

void GeoElevationGrid::checkRangesYScale(const SFFloat &value,
                                         const std::string &nodeType,
                                         const std::string &defName,
                                         std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "yScale",
                                  "yScale below minimum of 0"});
}
