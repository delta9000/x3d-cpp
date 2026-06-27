// NurbsSet.cpp
#include "x3d/nodes/NurbsSet.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string NurbsSet::nodeTypeName() const { return "NurbsSet"; }

std::string NurbsSet::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &NurbsSet::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{"addGeometry", X3DFieldType::MFNode,
                          AccessType::InputOnly, "addGeometry",

                          nullptr,

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<NurbsSet &>(n).onAddGeometry(
                                std::any_cast<MFNode>(v));
                          },

                          nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxCenter", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsSet &>(n)
                              .X3DBoundedObject::getBboxCenter());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSet &>(n).X3DBoundedObject::setBboxCenterUnchecked(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxDisplay", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsSet &>(n)
                              .X3DBoundedObject::getBboxDisplay());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSet &>(n).X3DBoundedObject::setBboxDisplay(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bboxSize", X3DFieldType::SFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsSet &>(n)
                              .X3DBoundedObject::getBboxSize());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSet &>(n).X3DBoundedObject::setBboxSizeUnchecked(
              std::any_cast<SFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "geometry", X3DFieldType::MFNode, AccessType::InputOutput, "geometry",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsSet &>(n).getGeometry());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSet &>(n).setGeometry(std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsSet &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSet &>(n).X3DNode::setIS(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NurbsSet &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSet &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{"removeGeometry", X3DFieldType::MFNode,
                          AccessType::InputOnly, "removeGeometry",

                          nullptr,

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<NurbsSet &>(n).onRemoveGeometry(
                                std::any_cast<MFNode>(v));
                          },

                          nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "tessellationScale", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NurbsSet &>(n).getTessellationScale());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSet &>(n).setTessellationScale(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "visible", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NurbsSet &>(n).X3DBoundedObject::getVisible());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSet &>(n).X3DBoundedObject::setVisible(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsSet &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSet &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsSet &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSet &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"class", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const NurbsSet &>(n).X3DNode::getClass_());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<NurbsSet &>(n).X3DNode::setClass_(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsSet &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSet &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"style", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const NurbsSet &>(n).X3DNode::getStyle());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<NurbsSet &>(n).X3DNode::setStyle(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    return t;
  }();
  return table;
}

void NurbsSet::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

} // namespace x3d::nodes
