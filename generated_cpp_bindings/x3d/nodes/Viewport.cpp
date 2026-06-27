// Viewport.cpp
#include "x3d/nodes/Viewport.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string Viewport::nodeTypeName() const { return "Viewport"; }

std::string Viewport::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &Viewport::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(
        FieldInfo{"addChildren", X3DFieldType::MFNode, AccessType::InputOnly,
                  "addChildren",

                  nullptr,

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<Viewport &>(n).X3DGroupingNode::onAddChildren(
                        std::any_cast<MFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "bboxCenter", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Viewport &>(n)
                              .X3DGroupingNode::getBboxCenter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Viewport &>(n).X3DGroupingNode::setBboxCenterUnchecked(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxDisplay", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Viewport &>(n)
                              .X3DGroupingNode::getBboxDisplay());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Viewport &>(n).X3DGroupingNode::setBboxDisplay(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxSize", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Viewport &>(n).X3DGroupingNode::getBboxSize());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Viewport &>(n).X3DGroupingNode::setBboxSizeUnchecked(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "children", X3DFieldType::MFNode, AccessType::InputOutput, "children",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Viewport &>(n).X3DGroupingNode::getChildren());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Viewport &>(n).X3DGroupingNode::setChildren(
              std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "clipBoundary", X3DFieldType::MFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Viewport &>(n).getClipBoundary());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Viewport &>(n).setClipBoundaryUnchecked(
              std::any_cast<MFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Viewport &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Viewport &>(n).X3DNode::setIS(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Viewport &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Viewport &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "removeChildren", X3DFieldType::MFNode, AccessType::InputOnly,
        "removeChildren",

        nullptr,

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Viewport &>(n).X3DGroupingNode::onRemoveChildren(
              std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "visible", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Viewport &>(n).X3DGroupingNode::getVisible());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Viewport &>(n).X3DGroupingNode::setVisible(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Viewport &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Viewport &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Viewport &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Viewport &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"class", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const Viewport &>(n).X3DNode::getClass_());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<Viewport &>(n).X3DNode::setClass_(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Viewport &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Viewport &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"style", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const Viewport &>(n).X3DNode::getStyle());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<Viewport &>(n).X3DNode::setStyle(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    return t;
  }();
  return table;
}

void Viewport::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void Viewport::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesClipBoundary(getClipBoundary(), nodeTypeName(), "", out);
}

void Viewport::checkRangesClipBoundary(const MFFloat &value,
                                       const std::string &nodeType,
                                       const std::string &defName,
                                       std::vector<RangeDiagnostic> &out) {
  for (const auto &v : value) {

    if (v < 0)
      out.push_back(RangeDiagnostic{nodeType, defName, "clipBoundary",
                                    "clipBoundary below minimum of 0"});
    if (v > 1)
      out.push_back(RangeDiagnostic{nodeType, defName, "clipBoundary",
                                    "clipBoundary above maximum of 1"});
  }
}

} // namespace x3d::nodes
