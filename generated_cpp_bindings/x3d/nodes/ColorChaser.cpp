// ColorChaser.cpp
#include "x3d/nodes/ColorChaser.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string ColorChaser::nodeTypeName() const { return "ColorChaser"; }

std::string ColorChaser::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &ColorChaser::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "duration", X3DFieldType::SFTime, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ColorChaser &>(n)
                              .X3DChaserNode::getDuration());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ColorChaser &>(n).X3DChaserNode::setDurationUnchecked(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "initialDestination", X3DFieldType::SFColor, AccessType::InitializeOnly,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ColorChaser &>(n).getInitialDestination());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ColorChaser &>(n).setInitialDestinationUnchecked(
              std::any_cast<SFColor>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "initialValue", X3DFieldType::SFColor, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ColorChaser &>(n).getInitialValue());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ColorChaser &>(n).setInitialValueUnchecked(
              std::any_cast<SFColor>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const ColorChaser &>(n).X3DNode::getIS());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<ColorChaser &>(n).X3DNode::setIS(
                        std::any_cast<SFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "isActive", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ColorChaser &>(n)
                              .X3DFollowerNode::getIsActive());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ColorChaser &>(n).X3DFollowerNode::emitIsActive(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ColorChaser &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ColorChaser &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{"set_destination", X3DFieldType::SFColor,
                          AccessType::InputOnly, "",

                          nullptr,

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<ColorChaser &>(n).onSet_destination(
                                std::any_cast<SFColor>(v));
                          },

                          nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "set_value", X3DFieldType::SFColor, AccessType::InputOnly, "",

        nullptr,

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ColorChaser &>(n).onSet_value(std::any_cast<SFColor>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "value_changed", X3DFieldType::SFColor, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ColorChaser &>(n).getValue_changed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ColorChaser &>(n).emitValue_changed(
              std::any_cast<SFColor>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const ColorChaser &>(n).X3DNode::getDEF());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<ColorChaser &>(n).X3DNode::setDEF(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(
        FieldInfo{"USE", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const ColorChaser &>(n).X3DNode::getUSE());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<ColorChaser &>(n).X3DNode::setUSE(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ColorChaser &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ColorChaser &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"id", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const ColorChaser &>(n).X3DNode::getId());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<ColorChaser &>(n).X3DNode::setId(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ColorChaser &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ColorChaser &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void ColorChaser::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void ColorChaser::validateRanges(std::vector<RangeDiagnostic> &out) const {

  X3DChaserNode::checkRangesDuration(X3DChaserNode::getDuration(),
                                     nodeTypeName(), "", out);

  checkRangesInitialDestination(getInitialDestination(), nodeTypeName(), "",
                                out);

  checkRangesInitialValue(getInitialValue(), nodeTypeName(), "", out);
}

void ColorChaser::checkRangesInitialDestination(
    const SFColor &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value.r < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "initialDestination",
                                  "initialDestination.r below minimum of 0"});
  if (value.r > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "initialDestination",
                                  "initialDestination.r above maximum of 1"});

  if (value.g < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "initialDestination",
                                  "initialDestination.g below minimum of 0"});
  if (value.g > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "initialDestination",
                                  "initialDestination.g above maximum of 1"});

  if (value.b < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "initialDestination",
                                  "initialDestination.b below minimum of 0"});
  if (value.b > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "initialDestination",
                                  "initialDestination.b above maximum of 1"});
}

void ColorChaser::checkRangesInitialValue(const SFColor &value,
                                          const std::string &nodeType,
                                          const std::string &defName,
                                          std::vector<RangeDiagnostic> &out) {
  if (value.r < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "initialValue",
                                  "initialValue.r below minimum of 0"});
  if (value.r > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "initialValue",
                                  "initialValue.r above maximum of 1"});

  if (value.g < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "initialValue",
                                  "initialValue.g below minimum of 0"});
  if (value.g > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "initialValue",
                                  "initialValue.g above maximum of 1"});

  if (value.b < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "initialValue",
                                  "initialValue.b below minimum of 0"});
  if (value.b > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "initialValue",
                                  "initialValue.b above maximum of 1"});
}

namespace factory_detail {
std::shared_ptr<X3DNode> createColorChaser() {
  return std::make_shared<ColorChaser>();
}
} // namespace factory_detail

} // namespace x3d::nodes
