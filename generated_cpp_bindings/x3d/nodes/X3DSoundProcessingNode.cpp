// X3DSoundProcessingNode.cpp
#include "x3d/nodes/X3DSoundProcessingNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string X3DSoundProcessingNode::nodeTypeName() const {
  return "X3DSoundProcessingNode";
}

std::string X3DSoundProcessingNode::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &X3DSoundProcessingNode::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "channelCount", X3DFieldType::SFInt32, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DSoundProcessingNode &>(n)
                              .getChannelCount());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundProcessingNode &>(n).emitChannelCount(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "channelCountMode", X3DFieldType::SFEnum, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DSoundProcessingNode &>(n)
                              .getChannelCountMode());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundProcessingNode &>(n).setChannelCountMode(
              std::any_cast<ChannelCountModeChoices>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(dynamic_cast<const X3DSoundProcessingNode &>(n)
                               .getChannelCountMode());
        },

        [](X3DNode &n, const std::string &s) {
          ChannelCountModeChoices ev;
          if (from_string(s, ev))
            dynamic_cast<X3DSoundProcessingNode &>(n).setChannelCountMode(ev);
        }

    });

    t.push_back(FieldInfo{
        "channelInterpretation", X3DFieldType::SFEnum, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DSoundProcessingNode &>(n)
                              .getChannelInterpretation());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundProcessingNode &>(n).setChannelInterpretation(
              std::any_cast<ChannelInterpretationChoices>(v));
        },

        [](const X3DNode &n) -> std::string {
          return to_string(dynamic_cast<const X3DSoundProcessingNode &>(n)
                               .getChannelInterpretation());
        },

        [](X3DNode &n, const std::string &s) {
          ChannelInterpretationChoices ev;
          if (from_string(s, ev))
            dynamic_cast<X3DSoundProcessingNode &>(n).setChannelInterpretation(
                ev);
        }

    });

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DSoundProcessingNode &>(n)
                              .X3DTimeDependentNode::getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundProcessingNode &>(n)
              .X3DTimeDependentNode::setDescription(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "elapsedTime", X3DFieldType::SFTime, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DSoundProcessingNode &>(n)
                              .X3DTimeDependentNode::getElapsedTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundProcessingNode &>(n)
              .X3DTimeDependentNode::emitElapsedTime(std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DSoundProcessingNode &>(n).getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundProcessingNode &>(n).setEnabled(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "gain", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DSoundProcessingNode &>(n).getGain());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundProcessingNode &>(n).setGain(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DSoundProcessingNode &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundProcessingNode &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isActive", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DSoundProcessingNode &>(n)
                              .X3DTimeDependentNode::getIsActive());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundProcessingNode &>(n)
              .X3DTimeDependentNode::emitIsActive(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isPaused", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DSoundProcessingNode &>(n)
                              .X3DTimeDependentNode::getIsPaused());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundProcessingNode &>(n)
              .X3DTimeDependentNode::emitIsPaused(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DSoundProcessingNode &>(n)
                              .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundProcessingNode &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "pauseTime", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DSoundProcessingNode &>(n)
                              .X3DTimeDependentNode::getPauseTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundProcessingNode &>(n)
              .X3DTimeDependentNode::setPauseTime(std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "resumeTime", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DSoundProcessingNode &>(n)
                              .X3DTimeDependentNode::getResumeTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundProcessingNode &>(n)
              .X3DTimeDependentNode::setResumeTime(std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "startTime", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DSoundProcessingNode &>(n)
                              .X3DTimeDependentNode::getStartTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundProcessingNode &>(n)
              .X3DTimeDependentNode::setStartTime(std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "stopTime", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DSoundProcessingNode &>(n)
                              .X3DTimeDependentNode::getStopTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundProcessingNode &>(n)
              .X3DTimeDependentNode::setStopTime(std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "tailTime", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DSoundProcessingNode &>(n).getTailTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundProcessingNode &>(n).setTailTimeUnchecked(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DSoundProcessingNode &>(n)
                              .X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundProcessingNode &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DSoundProcessingNode &>(n)
                              .X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundProcessingNode &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DSoundProcessingNode &>(n)
                              .X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundProcessingNode &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DSoundProcessingNode &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundProcessingNode &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DSoundProcessingNode &>(n)
                              .X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundProcessingNode &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void X3DSoundProcessingNode::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void X3DSoundProcessingNode::validateRanges(
    std::vector<RangeDiagnostic> &out) const {

  checkRangesTailTime(getTailTime(), nodeTypeName(), "", out);
}

void X3DSoundProcessingNode::checkRangesTailTime(
    const SFTime &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "tailTime",
                                  "tailTime below minimum of 0"});
}

} // namespace x3d::nodes
