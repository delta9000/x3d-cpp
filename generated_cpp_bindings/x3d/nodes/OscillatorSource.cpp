// OscillatorSource.cpp
#include "x3d/nodes/OscillatorSource.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string OscillatorSource::nodeTypeName() const {
  return "OscillatorSource";
}

std::string OscillatorSource::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &OscillatorSource::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const OscillatorSource &>(n)
                              .X3DTimeDependentNode::getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<OscillatorSource &>(n)
              .X3DTimeDependentNode::setDescription(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"detune", X3DFieldType::SFFloat, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const OscillatorSource &>(n).getDetune());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<OscillatorSource &>(n).setDetuneUnchecked(
                        std::any_cast<SFFloat>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "elapsedTime", X3DFieldType::SFTime, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const OscillatorSource &>(n)
                              .X3DTimeDependentNode::getElapsedTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<OscillatorSource &>(n)
              .X3DTimeDependentNode::emitElapsedTime(std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const OscillatorSource &>(n)
                              .X3DSoundSourceNode::getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<OscillatorSource &>(n).X3DSoundSourceNode::setEnabled(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "frequency", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const OscillatorSource &>(n).getFrequency());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<OscillatorSource &>(n).setFrequencyUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "gain", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const OscillatorSource &>(n)
                              .X3DSoundSourceNode::getGain());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<OscillatorSource &>(n).X3DSoundSourceNode::setGain(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const OscillatorSource &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<OscillatorSource &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isActive", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const OscillatorSource &>(n)
                              .X3DTimeDependentNode::getIsActive());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<OscillatorSource &>(n)
              .X3DTimeDependentNode::emitIsActive(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isPaused", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const OscillatorSource &>(n)
                              .X3DTimeDependentNode::getIsPaused());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<OscillatorSource &>(n)
              .X3DTimeDependentNode::emitIsPaused(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const OscillatorSource &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<OscillatorSource &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "pauseTime", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const OscillatorSource &>(n)
                              .X3DTimeDependentNode::getPauseTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<OscillatorSource &>(n)
              .X3DTimeDependentNode::setPauseTime(std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "resumeTime", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const OscillatorSource &>(n)
                              .X3DTimeDependentNode::getResumeTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<OscillatorSource &>(n)
              .X3DTimeDependentNode::setResumeTime(std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "startTime", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const OscillatorSource &>(n)
                              .X3DTimeDependentNode::getStartTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<OscillatorSource &>(n)
              .X3DTimeDependentNode::setStartTime(std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "stopTime", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const OscillatorSource &>(n)
                              .X3DTimeDependentNode::getStopTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<OscillatorSource &>(n).X3DTimeDependentNode::setStopTime(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const OscillatorSource &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<OscillatorSource &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const OscillatorSource &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<OscillatorSource &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const OscillatorSource &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<OscillatorSource &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const OscillatorSource &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<OscillatorSource &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const OscillatorSource &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<OscillatorSource &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void OscillatorSource::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void OscillatorSource::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesDetune(getDetune(), nodeTypeName(), "", out);

  checkRangesFrequency(getFrequency(), nodeTypeName(), "", out);
}

void OscillatorSource::checkRangesDetune(const SFFloat &value,
                                         const std::string &nodeType,
                                         const std::string &defName,
                                         std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "detune",
                                  "detune below minimum of 0"});
}

void OscillatorSource::checkRangesFrequency(const SFFloat &value,
                                            const std::string &nodeType,
                                            const std::string &defName,
                                            std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "frequency",
                                  "frequency below minimum of 0"});
}

} // namespace x3d::nodes
