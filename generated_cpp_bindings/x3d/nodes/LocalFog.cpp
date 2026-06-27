// LocalFog.cpp
#include "x3d/nodes/LocalFog.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string LocalFog::nodeTypeName() const { return "LocalFog"; }

std::string LocalFog::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &LocalFog::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "color", X3DFieldType::SFColor, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const LocalFog &>(n).X3DFogObject::getColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<LocalFog &>(n).X3DFogObject::setColorUnchecked(
              std::any_cast<SFColor>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const LocalFog &>(n).getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<LocalFog &>(n).setEnabled(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "fogType", X3DFieldType::SFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const LocalFog &>(n).X3DFogObject::getFogType());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<LocalFog &>(n).X3DFogObject::setFogType(
              std::any_cast<FogTypeChoices>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(
              dynamic_cast<const LocalFog &>(n).X3DFogObject::getFogType());
        },

        [](X3DNode &n, const std::string &s) {
          FogTypeChoices ev;
          if (from_string(s, ev))
            dynamic_cast<LocalFog &>(n).X3DFogObject::setFogType(ev);
        }

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const LocalFog &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<LocalFog &>(n).X3DNode::setIS(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const LocalFog &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<LocalFog &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "visibilityRange", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const LocalFog &>(n)
                              .X3DFogObject::getVisibilityRange());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<LocalFog &>(n).X3DFogObject::setVisibilityRangeUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const LocalFog &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<LocalFog &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const LocalFog &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<LocalFog &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"class", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const LocalFog &>(n).X3DNode::getClass_());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<LocalFog &>(n).X3DNode::setClass_(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const LocalFog &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<LocalFog &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"style", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const LocalFog &>(n).X3DNode::getStyle());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<LocalFog &>(n).X3DNode::setStyle(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    return t;
  }();
  return table;
}

void LocalFog::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void LocalFog::validateRanges(std::vector<RangeDiagnostic> &out) const {

  X3DFogObject::checkRangesColor(X3DFogObject::getColor(), nodeTypeName(), "",
                                 out);

  X3DFogObject::checkRangesVisibilityRange(X3DFogObject::getVisibilityRange(),
                                           nodeTypeName(), "", out);
}

} // namespace x3d::nodes
