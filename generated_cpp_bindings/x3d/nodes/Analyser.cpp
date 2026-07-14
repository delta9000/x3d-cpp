// Analyser.cpp
#include "x3d/nodes/Analyser.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string Analyser::nodeTypeName() const { return "Analyser"; }

std::string Analyser::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &Analyser::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "channelCount", X3DFieldType::SFInt32, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Analyser &>(n)
                              .X3DSoundProcessingNode::getChannelCount());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Analyser &>(n).X3DSoundProcessingNode::emitChannelCount(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "channelCountMode", X3DFieldType::SFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Analyser &>(n)
                              .X3DSoundProcessingNode::getChannelCountMode());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Analyser &>(n)
              .X3DSoundProcessingNode::setChannelCountMode(
                  std::any_cast<ChannelCountModeChoices>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(dynamic_cast<const Analyser &>(n)
                               .X3DSoundProcessingNode::getChannelCountMode());
        },

        [](X3DNode &n, const std::string &s) {
          ChannelCountModeChoices ev;
          if (from_string(s, ev))
            dynamic_cast<Analyser &>(n)
                .X3DSoundProcessingNode::setChannelCountMode(ev);
        }

    });

    t.push_back(FieldInfo{
        "channelInterpretation", X3DFieldType::SFEnum, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Analyser &>(n)
                  .X3DSoundProcessingNode::getChannelInterpretation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Analyser &>(n)
              .X3DSoundProcessingNode::setChannelInterpretation(
                  std::any_cast<ChannelInterpretationChoices>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(
              dynamic_cast<const Analyser &>(n)
                  .X3DSoundProcessingNode::getChannelInterpretation());
        },

        [](X3DNode &n, const std::string &s) {
          ChannelInterpretationChoices ev;
          if (from_string(s, ev))
            dynamic_cast<Analyser &>(n)
                .X3DSoundProcessingNode::setChannelInterpretation(ev);
        }

    });

    t.push_back(FieldInfo{
        "children", X3DFieldType::MFNode, AccessType::InputOutput, "children",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Analyser &>(n).getChildren());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Analyser &>(n).setChildren(std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Analyser &>(n)
                              .X3DTimeDependentNode::getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Analyser &>(n).X3DTimeDependentNode::setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "elapsedTime", X3DFieldType::SFTime, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Analyser &>(n)
                              .X3DTimeDependentNode::getElapsedTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Analyser &>(n).X3DTimeDependentNode::emitElapsedTime(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Analyser &>(n)
                              .X3DSoundProcessingNode::getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Analyser &>(n).X3DSoundProcessingNode::setEnabled(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "fftSize", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Analyser &>(n).getFftSize());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Analyser &>(n).setFftSizeUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "frequencyBinCount", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Analyser &>(n).getFrequencyBinCount());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Analyser &>(n).setFrequencyBinCountUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"gain", X3DFieldType::SFFloat, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const Analyser &>(n)
                                        .X3DSoundProcessingNode::getGain());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<Analyser &>(n).X3DSoundProcessingNode::setGain(
                        std::any_cast<SFFloat>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Analyser &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Analyser &>(n).X3DNode::setIS(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isActive", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Analyser &>(n)
                              .X3DTimeDependentNode::getIsActive());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Analyser &>(n).X3DTimeDependentNode::emitIsActive(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isPaused", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Analyser &>(n)
                              .X3DTimeDependentNode::getIsPaused());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Analyser &>(n).X3DTimeDependentNode::emitIsPaused(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "maxDecibels", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Analyser &>(n).getMaxDecibels());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Analyser &>(n).setMaxDecibels(std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Analyser &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Analyser &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "minDecibels", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Analyser &>(n).getMinDecibels());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Analyser &>(n).setMinDecibels(std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "pauseTime", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Analyser &>(n)
                              .X3DTimeDependentNode::getPauseTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Analyser &>(n).X3DTimeDependentNode::setPauseTime(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "resumeTime", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Analyser &>(n)
                              .X3DTimeDependentNode::getResumeTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Analyser &>(n).X3DTimeDependentNode::setResumeTime(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "smoothingTimeConstant", X3DFieldType::SFFloat, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const Analyser &>(n).getSmoothingTimeConstant());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Analyser &>(n).setSmoothingTimeConstantUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "startTime", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Analyser &>(n)
                              .X3DTimeDependentNode::getStartTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Analyser &>(n).X3DTimeDependentNode::setStartTime(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "stopTime", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Analyser &>(n)
                              .X3DTimeDependentNode::getStopTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Analyser &>(n).X3DTimeDependentNode::setStopTime(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"tailTime", X3DFieldType::SFTime, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const Analyser &>(n)
                                        .X3DSoundProcessingNode::getTailTime());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<Analyser &>(n)
                        .X3DSoundProcessingNode::setTailTimeUnchecked(
                            std::any_cast<SFTime>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Analyser &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Analyser &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Analyser &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Analyser &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"class", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const Analyser &>(n).X3DNode::getClass_());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<Analyser &>(n).X3DNode::setClass_(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const Analyser &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<Analyser &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"style", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const Analyser &>(n).X3DNode::getStyle());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<Analyser &>(n).X3DNode::setStyle(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    return t;
  }();
  return table;
}

void Analyser::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void Analyser::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesFftSize(getFftSize(), nodeTypeName(), "", out);

  checkRangesFrequencyBinCount(getFrequencyBinCount(), nodeTypeName(), "", out);

  checkRangesSmoothingTimeConstant(getSmoothingTimeConstant(), nodeTypeName(),
                                   "", out);

  X3DSoundProcessingNode::checkRangesTailTime(
      X3DSoundProcessingNode::getTailTime(), nodeTypeName(), "", out);
}

void Analyser::checkRangesFftSize(const SFInt32 &value,
                                  const std::string &nodeType,
                                  const std::string &defName,
                                  std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "fftSize",
                                  "fftSize below minimum of 0"});
}

void Analyser::checkRangesFrequencyBinCount(const SFInt32 &value,
                                            const std::string &nodeType,
                                            const std::string &defName,
                                            std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "frequencyBinCount",
                                  "frequencyBinCount below minimum of 0"});
}

void Analyser::checkRangesSmoothingTimeConstant(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "smoothingTimeConstant",
                                  "smoothingTimeConstant below minimum of 0"});
}

namespace factory_detail {
std::shared_ptr<X3DNode> createAnalyser() {
  return std::make_shared<Analyser>();
}
} // namespace factory_detail

} // namespace x3d::nodes
