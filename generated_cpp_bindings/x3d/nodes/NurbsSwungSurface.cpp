// NurbsSwungSurface.cpp
#include "x3d/nodes/NurbsSwungSurface.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string NurbsSwungSurface::nodeTypeName() const {
  return "NurbsSwungSurface";
}

std::string NurbsSwungSurface::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &NurbsSwungSurface::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "ccw", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsSwungSurface &>(n).getCcw());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSwungSurface &>(n).setCcwUnchecked(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NurbsSwungSurface &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSwungSurface &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NurbsSwungSurface &>(n)
                              .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSwungSurface &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "profileCurve", X3DFieldType::SFNode, AccessType::InputOutput,
        "profileCurve",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NurbsSwungSurface &>(n).getProfileCurve());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSwungSurface &>(n).setProfileCurve(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"solid", X3DFieldType::SFBool, AccessType::InitializeOnly, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const NurbsSwungSurface &>(n).getSolid());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<NurbsSwungSurface &>(n).setSolidUnchecked(
                        std::any_cast<SFBool>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "trajectoryCurve", X3DFieldType::SFNode, AccessType::InputOutput,
        "trajectoryCurve",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NurbsSwungSurface &>(n).getTrajectoryCurve());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSwungSurface &>(n).setTrajectoryCurve(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NurbsSwungSurface &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSwungSurface &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NurbsSwungSurface &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSwungSurface &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NurbsSwungSurface &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSwungSurface &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NurbsSwungSurface &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSwungSurface &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NurbsSwungSurface &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NurbsSwungSurface &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void NurbsSwungSurface::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

namespace factory_detail {
std::shared_ptr<X3DNode> createNurbsSwungSurface() {
  return std::make_shared<NurbsSwungSurface>();
}
} // namespace factory_detail

} // namespace x3d::nodes
