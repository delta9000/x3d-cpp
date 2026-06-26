// BufferAudioSource.cpp
#include "BufferAudioSource.hpp"

std::string BufferAudioSource::nodeTypeName() const {
  return "BufferAudioSource";
}

std::string BufferAudioSource::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &BufferAudioSource::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "autoRefresh", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const BufferAudioSource &>(n)
                              .X3DUrlObject::getAutoRefresh());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BufferAudioSource &>(n)
              .X3DUrlObject::setAutoRefreshUnchecked(std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{"autoRefreshTimeLimit", X3DFieldType::SFTime,
                          AccessType::InputOutput, "",

                          [](const X3DNode &n) -> std::any {
                            return std::any(
                                dynamic_cast<const BufferAudioSource &>(n)
                                    .X3DUrlObject::getAutoRefreshTimeLimit());
                          },

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<BufferAudioSource &>(n)
                                .X3DUrlObject::setAutoRefreshTimeLimitUnchecked(
                                    std::any_cast<SFTime>(v));
                          },

                          nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"buffer", X3DFieldType::MFFloat, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const BufferAudioSource &>(n).getBuffer());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<BufferAudioSource &>(n).setBufferUnchecked(
                        std::any_cast<MFFloat>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "bufferDuration", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BufferAudioSource &>(n).getBufferDuration());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BufferAudioSource &>(n).setBufferDurationUnchecked(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bufferlength", X3DFieldType::SFInt32, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BufferAudioSource &>(n).getBufferlength());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BufferAudioSource &>(n).emitBufferlength(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "channelCount", X3DFieldType::SFInt32, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BufferAudioSource &>(n).getChannelCount());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BufferAudioSource &>(n).emitChannelCount(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "channelCountMode", X3DFieldType::SFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BufferAudioSource &>(n).getChannelCountMode());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BufferAudioSource &>(n).setChannelCountMode(
              std::any_cast<ChannelCountModeChoices>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(
              dynamic_cast<const BufferAudioSource &>(n).getChannelCountMode());
        },

        [](X3DNode &n, const std::string &s) {
          ChannelCountModeChoices ev;
          if (from_string(s, ev))
            dynamic_cast<BufferAudioSource &>(n).setChannelCountMode(ev);
        }

    });

    t.push_back(FieldInfo{
        "channelInterpretation", X3DFieldType::SFEnum, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const BufferAudioSource &>(n)
                              .getChannelInterpretation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BufferAudioSource &>(n).setChannelInterpretation(
              std::any_cast<ChannelInterpretationChoices>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(dynamic_cast<const BufferAudioSource &>(n)
                               .getChannelInterpretation());
        },

        [](X3DNode &n, const std::string &s) {
          ChannelInterpretationChoices ev;
          if (from_string(s, ev))
            dynamic_cast<BufferAudioSource &>(n).setChannelInterpretation(ev);
        }

    });

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const BufferAudioSource &>(n)
                              .X3DUrlObject::getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BufferAudioSource &>(n).X3DUrlObject::setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"detune", X3DFieldType::SFFloat, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const BufferAudioSource &>(n).getDetune());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<BufferAudioSource &>(n).setDetuneUnchecked(
                        std::any_cast<SFFloat>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "elapsedTime", X3DFieldType::SFTime, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const BufferAudioSource &>(n)
                              .X3DTimeDependentNode::getElapsedTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BufferAudioSource &>(n)
              .X3DTimeDependentNode::emitElapsedTime(std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const BufferAudioSource &>(n)
                              .X3DSoundSourceNode::getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BufferAudioSource &>(n).X3DSoundSourceNode::setEnabled(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "gain", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const BufferAudioSource &>(n)
                              .X3DSoundSourceNode::getGain());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BufferAudioSource &>(n).X3DSoundSourceNode::setGain(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BufferAudioSource &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BufferAudioSource &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isActive", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const BufferAudioSource &>(n)
                              .X3DTimeDependentNode::getIsActive());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BufferAudioSource &>(n)
              .X3DTimeDependentNode::emitIsActive(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isPaused", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const BufferAudioSource &>(n)
                              .X3DTimeDependentNode::getIsPaused());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BufferAudioSource &>(n)
              .X3DTimeDependentNode::emitIsPaused(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"length", X3DFieldType::SFInt32, AccessType::OutputOnly, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const BufferAudioSource &>(n).getLength());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<BufferAudioSource &>(n).emitLength(
                        std::any_cast<SFInt32>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(
        FieldInfo{"load", X3DFieldType::SFBool, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const BufferAudioSource &>(n)
                                        .X3DUrlObject::getLoad());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<BufferAudioSource &>(n).X3DUrlObject::setLoad(
                        std::any_cast<SFBool>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "loop", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const BufferAudioSource &>(n).getLoop());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BufferAudioSource &>(n).setLoop(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "loopEnd", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BufferAudioSource &>(n).getLoopEnd());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BufferAudioSource &>(n).setLoopEndUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "loopStart", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BufferAudioSource &>(n).getLoopStart());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BufferAudioSource &>(n).setLoopStartUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const BufferAudioSource &>(n)
                              .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BufferAudioSource &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "numberOfChannels", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BufferAudioSource &>(n).getNumberOfChannels());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BufferAudioSource &>(n).setNumberOfChannelsUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "pauseTime", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const BufferAudioSource &>(n)
                              .X3DTimeDependentNode::getPauseTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BufferAudioSource &>(n)
              .X3DTimeDependentNode::setPauseTime(std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "playbackRate", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BufferAudioSource &>(n).getPlaybackRate());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BufferAudioSource &>(n).setPlaybackRate(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "resumeTime", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const BufferAudioSource &>(n)
                              .X3DTimeDependentNode::getResumeTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BufferAudioSource &>(n)
              .X3DTimeDependentNode::setResumeTime(std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "sampleRate", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BufferAudioSource &>(n).getSampleRate());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BufferAudioSource &>(n).setSampleRateUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "startTime", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const BufferAudioSource &>(n)
                              .X3DTimeDependentNode::getStartTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BufferAudioSource &>(n)
              .X3DTimeDependentNode::setStartTime(std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "stopTime", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const BufferAudioSource &>(n)
                              .X3DTimeDependentNode::getStopTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BufferAudioSource &>(n)
              .X3DTimeDependentNode::setStopTime(std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"url", X3DFieldType::MFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(dynamic_cast<const BufferAudioSource &>(n)
                                        .X3DUrlObject::getUrl());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<BufferAudioSource &>(n).X3DUrlObject::setUrl(
                        std::any_cast<MFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BufferAudioSource &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BufferAudioSource &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BufferAudioSource &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BufferAudioSource &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BufferAudioSource &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BufferAudioSource &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BufferAudioSource &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BufferAudioSource &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BufferAudioSource &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BufferAudioSource &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void BufferAudioSource::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void BufferAudioSource::validateRanges(
    std::vector<RangeDiagnostic> &out) const {

  X3DUrlObject::checkRangesAutoRefresh(X3DUrlObject::getAutoRefresh(),
                                       nodeTypeName(), "", out);

  X3DUrlObject::checkRangesAutoRefreshTimeLimit(
      X3DUrlObject::getAutoRefreshTimeLimit(), nodeTypeName(), "", out);

  checkRangesBuffer(getBuffer(), nodeTypeName(), "", out);

  checkRangesBufferDuration(getBufferDuration(), nodeTypeName(), "", out);

  checkRangesDetune(getDetune(), nodeTypeName(), "", out);

  checkRangesLoopEnd(getLoopEnd(), nodeTypeName(), "", out);

  checkRangesLoopStart(getLoopStart(), nodeTypeName(), "", out);

  checkRangesNumberOfChannels(getNumberOfChannels(), nodeTypeName(), "", out);

  checkRangesSampleRate(getSampleRate(), nodeTypeName(), "", out);
}

void BufferAudioSource::checkRangesBuffer(const MFFloat &value,
                                          const std::string &nodeType,
                                          const std::string &defName,
                                          std::vector<RangeDiagnostic> &out) {
  for (const auto &v : value) {

    if (v < -1)
      out.push_back(RangeDiagnostic{nodeType, defName, "buffer",
                                    "buffer below minimum of -1"});
    if (v > 1)
      out.push_back(RangeDiagnostic{nodeType, defName, "buffer",
                                    "buffer above maximum of 1"});
  }
}

void BufferAudioSource::checkRangesBufferDuration(
    const SFTime &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "bufferDuration",
                                  "bufferDuration below minimum of 0"});
}

void BufferAudioSource::checkRangesDetune(const SFFloat &value,
                                          const std::string &nodeType,
                                          const std::string &defName,
                                          std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "detune",
                                  "detune below minimum of 0"});
}

void BufferAudioSource::checkRangesLoopEnd(const SFFloat &value,
                                           const std::string &nodeType,
                                           const std::string &defName,
                                           std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "loopEnd",
                                  "loopEnd below minimum of 0"});
}

void BufferAudioSource::checkRangesLoopStart(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "loopStart",
                                  "loopStart below minimum of 0"});
}

void BufferAudioSource::checkRangesNumberOfChannels(
    const SFInt32 &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "numberOfChannels",
                                  "numberOfChannels below minimum of 0"});
}

void BufferAudioSource::checkRangesSampleRate(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "sampleRate",
                                  "sampleRate below minimum of 0"});
}
