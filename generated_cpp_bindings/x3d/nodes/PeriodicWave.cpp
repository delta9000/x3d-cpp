// PeriodicWave.cpp
#include "x3d/nodes/PeriodicWave.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string PeriodicWave::nodeTypeName() const { return "PeriodicWave"; }

std::string PeriodicWave::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &PeriodicWave::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PeriodicWave &>(n)
                              .X3DSoundNode::getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PeriodicWave &>(n).X3DSoundNode::setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const PeriodicWave &>(n)
                       .X3DSoundNode::getDescription(),
                  &typeid(SFString)};
        }

    });

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PeriodicWave &>(n).X3DSoundNode::getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PeriodicWave &>(n).X3DSoundNode::setEnabled(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {
              &dynamic_cast<const PeriodicWave &>(n).X3DSoundNode::getEnabled(),
              &typeid(SFBool)};
        }

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PeriodicWave &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PeriodicWave &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const PeriodicWave &>(n).X3DNode::getIS(),
                  &typeid(SFNode)};
        }

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PeriodicWave &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PeriodicWave &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const PeriodicWave &>(n).X3DNode::getMetadata(),
                  &typeid(SFNode)};
        }

    });

    t.push_back(FieldInfo{
        "optionsImag", X3DFieldType::MFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PeriodicWave &>(n).getOptionsImag());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PeriodicWave &>(n).setOptionsImag(
              std::any_cast<MFFloat>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const PeriodicWave &>(n).getOptionsImag(),
                  &typeid(MFFloat)};
        }

    });

    t.push_back(FieldInfo{
        "optionsReal", X3DFieldType::MFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PeriodicWave &>(n).getOptionsReal());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PeriodicWave &>(n).setOptionsReal(
              std::any_cast<MFFloat>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const PeriodicWave &>(n).getOptionsReal(),
                  &typeid(MFFloat)};
        }

    });

    t.push_back(FieldInfo{
        "type", X3DFieldType::SFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const PeriodicWave &>(n).getType());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PeriodicWave &>(n).setType(
              std::any_cast<PeriodicWaveTypeChoices>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(dynamic_cast<const PeriodicWave &>(n).getType());
        },

        [](X3DNode &n, const std::string &s) {
          PeriodicWaveTypeChoices ev;
          if (from_string(s, ev))
            dynamic_cast<PeriodicWave &>(n).setType(ev);
        }

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const PeriodicWave &>(n).getType(),
                  &typeid(PeriodicWaveTypeChoices)};
        }

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PeriodicWave &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PeriodicWave &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const PeriodicWave &>(n).X3DNode::getDEF(),
                  &typeid(SFString)};
        }

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PeriodicWave &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PeriodicWave &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const PeriodicWave &>(n).X3DNode::getUSE(),
                  &typeid(SFString)};
        }

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PeriodicWave &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PeriodicWave &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const PeriodicWave &>(n).X3DNode::getClass_(),
                  &typeid(SFString)};
        }

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PeriodicWave &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PeriodicWave &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const PeriodicWave &>(n).X3DNode::getId(),
                  &typeid(SFString)};
        }

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const PeriodicWave &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<PeriodicWave &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const PeriodicWave &>(n).X3DNode::getStyle(),
                  &typeid(SFString)};
        }

    });

    return t;
  }();
  return table;
}

void PeriodicWave::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

namespace factory_detail {
std::shared_ptr<X3DNode> createPeriodicWave() {
  return std::make_shared<PeriodicWave>();
}
} // namespace factory_detail

} // namespace x3d::nodes
