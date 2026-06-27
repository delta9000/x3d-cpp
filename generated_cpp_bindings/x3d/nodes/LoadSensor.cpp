// LoadSensor.cpp
#include "x3d/nodes/LoadSensor.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string LoadSensor::nodeTypeName() const { return "LoadSensor"; }

std::string LoadSensor::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &LoadSensor::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "children", X3DFieldType::MFNode, AccessType::InputOutput, "children",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const LoadSensor &>(n).getChildren());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<LoadSensor &>(n).setChildren(std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const LoadSensor &>(n)
                              .X3DSensorNode::getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<LoadSensor &>(n).X3DSensorNode::setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const LoadSensor &>(n).X3DSensorNode::getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<LoadSensor &>(n).X3DSensorNode::setEnabled(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const LoadSensor &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<LoadSensor &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isActive", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const LoadSensor &>(n).X3DSensorNode::getIsActive());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<LoadSensor &>(n).X3DSensorNode::emitIsActive(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isLoaded", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const LoadSensor &>(n).getIsLoaded());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<LoadSensor &>(n).emitIsLoaded(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "loadTime", X3DFieldType::SFTime, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const LoadSensor &>(n).getLoadTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<LoadSensor &>(n).emitLoadTime(std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const LoadSensor &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<LoadSensor &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "progress", X3DFieldType::SFFloat, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const LoadSensor &>(n).getProgress());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<LoadSensor &>(n).emitProgress(std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "timeOut", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const LoadSensor &>(n).getTimeOut());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<LoadSensor &>(n).setTimeOutUnchecked(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const LoadSensor &>(n).X3DNode::getDEF());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<LoadSensor &>(n).X3DNode::setDEF(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(
        FieldInfo{"USE", X3DFieldType::SFString, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const LoadSensor &>(n).X3DNode::getUSE());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<LoadSensor &>(n).X3DNode::setUSE(
                        std::any_cast<SFString>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const LoadSensor &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<LoadSensor &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const LoadSensor &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<LoadSensor &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const LoadSensor &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<LoadSensor &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void LoadSensor::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void LoadSensor::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesTimeOut(getTimeOut(), nodeTypeName(), "", out);
}

void LoadSensor::checkRangesTimeOut(const SFTime &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "timeOut",
                                  "timeOut below minimum of 0"});
}

} // namespace x3d::nodes
