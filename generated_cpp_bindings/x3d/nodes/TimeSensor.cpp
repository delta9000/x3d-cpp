// TimeSensor.cpp
#include "x3d/nodes/TimeSensor.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string TimeSensor::nodeTypeName() const { return "TimeSensor"; }

std::string TimeSensor::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &TimeSensor::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "cycleInterval", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TimeSensor &>(n).getCycleInterval());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TimeSensor &>(n).setCycleIntervalUnchecked(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const TimeSensor &>(n).getCycleInterval(),
                  &typeid(SFTime)};
        }

    });

    t.push_back(FieldInfo{
        "cycleTime", X3DFieldType::SFTime, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TimeSensor &>(n).getCycleTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TimeSensor &>(n).emitCycleTime(std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const TimeSensor &>(n).getCycleTime(),
                  &typeid(SFTime)};
        }

    });

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TimeSensor &>(n)
                              .X3DTimeDependentNode::getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TimeSensor &>(n).X3DTimeDependentNode::setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const TimeSensor &>(n)
                       .X3DTimeDependentNode::getDescription(),
                  &typeid(SFString)};
        }

    });

    t.push_back(FieldInfo{
        "elapsedTime", X3DFieldType::SFTime, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TimeSensor &>(n)
                              .X3DTimeDependentNode::getElapsedTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TimeSensor &>(n).X3DTimeDependentNode::emitElapsedTime(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const TimeSensor &>(n)
                       .X3DTimeDependentNode::getElapsedTime(),
                  &typeid(SFTime)};
        }

    });

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TimeSensor &>(n).X3DSensorNode::getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TimeSensor &>(n).X3DSensorNode::setEnabled(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {
              &dynamic_cast<const TimeSensor &>(n).X3DSensorNode::getEnabled(),
              &typeid(SFBool)};
        }

    });

    t.push_back(FieldInfo{
        "fraction_changed", X3DFieldType::SFFloat, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TimeSensor &>(n).getFraction_changed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TimeSensor &>(n).emitFraction_changed(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const TimeSensor &>(n).getFraction_changed(),
                  &typeid(SFFloat)};
        }

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TimeSensor &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TimeSensor &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const TimeSensor &>(n).X3DNode::getIS(),
                  &typeid(SFNode)};
        }

    });

    t.push_back(FieldInfo{
        "isActive", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TimeSensor &>(n)
                              .X3DTimeDependentNode::getIsActive());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TimeSensor &>(n).X3DTimeDependentNode::emitIsActive(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const TimeSensor &>(n)
                       .X3DTimeDependentNode::getIsActive(),
                  &typeid(SFBool)};
        }

    });

    t.push_back(FieldInfo{
        "isPaused", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TimeSensor &>(n)
                              .X3DTimeDependentNode::getIsPaused());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TimeSensor &>(n).X3DTimeDependentNode::emitIsPaused(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const TimeSensor &>(n)
                       .X3DTimeDependentNode::getIsPaused(),
                  &typeid(SFBool)};
        }

    });

    t.push_back(FieldInfo{
        "loop", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TimeSensor &>(n).getLoop());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TimeSensor &>(n).setLoop(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const TimeSensor &>(n).getLoop(),
                  &typeid(SFBool)};
        }

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TimeSensor &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TimeSensor &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const TimeSensor &>(n).X3DNode::getMetadata(),
                  &typeid(SFNode)};
        }

    });

    t.push_back(FieldInfo{
        "pauseTime", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TimeSensor &>(n)
                              .X3DTimeDependentNode::getPauseTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TimeSensor &>(n).X3DTimeDependentNode::setPauseTime(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const TimeSensor &>(n)
                       .X3DTimeDependentNode::getPauseTime(),
                  &typeid(SFTime)};
        }

    });

    t.push_back(FieldInfo{
        "resumeTime", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TimeSensor &>(n)
                              .X3DTimeDependentNode::getResumeTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TimeSensor &>(n).X3DTimeDependentNode::setResumeTime(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const TimeSensor &>(n)
                       .X3DTimeDependentNode::getResumeTime(),
                  &typeid(SFTime)};
        }

    });

    t.push_back(FieldInfo{
        "startTime", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TimeSensor &>(n)
                              .X3DTimeDependentNode::getStartTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TimeSensor &>(n).X3DTimeDependentNode::setStartTime(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const TimeSensor &>(n)
                       .X3DTimeDependentNode::getStartTime(),
                  &typeid(SFTime)};
        }

    });

    t.push_back(FieldInfo{
        "stopTime", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TimeSensor &>(n)
                              .X3DTimeDependentNode::getStopTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TimeSensor &>(n).X3DTimeDependentNode::setStopTime(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const TimeSensor &>(n)
                       .X3DTimeDependentNode::getStopTime(),
                  &typeid(SFTime)};
        }

    });

    t.push_back(FieldInfo{
        "time", X3DFieldType::SFTime, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TimeSensor &>(n).getTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TimeSensor &>(n).emitTime(std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const TimeSensor &>(n).getTime(),
                  &typeid(SFTime)};
        }

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TimeSensor &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TimeSensor &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const TimeSensor &>(n).X3DNode::getDEF(),
                  &typeid(SFString)};
        }

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TimeSensor &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TimeSensor &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const TimeSensor &>(n).X3DNode::getUSE(),
                  &typeid(SFString)};
        }

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TimeSensor &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TimeSensor &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const TimeSensor &>(n).X3DNode::getClass_(),
                  &typeid(SFString)};
        }

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const TimeSensor &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TimeSensor &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const TimeSensor &>(n).X3DNode::getId(),
                  &typeid(SFString)};
        }

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const TimeSensor &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<TimeSensor &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const TimeSensor &>(n).X3DNode::getStyle(),
                  &typeid(SFString)};
        }

    });

    return t;
  }();
  return table;
}

void TimeSensor::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void TimeSensor::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesCycleInterval(getCycleInterval(), nodeTypeName(), "", out);
}

void TimeSensor::checkRangesCycleInterval(const SFTime &value,
                                          const std::string &nodeType,
                                          const std::string &defName,
                                          std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "cycleInterval",
                                  "cycleInterval below minimum of 0"});
}

namespace factory_detail {
std::shared_ptr<X3DNode> createTimeSensor() {
  return std::make_shared<TimeSensor>();
}
} // namespace factory_detail

} // namespace x3d::nodes
