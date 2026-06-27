// ChannelSelector.cpp
#include "x3d/nodes/ChannelSelector.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string ChannelSelector::nodeTypeName() const { return "ChannelSelector"; }

std::string ChannelSelector::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &ChannelSelector::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "channelCount", X3DFieldType::SFInt32, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ChannelSelector &>(n)
                              .X3DSoundChannelNode::getChannelCount());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ChannelSelector &>(n)
              .X3DSoundChannelNode::emitChannelCount(std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "channelCountMode", X3DFieldType::SFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ChannelSelector &>(n)
                              .X3DSoundChannelNode::getChannelCountMode());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ChannelSelector &>(n)
              .X3DSoundChannelNode::setChannelCountMode(
                  std::any_cast<ChannelCountModeChoices>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(dynamic_cast<const ChannelSelector &>(n)
                               .X3DSoundChannelNode::getChannelCountMode());
        },

        [](X3DNode &n, const std::string &s) {
          ChannelCountModeChoices ev;
          if (from_string(s, ev))
            dynamic_cast<ChannelSelector &>(n)
                .X3DSoundChannelNode::setChannelCountMode(ev);
        }

    });

    t.push_back(FieldInfo{
        "channelInterpretation", X3DFieldType::SFEnum, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ChannelSelector &>(n)
                              .X3DSoundChannelNode::getChannelInterpretation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ChannelSelector &>(n)
              .X3DSoundChannelNode::setChannelInterpretation(
                  std::any_cast<ChannelInterpretationChoices>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(
              dynamic_cast<const ChannelSelector &>(n)
                  .X3DSoundChannelNode::getChannelInterpretation());
        },

        [](X3DNode &n, const std::string &s) {
          ChannelInterpretationChoices ev;
          if (from_string(s, ev))
            dynamic_cast<ChannelSelector &>(n)
                .X3DSoundChannelNode::setChannelInterpretation(ev);
        }

    });

    t.push_back(FieldInfo{
        "channelSelection", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ChannelSelector &>(n).getChannelSelection());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ChannelSelector &>(n).setChannelSelectionUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "children", X3DFieldType::MFNode, AccessType::InputOutput, "children",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ChannelSelector &>(n).getChildren());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ChannelSelector &>(n).setChildren(
              std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ChannelSelector &>(n)
                              .X3DSoundNode::getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ChannelSelector &>(n).X3DSoundNode::setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const ChannelSelector &>(n)
                                        .X3DSoundNode::getEnabled());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<ChannelSelector &>(n).X3DSoundNode::setEnabled(
                        std::any_cast<SFBool>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "gain", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ChannelSelector &>(n)
                              .X3DSoundChannelNode::getGain());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ChannelSelector &>(n).X3DSoundChannelNode::setGain(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ChannelSelector &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ChannelSelector &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ChannelSelector &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ChannelSelector &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ChannelSelector &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ChannelSelector &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ChannelSelector &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ChannelSelector &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ChannelSelector &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ChannelSelector &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ChannelSelector &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ChannelSelector &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ChannelSelector &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ChannelSelector &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void ChannelSelector::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void ChannelSelector::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesChannelSelection(getChannelSelection(), nodeTypeName(), "", out);
}

void ChannelSelector::checkRangesChannelSelection(
    const SFInt32 &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "channelSelection",
                                  "channelSelection below minimum of 0"});
}

} // namespace x3d::nodes
