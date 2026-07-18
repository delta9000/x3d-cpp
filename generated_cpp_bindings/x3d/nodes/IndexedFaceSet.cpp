// IndexedFaceSet.cpp
#include "x3d/nodes/IndexedFaceSet.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string IndexedFaceSet::nodeTypeName() const { return "IndexedFaceSet"; }

std::string IndexedFaceSet::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &IndexedFaceSet::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "attrib", X3DFieldType::MFNode, AccessType::InputOutput, "attrib",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const IndexedFaceSet &>(n)
                              .X3DComposedGeometryNode::getAttrib());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedFaceSet &>(n).X3DComposedGeometryNode::setAttrib(
              std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"ccw", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const IndexedFaceSet &>(n)
                                        .X3DComposedGeometryNode::getCcw());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<IndexedFaceSet &>(n)
                        .X3DComposedGeometryNode::setCcwUnchecked(
                            std::any_cast<SFBool>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "color", X3DFieldType::SFNode, AccessType::InputOutput, "color",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const IndexedFaceSet &>(n)
                              .X3DComposedGeometryNode::getColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedFaceSet &>(n).X3DComposedGeometryNode::setColor(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "colorIndex", X3DFieldType::MFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const IndexedFaceSet &>(n).getColorIndex());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedFaceSet &>(n).setColorIndexUnchecked(
              std::any_cast<MFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "colorPerVertex", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const IndexedFaceSet &>(n)
                              .X3DComposedGeometryNode::getColorPerVertex());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedFaceSet &>(n)
              .X3DComposedGeometryNode::setColorPerVertexUnchecked(
                  std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "convex", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const IndexedFaceSet &>(n).getConvex());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedFaceSet &>(n).setConvexUnchecked(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "coord", X3DFieldType::SFNode, AccessType::InputOutput, "coord",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const IndexedFaceSet &>(n)
                              .X3DComposedGeometryNode::getCoord());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedFaceSet &>(n).X3DComposedGeometryNode::setCoord(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "coordIndex", X3DFieldType::MFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const IndexedFaceSet &>(n).getCoordIndex());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedFaceSet &>(n).setCoordIndexUnchecked(
              std::any_cast<MFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "creaseAngle", X3DFieldType::SFFloat, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const IndexedFaceSet &>(n).getCreaseAngle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedFaceSet &>(n).setCreaseAngleUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "fogCoord", X3DFieldType::SFNode, AccessType::InputOutput, "fogCoord",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const IndexedFaceSet &>(n)
                              .X3DComposedGeometryNode::getFogCoord());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedFaceSet &>(n)
              .X3DComposedGeometryNode::setFogCoord(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const IndexedFaceSet &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedFaceSet &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const IndexedFaceSet &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedFaceSet &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "normal", X3DFieldType::SFNode, AccessType::InputOutput, "normal",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const IndexedFaceSet &>(n)
                              .X3DComposedGeometryNode::getNormal());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedFaceSet &>(n).X3DComposedGeometryNode::setNormal(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "normalIndex", X3DFieldType::MFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const IndexedFaceSet &>(n).getNormalIndex());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedFaceSet &>(n).setNormalIndexUnchecked(
              std::any_cast<MFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "normalPerVertex", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const IndexedFaceSet &>(n)
                              .X3DComposedGeometryNode::getNormalPerVertex());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedFaceSet &>(n)
              .X3DComposedGeometryNode::setNormalPerVertexUnchecked(
                  std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{"set_colorIndex", X3DFieldType::MFInt32,
                          AccessType::InputOnly, "",

                          nullptr,

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<IndexedFaceSet &>(n).onSet_colorIndex(
                                std::any_cast<MFInt32>(v));
                          },

                          nullptr, nullptr

    });

    t.push_back(FieldInfo{"set_coordIndex", X3DFieldType::MFInt32,
                          AccessType::InputOnly, "",

                          nullptr,

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<IndexedFaceSet &>(n).onSet_coordIndex(
                                std::any_cast<MFInt32>(v));
                          },

                          nullptr, nullptr

    });

    t.push_back(FieldInfo{"set_normalIndex", X3DFieldType::MFInt32,
                          AccessType::InputOnly, "",

                          nullptr,

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<IndexedFaceSet &>(n).onSet_normalIndex(
                                std::any_cast<MFInt32>(v));
                          },

                          nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "set_texCoordIndex", X3DFieldType::MFInt32, AccessType::InputOnly, "",

        nullptr,

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedFaceSet &>(n).onSet_texCoordIndex(
              std::any_cast<MFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"solid", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const IndexedFaceSet &>(n)
                                        .X3DComposedGeometryNode::getSolid());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<IndexedFaceSet &>(n)
                        .X3DComposedGeometryNode::setSolidUnchecked(
                            std::any_cast<SFBool>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "texCoord", X3DFieldType::SFNode, AccessType::InputOutput, "texCoord",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const IndexedFaceSet &>(n)
                              .X3DComposedGeometryNode::getTexCoord());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedFaceSet &>(n)
              .X3DComposedGeometryNode::setTexCoord(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "texCoordIndex", X3DFieldType::MFInt32, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const IndexedFaceSet &>(n).getTexCoordIndex());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedFaceSet &>(n).setTexCoordIndexUnchecked(
              std::any_cast<MFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const IndexedFaceSet &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedFaceSet &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const IndexedFaceSet &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedFaceSet &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const IndexedFaceSet &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedFaceSet &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const IndexedFaceSet &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedFaceSet &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const IndexedFaceSet &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<IndexedFaceSet &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void IndexedFaceSet::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void IndexedFaceSet::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesColorIndex(getColorIndex(), nodeTypeName(), "", out);

  checkRangesCoordIndex(getCoordIndex(), nodeTypeName(), "", out);

  checkRangesCreaseAngle(getCreaseAngle(), nodeTypeName(), "", out);

  checkRangesNormalIndex(getNormalIndex(), nodeTypeName(), "", out);

  checkRangesTexCoordIndex(getTexCoordIndex(), nodeTypeName(), "", out);
}

void IndexedFaceSet::checkRangesColorIndex(const MFInt32 &value,
                                           const std::string &nodeType,
                                           const std::string &defName,
                                           std::vector<RangeDiagnostic> &out) {
  for (const auto &v : value) {

    if (v < -1)
      out.push_back(RangeDiagnostic{nodeType, defName, "colorIndex",
                                    "colorIndex below minimum of -1"});
  }
}

void IndexedFaceSet::checkRangesCoordIndex(const MFInt32 &value,
                                           const std::string &nodeType,
                                           const std::string &defName,
                                           std::vector<RangeDiagnostic> &out) {
  for (const auto &v : value) {

    if (v < -1)
      out.push_back(RangeDiagnostic{nodeType, defName, "coordIndex",
                                    "coordIndex below minimum of -1"});
  }
}

void IndexedFaceSet::checkRangesCreaseAngle(const SFFloat &value,
                                            const std::string &nodeType,
                                            const std::string &defName,
                                            std::vector<RangeDiagnostic> &out) {
  if (value < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "creaseAngle",
                                  "creaseAngle below minimum of 0"});
}

void IndexedFaceSet::checkRangesNormalIndex(const MFInt32 &value,
                                            const std::string &nodeType,
                                            const std::string &defName,
                                            std::vector<RangeDiagnostic> &out) {
  for (const auto &v : value) {

    if (v < -1)
      out.push_back(RangeDiagnostic{nodeType, defName, "normalIndex",
                                    "normalIndex below minimum of -1"});
  }
}

void IndexedFaceSet::checkRangesTexCoordIndex(
    const MFInt32 &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  for (const auto &v : value) {

    if (v < -1)
      out.push_back(RangeDiagnostic{nodeType, defName, "texCoordIndex",
                                    "texCoordIndex below minimum of -1"});
  }
}

namespace factory_detail {
std::shared_ptr<X3DNode> createIndexedFaceSet() {
  return std::make_shared<IndexedFaceSet>();
}
} // namespace factory_detail

} // namespace x3d::nodes
