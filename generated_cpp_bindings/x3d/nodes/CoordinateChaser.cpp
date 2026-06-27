// CoordinateChaser.cpp
#include "x3d/nodes/CoordinateChaser.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string CoordinateChaser::nodeTypeName() const {
  return "CoordinateChaser";
}

std::string CoordinateChaser::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &CoordinateChaser::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "duration", X3DFieldType::SFTime, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const CoordinateChaser &>(n)
                              .X3DChaserNode::getDuration());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CoordinateChaser &>(n)
              .X3DChaserNode::setDurationUnchecked(std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "initialDestination", X3DFieldType::MFVec3f, AccessType::InitializeOnly,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const CoordinateChaser &>(n)
                              .getInitialDestination());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CoordinateChaser &>(n).setInitialDestinationUnchecked(
              std::any_cast<MFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "initialValue", X3DFieldType::MFVec3f, AccessType::InitializeOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CoordinateChaser &>(n).getInitialValue());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CoordinateChaser &>(n).setInitialValueUnchecked(
              std::any_cast<MFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CoordinateChaser &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CoordinateChaser &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isActive", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const CoordinateChaser &>(n)
                              .X3DFollowerNode::getIsActive());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CoordinateChaser &>(n).X3DFollowerNode::emitIsActive(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CoordinateChaser &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CoordinateChaser &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "set_destination", X3DFieldType::MFVec3f, AccessType::InputOnly, "",

        nullptr,

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CoordinateChaser &>(n).onSet_destination(
              std::any_cast<MFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{"set_value", X3DFieldType::MFVec3f,
                          AccessType::InputOnly, "",

                          nullptr,

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<CoordinateChaser &>(n).onSet_value(
                                std::any_cast<MFVec3f>(v));
                          },

                          nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "value_changed", X3DFieldType::MFVec3f, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CoordinateChaser &>(n).getValue_changed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CoordinateChaser &>(n).emitValue_changed(
              std::any_cast<MFVec3f>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CoordinateChaser &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CoordinateChaser &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CoordinateChaser &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CoordinateChaser &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CoordinateChaser &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CoordinateChaser &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CoordinateChaser &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CoordinateChaser &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CoordinateChaser &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CoordinateChaser &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void CoordinateChaser::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

} // namespace x3d::nodes
