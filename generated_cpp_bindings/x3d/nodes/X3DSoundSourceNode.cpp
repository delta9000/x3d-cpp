// X3DSoundSourceNode.cpp
#include "x3d/nodes/X3DSoundSourceNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string X3DSoundSourceNode::nodeTypeName() const {
  return "X3DSoundSourceNode";
}

std::string X3DSoundSourceNode::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &X3DSoundSourceNode::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DSoundSourceNode &>(n)
                              .X3DTimeDependentNode::getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundSourceNode &>(n)
              .X3DTimeDependentNode::setDescription(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "elapsedTime", X3DFieldType::SFTime, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DSoundSourceNode &>(n)
                              .X3DTimeDependentNode::getElapsedTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundSourceNode &>(n)
              .X3DTimeDependentNode::emitElapsedTime(std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DSoundSourceNode &>(n).getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundSourceNode &>(n).setEnabled(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(
        FieldInfo{"gain", X3DFieldType::SFFloat, AccessType::InputOutput, "",

                  [](const X3DNode &n) -> std::any {
                    return std::any(
                        dynamic_cast<const X3DSoundSourceNode &>(n).getGain());
                  },

                  [](X3DNode &n, const std::any &v) {
                    dynamic_cast<X3DSoundSourceNode &>(n).setGain(
                        std::any_cast<SFFloat>(v));
                  },

                  nullptr, nullptr

        });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DSoundSourceNode &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundSourceNode &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isActive", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DSoundSourceNode &>(n)
                              .X3DTimeDependentNode::getIsActive());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundSourceNode &>(n)
              .X3DTimeDependentNode::emitIsActive(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isPaused", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DSoundSourceNode &>(n)
                              .X3DTimeDependentNode::getIsPaused());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundSourceNode &>(n)
              .X3DTimeDependentNode::emitIsPaused(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DSoundSourceNode &>(n)
                              .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundSourceNode &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "pauseTime", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DSoundSourceNode &>(n)
                              .X3DTimeDependentNode::getPauseTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundSourceNode &>(n)
              .X3DTimeDependentNode::setPauseTime(std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "resumeTime", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DSoundSourceNode &>(n)
                              .X3DTimeDependentNode::getResumeTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundSourceNode &>(n)
              .X3DTimeDependentNode::setResumeTime(std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "startTime", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DSoundSourceNode &>(n)
                              .X3DTimeDependentNode::getStartTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundSourceNode &>(n)
              .X3DTimeDependentNode::setStartTime(std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "stopTime", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DSoundSourceNode &>(n)
                              .X3DTimeDependentNode::getStopTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundSourceNode &>(n)
              .X3DTimeDependentNode::setStopTime(std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DSoundSourceNode &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundSourceNode &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DSoundSourceNode &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundSourceNode &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DSoundSourceNode &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundSourceNode &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DSoundSourceNode &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundSourceNode &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DSoundSourceNode &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DSoundSourceNode &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void X3DSoundSourceNode::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

} // namespace x3d::nodes
