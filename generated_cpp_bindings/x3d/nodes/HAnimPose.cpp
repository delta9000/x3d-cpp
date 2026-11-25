#include "x3d/nodes/HAnimPose.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string HAnimPose::nodeTypeName() const { return "HAnimPose"; }

std::string HAnimPose::defaultContainerField() const {
  return getDefaultContainerField();
}

X3DVersion HAnimPose::introducedIn() const { return X3DVersion{4, 1}; }

const FieldTable &HAnimPose::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "children", X3DFieldType::MFNode, AccessType::InputOutput, "children",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimPose &>(n).getChildren());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimPose &>(n).setChildren(std::any_cast<MFNode>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const HAnimPose &>(n).getChildren(),
                  &typeid(MFNode)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "commencePose", X3DFieldType::SFBool, AccessType::InputOnly, "",

        nullptr,

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimPose &>(n).onCommencePose(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

        ,

        nullptr,

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "description", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimPose &>(n).getDescription());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimPose &>(n).setDescription(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const HAnimPose &>(n).getDescription(),
                  &typeid(SFString)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimPose &>(n).getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimPose &>(n).setEnabled(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const HAnimPose &>(n).getEnabled(),
                  &typeid(SFBool)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimPose &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimPose &>(n).X3DNode::setIS(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const HAnimPose &>(n).X3DNode::getIS(),
                  &typeid(SFNode)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "isActive", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimPose &>(n).getIsActive());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimPose &>(n).emitIsActive(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const HAnimPose &>(n).getIsActive(),
                  &typeid(SFBool)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "loa", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimPose &>(n).getLoa());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimPose &>(n).setLoaUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const HAnimPose &>(n).getLoa(),
                  &typeid(SFInt32)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimPose &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimPose &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const HAnimPose &>(n).X3DNode::getMetadata(),
                  &typeid(SFNode)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "name", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimPose &>(n).getName());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimPose &>(n).setName(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const HAnimPose &>(n).getName(),
                  &typeid(SFString)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{"resetAllJoints", X3DFieldType::SFBool,
                          AccessType::InputOnly, "",

                          nullptr,

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<HAnimPose &>(n).onResetAllJoints(
                                std::any_cast<SFBool>(v));
                          },

                          nullptr, nullptr

                          ,

                          nullptr,

                          X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "set_fraction", X3DFieldType::SFTime, AccessType::InputOnly, "",

        nullptr,

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimPose &>(n).onSet_fraction(std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

        ,

        nullptr,

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{"set_startTime", X3DFieldType::SFTime,
                          AccessType::InputOnly, "",

                          nullptr,

                          [](X3DNode &n, const std::any &v) {
                            dynamic_cast<HAnimPose &>(n).onSet_startTime(
                                std::any_cast<SFTime>(v));
                          },

                          nullptr, nullptr

                          ,

                          nullptr,

                          X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "transitionDuration", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimPose &>(n).getTransitionDuration());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimPose &>(n).setTransitionDurationUnchecked(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const HAnimPose &>(n).getTransitionDuration(),
                  &typeid(SFTime)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimPose &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimPose &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const HAnimPose &>(n).X3DNode::getDEF(),
                  &typeid(SFString)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimPose &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimPose &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const HAnimPose &>(n).X3DNode::getUSE(),
                  &typeid(SFString)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimPose &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimPose &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const HAnimPose &>(n).X3DNode::getClass_(),
                  &typeid(SFString)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const HAnimPose &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimPose &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const HAnimPose &>(n).X3DNode::getId(),
                  &typeid(SFString)};
        },

        X3DVersion{4, 1}});

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const HAnimPose &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<HAnimPose &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

        ,

        [](const X3DNode &n) -> FieldView {
          return {&dynamic_cast<const HAnimPose &>(n).X3DNode::getStyle(),
                  &typeid(SFString)};
        },

        X3DVersion{4, 1}});

    return t;
  }();
  return table;
}

void HAnimPose::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void HAnimPose::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesLoa(getLoa(), nodeTypeName(), "", out);

  checkRangesTransitionDuration(getTransitionDuration(), nodeTypeName(), "",
                                out);
}

void HAnimPose::checkRangesLoa(const SFInt32 &value,
                               const std::string &nodeType,
                               const std::string &defName,
                               std::vector<RangeDiagnostic> &out) {
  if (value < -1)
    out.push_back(
        RangeDiagnostic{nodeType, defName, "loa", "loa below minimum of -1"});
  if (value > 5)
    out.push_back(
        RangeDiagnostic{nodeType, defName, "loa", "loa above maximum of 5"});
}

void HAnimPose::checkRangesTransitionDuration(
    const SFTime &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0.0)
    out.push_back(RangeDiagnostic{nodeType, defName, "transitionDuration",
                                  "transitionDuration below minimum of 0.0"});
}

namespace factory_detail {
std::shared_ptr<X3DNode> createHAnimPose() {
  return std::make_shared<HAnimPose>();
}
} // namespace factory_detail

} // namespace x3d::nodes
