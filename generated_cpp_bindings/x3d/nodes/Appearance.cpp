// Appearance.cpp
#include "x3d/nodes/Appearance.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string Appearance::nodeTypeName() const { return "Appearance"; }

std::string Appearance::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &Appearance::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "acousticProperties", X3DFieldType::SFNode, AccessType::InputOutput,
        "acousticProperties",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Appearance &>(n).getAcousticProperties());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Appearance &>(n).setAcousticProperties(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "alphaCutoff", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Appearance &>(n).getAlphaCutoff());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Appearance &>(n).setAlphaCutoffUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "alphaMode", X3DFieldType::SFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Appearance &>(n).getAlphaMode());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Appearance &>(n).setAlphaMode(
              std::any_cast<AlphaModeChoices>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(dynamic_cast<const Appearance &>(n).getAlphaMode());
        },

        [](X3DNode &n, const std::string &s) {
          AlphaModeChoices ev;
          if (from_string(s, ev))
            dynamic_cast<Appearance &>(n).setAlphaMode(ev);
        }

    });

    t.push_back(
        FieldInfo{"backMaterial", X3DFieldType::SFNode, AccessType::InputOutput,
                  "backMaterial",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const Appearance &>(n).getBackMaterial());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<Appearance &>(n).setBackMaterial(
                        std::any_cast<SFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "fillProperties", X3DFieldType::SFNode, AccessType::InputOutput,
        "fillProperties",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Appearance &>(n).getFillProperties());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Appearance &>(n).setFillProperties(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Appearance &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Appearance &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "lineProperties", X3DFieldType::SFNode, AccessType::InputOutput,
        "lineProperties",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Appearance &>(n).getLineProperties());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Appearance &>(n).setLineProperties(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "material", X3DFieldType::SFNode, AccessType::InputOutput, "material",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Appearance &>(n).getMaterial());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Appearance &>(n).setMaterial(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Appearance &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Appearance &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "pointProperties", X3DFieldType::SFNode, AccessType::InputOutput,
        "pointProperties",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Appearance &>(n).getPointProperties());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Appearance &>(n).setPointProperties(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "shaders", X3DFieldType::MFNode, AccessType::InputOutput, "shaders",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Appearance &>(n).getShaders());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Appearance &>(n).setShaders(std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "texture", X3DFieldType::SFNode, AccessType::InputOutput, "texture",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Appearance &>(n).getTexture());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Appearance &>(n).setTexture(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "textureTransform", X3DFieldType::SFNode, AccessType::InputOutput,
        "textureTransform",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Appearance &>(n).getTextureTransform());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Appearance &>(n).setTextureTransform(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const Appearance &>(n).X3DNode::getDEF());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<Appearance &>(n).X3DNode::setDEF(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(
        FieldInfo{"USE", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const Appearance &>(n).X3DNode::getUSE());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<Appearance &>(n).X3DNode::setUSE(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Appearance &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Appearance &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Appearance &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Appearance &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Appearance &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Appearance &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void Appearance::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void Appearance::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesAlphaCutoff(getAlphaCutoff(), nodeTypeName(), "", out);
}

void Appearance::checkRangesAlphaCutoff(const SFFloat &value,
                                        const std::string &nodeType,
                                        const std::string &defName,
                                        std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "alphaCutoff",
                                  "alphaCutoff below minimum of 0"});
  if (value > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "alphaCutoff",
                                  "alphaCutoff above maximum of 1"});
}

} // namespace x3d::nodes
