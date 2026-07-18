// NavigationInfo.cpp
#include "x3d/nodes/NavigationInfo.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string NavigationInfo::nodeTypeName() const { return "NavigationInfo"; }

std::string NavigationInfo::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &NavigationInfo::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "avatarSize", X3DFieldType::MFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NavigationInfo &>(n).getAvatarSize());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NavigationInfo &>(n).setAvatarSizeUnchecked(
              std::any_cast<MFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "bindTime", X3DFieldType::SFTime, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NavigationInfo &>(n)
                              .X3DBindableNode::getBindTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NavigationInfo &>(n).X3DBindableNode::emitBindTime(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "headlight", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NavigationInfo &>(n).getHeadlight());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NavigationInfo &>(n).setHeadlight(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NavigationInfo &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NavigationInfo &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isBound", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NavigationInfo &>(n)
                              .X3DBindableNode::getIsBound());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NavigationInfo &>(n).X3DBindableNode::emitIsBound(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NavigationInfo &>(n).X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NavigationInfo &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "set_bind", X3DFieldType::SFBool, AccessType::InputOnly, "",

        nullptr,

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NavigationInfo &>(n).X3DBindableNode::onSet_bind(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "speed", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NavigationInfo &>(n).getSpeed());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NavigationInfo &>(n).setSpeedUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "transitionComplete", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NavigationInfo &>(n).getTransitionComplete());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NavigationInfo &>(n).emitTransitionComplete(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "transitionTime", X3DFieldType::SFTime, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NavigationInfo &>(n).getTransitionTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NavigationInfo &>(n).setTransitionTimeUnchecked(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "transitionType", X3DFieldType::MFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NavigationInfo &>(n).getTransitionType());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NavigationInfo &>(n).setTransitionType(
              std::any_cast<MFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "type", X3DFieldType::MFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const NavigationInfo &>(n).getType());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NavigationInfo &>(n).setType(std::any_cast<MFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "visibilityLimit", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NavigationInfo &>(n).getVisibilityLimit());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NavigationInfo &>(n).setVisibilityLimitUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NavigationInfo &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NavigationInfo &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NavigationInfo &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NavigationInfo &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NavigationInfo &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NavigationInfo &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NavigationInfo &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NavigationInfo &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const NavigationInfo &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<NavigationInfo &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void NavigationInfo::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void NavigationInfo::validateRanges(std::vector<RangeDiagnostic> &out) const {

  checkRangesAvatarSize(getAvatarSize(), nodeTypeName(), "", out);

  checkRangesSpeed(getSpeed(), nodeTypeName(), "", out);

  checkRangesTransitionTime(getTransitionTime(), nodeTypeName(), "", out);

  checkRangesVisibilityLimit(getVisibilityLimit(), nodeTypeName(), "", out);
}

void NavigationInfo::checkRangesAvatarSize(const MFFloat &value,
                                           const std::string &nodeType,
                                           const std::string &defName,
                                           std::vector<RangeDiagnostic> &out) {
  for (const auto &v : value) {

    if (v < 0.0f)
      out.push_back(RangeDiagnostic{nodeType, defName, "avatarSize",
                                    "avatarSize below minimum of 0"});
  }
}

void NavigationInfo::checkRangesSpeed(const SFFloat &value,
                                      const std::string &nodeType,
                                      const std::string &defName,
                                      std::vector<RangeDiagnostic> &out) {
  if (value < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "speed",
                                  "speed below minimum of 0"});
}

void NavigationInfo::checkRangesTransitionTime(
    const SFTime &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "transitionTime",
                                  "transitionTime below minimum of 0"});
}

void NavigationInfo::checkRangesVisibilityLimit(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "visibilityLimit",
                                  "visibilityLimit below minimum of 0"});
}

namespace factory_detail {
std::shared_ptr<X3DNode> createNavigationInfo() {
  return std::make_shared<NavigationInfo>();
}
} // namespace factory_detail

} // namespace x3d::nodes
