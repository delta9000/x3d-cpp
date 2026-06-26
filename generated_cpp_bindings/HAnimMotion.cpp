// HAnimMotion.cpp
#include "HAnimMotion.hpp"

std::string HAnimMotion::nodeTypeName() const { return "HAnimMotion"; }

std::string HAnimMotion::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &HAnimMotion::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "channels", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimMotion &>(n).getChannels());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimMotion &>(n).setChannels(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "channelsEnabled", X3DFieldType::MFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimMotion &>(n).getChannelsEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimMotion &>(n).setChannelsEnabled(
              std::any_cast<MFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "cycleTime", X3DFieldType::SFTime, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimMotion &>(n).getCycleTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimMotion &>(n).emitCycleTime(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimMotion &>(n).getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimMotion &>(n).setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "elapsedTime", X3DFieldType::SFTime, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimMotion &>(n).getElapsedTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimMotion &>(n).emitElapsedTime(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimMotion &>(n).getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimMotion &>(n).setEnabled(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "endFrame", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimMotion &>(n).getEndFrame());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimMotion &>(n).setEndFrameUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "frameCount", X3DFieldType::SFInt32, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimMotion &>(n).getFrameCount());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimMotion &>(n).emitFrameCount(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "frameDuration", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimMotion &>(n).getFrameDuration());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimMotion &>(n).setFrameDuration(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "frameIncrement", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimMotion &>(n).getFrameIncrement());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimMotion &>(n).setFrameIncrement(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "frameIndex", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimMotion &>(n).getFrameIndex());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimMotion &>(n).setFrameIndexUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const HAnimMotion &>(n).X3DNode::getIS());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<HAnimMotion &>(n).X3DNode::setIS(
                        std::any_cast<SFNode>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "joints", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimMotion &>(n).getJoints());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimMotion &>(n).setJoints(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "loa", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimMotion &>(n).getLoa());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimMotion &>(n).setLoaUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "loop", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimMotion &>(n).getLoop());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimMotion &>(n).setLoop(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimMotion &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimMotion &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "name", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimMotion &>(n).getName());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimMotion &>(n).setName(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "next", X3DFieldType::SFBool, AccessType::InputOnly, "",

        nullptr,

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimMotion &>(n).onNext(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "previous", X3DFieldType::SFBool, AccessType::InputOnly, "",

        nullptr,

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimMotion &>(n).onPrevious(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "startFrame", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimMotion &>(n).getStartFrame());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimMotion &>(n).setStartFrameUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "values", X3DFieldType::MFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimMotion &>(n).getValues());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimMotion &>(n).setValues(std::any_cast<MFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const HAnimMotion &>(n).X3DNode::getDEF());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<HAnimMotion &>(n).X3DNode::setDEF(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(
        FieldInfo{"USE", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const HAnimMotion &>(n).X3DNode::getUSE());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<HAnimMotion &>(n).X3DNode::setUSE(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimMotion &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimMotion &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"id", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const HAnimMotion &>(n).X3DNode::getId());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<HAnimMotion &>(n).X3DNode::setId(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimMotion &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimMotion &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void HAnimMotion::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void HAnimMotion::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesEndFrame(getEndFrame(), nodeTypeName(), "", out);

  checkRangesFrameIndex(getFrameIndex(), nodeTypeName(), "", out);

  checkRangesLoa(getLoa(), nodeTypeName(), "", out);

  checkRangesStartFrame(getStartFrame(), nodeTypeName(), "", out);
}

void HAnimMotion::checkRangesEndFrame(const SFInt32 &value,
                                      const std::string &nodeType,
                                      const std::string &defName,
                                      std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "endFrame",
                                  "endFrame below minimum of 0"});
}

void HAnimMotion::checkRangesFrameIndex(const SFInt32 &value,
                                        const std::string &nodeType,
                                        const std::string &defName,
                                        std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "frameIndex",
                                  "frameIndex below minimum of 0"});
}

void HAnimMotion::checkRangesLoa(const SFInt32 &value,
                                 const std::string &nodeType,
                                 const std::string &defName,
                                 std::vector<RangeDiagnostic> &out) {
  if (value < -1)
    out.push_back(
        RangeDiagnostic{nodeType, defName, "loa", "loa below minimum of -1"});
  if (value > 4)
    out.push_back(
        RangeDiagnostic{nodeType, defName, "loa", "loa above maximum of 4"});
}

void HAnimMotion::checkRangesStartFrame(const SFInt32 &value,
                                        const std::string &nodeType,
                                        const std::string &defName,
                                        std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "startFrame",
                                  "startFrame below minimum of 0"});
}
