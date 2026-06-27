// ClipPlane.cpp
#include "x3d/nodes/ClipPlane.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string ClipPlane::nodeTypeName() const { return "ClipPlane"; }

std::string ClipPlane::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &ClipPlane::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ClipPlane &>(n).getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ClipPlane &>(n).setEnabled(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ClipPlane &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ClipPlane &>(n).X3DNode::setIS(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ClipPlane &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ClipPlane &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "plane", X3DFieldType::SFVec4f, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ClipPlane &>(n).getPlane());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ClipPlane &>(n).setPlaneUnchecked(
              std::any_cast<SFVec4f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ClipPlane &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ClipPlane &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ClipPlane &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ClipPlane &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ClipPlane &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ClipPlane &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ClipPlane &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ClipPlane &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"style", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const ClipPlane &>(n).X3DNode::getStyle());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<ClipPlane &>(n).X3DNode::setStyle(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    return t;
  }();
  return table;
}

void ClipPlane::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void ClipPlane::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesPlane(getPlane(), nodeTypeName(), "", out);
}

void ClipPlane::checkRangesPlane(const SFVec4f &value,
                                 const std::string &nodeType,
                                 const std::string &defName,
                                 std::vector<RangeDiagnostic> &out) {
  if (value.x < -1)
    out.push_back(RangeDiagnostic{nodeType, defName, "plane",
                                  "plane.x below minimum of -1"});
  if (value.x > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "plane",
                                  "plane.x above maximum of 1"});

  if (value.y < -1)
    out.push_back(RangeDiagnostic{nodeType, defName, "plane",
                                  "plane.y below minimum of -1"});
  if (value.y > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "plane",
                                  "plane.y above maximum of 1"});

  if (value.z < -1)
    out.push_back(RangeDiagnostic{nodeType, defName, "plane",
                                  "plane.z below minimum of -1"});
  if (value.z > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "plane",
                                  "plane.z above maximum of 1"});

  if (value.w < -1)
    out.push_back(RangeDiagnostic{nodeType, defName, "plane",
                                  "plane.w below minimum of -1"});
  if (value.w > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "plane",
                                  "plane.w above maximum of 1"});
}

} // namespace x3d::nodes
