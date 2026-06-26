// GeoLOD.cpp
#include "GeoLOD.hpp"

std::string GeoLOD::nodeTypeName() const { return "GeoLOD"; }

std::string GeoLOD::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &GeoLOD::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "bboxCenter", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeoLOD &>(n)
                              .X3DBoundedObject::getBboxCenter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoLOD &>(n).X3DBoundedObject::setBboxCenterUnchecked(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxDisplay", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeoLOD &>(n)
                              .X3DBoundedObject::getBboxDisplay());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoLOD &>(n).X3DBoundedObject::setBboxDisplay(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxSize", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const GeoLOD &>(n).X3DBoundedObject::getBboxSize());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoLOD &>(n).X3DBoundedObject::setBboxSizeUnchecked(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "center", X3DFieldType::SFVec3d, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeoLOD &>(n).getCenter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoLOD &>(n).setCenterUnchecked(
              std::any_cast<SFVec3d>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "child1Url", X3DFieldType::MFString, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeoLOD &>(n).getChild1Url());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoLOD &>(n).setChild1UrlUnchecked(
              std::any_cast<MFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "child2Url", X3DFieldType::MFString, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeoLOD &>(n).getChild2Url());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoLOD &>(n).setChild2UrlUnchecked(
              std::any_cast<MFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "child3Url", X3DFieldType::MFString, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeoLOD &>(n).getChild3Url());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoLOD &>(n).setChild3UrlUnchecked(
              std::any_cast<MFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "child4Url", X3DFieldType::MFString, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeoLOD &>(n).getChild4Url());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoLOD &>(n).setChild4UrlUnchecked(
              std::any_cast<MFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "children", X3DFieldType::MFNode, AccessType::OutputOnly, "children",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeoLOD &>(n).getChildren());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoLOD &>(n).emitChildren(std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{"geoOrigin", X3DFieldType::SFNode,
                          AccessType::InitializeOnly, "geoOrigin",

                          [](const X3DNode &n) -> std::any {
                            return std::any(
                                dynamic_cast<const GeoLOD &>(n).getGeoOrigin());
                          },

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<GeoLOD &>(n).setGeoOriginUnchecked(
                                std::any_cast<SFNode>(v));
                          },

                          nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "geoSystem", X3DFieldType::MFString, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeoLOD &>(n).getGeoSystem());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoLOD &>(n).setGeoSystemUnchecked(
              std::any_cast<MFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeoLOD &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoLOD &>(n).X3DNode::setIS(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "level_changed", X3DFieldType::SFInt32, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeoLOD &>(n).getLevel_changed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoLOD &>(n).emitLevel_changed(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const GeoLOD &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoLOD &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "range", X3DFieldType::SFFloat, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeoLOD &>(n).getRange());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoLOD &>(n).setRangeUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{"rootNode", X3DFieldType::MFNode,
                          AccessType::InitializeOnly, "rootNode",

                          [](const X3DNode &n) -> std::any {
                            return std::any(
                                dynamic_cast<const GeoLOD &>(n).getRootNode());
                          },

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<GeoLOD &>(n).setRootNodeUnchecked(
                                std::any_cast<MFNode>(v));
                          },

                          nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "rootUrl", X3DFieldType::MFString, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeoLOD &>(n).getRootUrl());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoLOD &>(n).setRootUrlUnchecked(
              std::any_cast<MFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "visible", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const GeoLOD &>(n).X3DBoundedObject::getVisible());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoLOD &>(n).X3DBoundedObject::setVisible(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeoLOD &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoLOD &>(n).X3DNode::setDEF(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeoLOD &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoLOD &>(n).X3DNode::setUSE(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeoLOD &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoLOD &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeoLOD &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoLOD &>(n).X3DNode::setId(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const GeoLOD &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<GeoLOD &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void GeoLOD::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}
