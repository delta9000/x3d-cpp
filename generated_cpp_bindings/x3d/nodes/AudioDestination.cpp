// AudioDestination.cpp
#include "x3d/nodes/AudioDestination.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string AudioDestination::nodeTypeName() const {
  return "AudioDestination";
}

std::string AudioDestination::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &AudioDestination::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "channelCount", X3DFieldType::SFInt32, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const AudioDestination &>(n)
                              .X3DSoundDestinationNode::getChannelCount());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<AudioDestination &>(n)
              .X3DSoundDestinationNode::emitChannelCount(
                  std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "channelCountMode", X3DFieldType::SFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const AudioDestination &>(n)
                              .X3DSoundDestinationNode::getChannelCountMode());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<AudioDestination &>(n)
              .X3DSoundDestinationNode::setChannelCountMode(
                  std::any_cast<ChannelCountModeChoices>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(dynamic_cast<const AudioDestination &>(n)
                               .X3DSoundDestinationNode::getChannelCountMode());
        },

        [](X3DNode &n, const std::string &s) {
          ChannelCountModeChoices ev;
          if (from_string(s, ev))
            dynamic_cast<AudioDestination &>(n)
                .X3DSoundDestinationNode::setChannelCountMode(ev);
        }

    });

    t.push_back(FieldInfo{
        "channelInterpretation", X3DFieldType::SFEnum, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const AudioDestination &>(n)
                  .X3DSoundDestinationNode::getChannelInterpretation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<AudioDestination &>(n)
              .X3DSoundDestinationNode::setChannelInterpretation(
                  std::any_cast<ChannelInterpretationChoices>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(
              dynamic_cast<const AudioDestination &>(n)
                  .X3DSoundDestinationNode::getChannelInterpretation());
        },

        [](X3DNode &n, const std::string &s) {
          ChannelInterpretationChoices ev;
          if (from_string(s, ev))
            dynamic_cast<AudioDestination &>(n)
                .X3DSoundDestinationNode::setChannelInterpretation(ev);
        }

    });

    t.push_back(FieldInfo{
        "children", X3DFieldType::MFNode, AccessType::InputOutput, "children",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const AudioDestination &>(n).getChildren());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<AudioDestination &>(n).setChildren(
              std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const AudioDestination &>(n)
                              .X3DSoundNode::getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<AudioDestination &>(n).X3DSoundNode::setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const AudioDestination &>(n)
                              .X3DSoundNode::getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<AudioDestination &>(n).X3DSoundNode::setEnabled(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "gain", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const AudioDestination &>(n)
                              .X3DSoundDestinationNode::getGain());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<AudioDestination &>(n).X3DSoundDestinationNode::setGain(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const AudioDestination &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<AudioDestination &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isActive", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const AudioDestination &>(n)
                              .X3DSoundDestinationNode::getIsActive());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<AudioDestination &>(n)
              .X3DSoundDestinationNode::emitIsActive(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "maxChannelCount", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const AudioDestination &>(n).getMaxChannelCount());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<AudioDestination &>(n).setMaxChannelCountUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "mediaDeviceID", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const AudioDestination &>(n)
                              .X3DSoundDestinationNode::getMediaDeviceID());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<AudioDestination &>(n)
              .X3DSoundDestinationNode::setMediaDeviceID(
                  std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const AudioDestination &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<AudioDestination &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const AudioDestination &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<AudioDestination &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const AudioDestination &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<AudioDestination &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const AudioDestination &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<AudioDestination &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const AudioDestination &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<AudioDestination &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const AudioDestination &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<AudioDestination &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void AudioDestination::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void AudioDestination::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesMaxChannelCount(getMaxChannelCount(), nodeTypeName(), "", out);
}

void AudioDestination::checkRangesMaxChannelCount(
    const SFInt32 &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "maxChannelCount",
                                  "maxChannelCount below minimum of 0"});
}

namespace factory_detail {
std::shared_ptr<X3DNode> createAudioDestination() {
  return std::make_shared<AudioDestination>();
}
} // namespace factory_detail

} // namespace x3d::nodes
