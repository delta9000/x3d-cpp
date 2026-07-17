// Extrusion.cpp
#include "x3d/nodes/Extrusion.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string Extrusion::nodeTypeName() const { return "Extrusion"; }

std::string Extrusion::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &Extrusion::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "beginCap", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Extrusion &>(n).getBeginCap());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Extrusion &>(n).setBeginCapUnchecked(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "ccw", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Extrusion &>(n).getCcw());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Extrusion &>(n).setCcwUnchecked(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "convex", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Extrusion &>(n).getConvex());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Extrusion &>(n).setConvexUnchecked(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "creaseAngle", X3DFieldType::SFFloat, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Extrusion &>(n).getCreaseAngle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Extrusion &>(n).setCreaseAngleUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "crossSection", X3DFieldType::MFVec2f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Extrusion &>(n).getCrossSection());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Extrusion &>(n).setCrossSectionUnchecked(
              std::any_cast<MFVec2f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "endCap", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Extrusion &>(n).getEndCap());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Extrusion &>(n).setEndCapUnchecked(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Extrusion &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Extrusion &>(n).X3DNode::setIS(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Extrusion &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Extrusion &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "orientation", X3DFieldType::MFRotation, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Extrusion &>(n).getOrientation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Extrusion &>(n).setOrientationUnchecked(
              std::any_cast<MFRotation>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "scale", X3DFieldType::MFVec2f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Extrusion &>(n).getScale());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Extrusion &>(n).setScaleUnchecked(
              std::any_cast<MFVec2f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{"set_crossSection", X3DFieldType::MFVec2f,
                          AccessType::InputOnly, "",

                          nullptr,

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<Extrusion &>(n).onSet_crossSection(
                                std::any_cast<MFVec2f>(v));
                          },

                          nullptr, nullptr

    });

    t.push_back(FieldInfo{"set_orientation", X3DFieldType::MFRotation,
                          AccessType::InputOnly, "",

                          nullptr,

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<Extrusion &>(n).onSet_orientation(
                                std::any_cast<MFRotation>(v));
                          },

                          nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "set_scale", X3DFieldType::MFVec2f, AccessType::InputOnly, "",

        nullptr,

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Extrusion &>(n).onSet_scale(std::any_cast<MFVec2f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "set_spine", X3DFieldType::MFVec3f, AccessType::InputOnly, "",

        nullptr,

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Extrusion &>(n).onSet_spine(std::any_cast<MFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "solid", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Extrusion &>(n).getSolid());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Extrusion &>(n).setSolidUnchecked(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "spine", X3DFieldType::MFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Extrusion &>(n).getSpine());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Extrusion &>(n).setSpineUnchecked(
              std::any_cast<MFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Extrusion &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Extrusion &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Extrusion &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Extrusion &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Extrusion &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Extrusion &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Extrusion &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Extrusion &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"style", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const Extrusion &>(n).X3DNode::getStyle());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<Extrusion &>(n).X3DNode::setStyle(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    return t;
  }();
  return table;
}

void Extrusion::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void Extrusion::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesCreaseAngle(getCreaseAngle(), nodeTypeName(), "", out);
}

void Extrusion::checkRangesCreaseAngle(const SFFloat &value,
                                       const std::string &nodeType,
                                       const std::string &defName,
                                       std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "creaseAngle",
                                  "creaseAngle below minimum of 0"});
}

namespace factory_detail {
std::shared_ptr<X3DNode> createExtrusion() {
  return std::make_shared<Extrusion>();
}
} // namespace factory_detail

} // namespace x3d::nodes
