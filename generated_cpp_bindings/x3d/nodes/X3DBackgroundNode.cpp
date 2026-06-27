// X3DBackgroundNode.cpp
#include "x3d/nodes/X3DBackgroundNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string X3DBackgroundNode::nodeTypeName() const {
  return "X3DBackgroundNode";
}

std::string X3DBackgroundNode::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &X3DBackgroundNode::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "bindTime", X3DFieldType::SFTime, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DBackgroundNode &>(n)
                              .X3DBindableNode::getBindTime());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DBackgroundNode &>(n).X3DBindableNode::emitBindTime(
              std::any_cast<SFTime>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "groundAngle", X3DFieldType::MFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DBackgroundNode &>(n).getGroundAngle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DBackgroundNode &>(n).setGroundAngleUnchecked(
              std::any_cast<MFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "groundColor", X3DFieldType::MFColor, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DBackgroundNode &>(n).getGroundColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DBackgroundNode &>(n).setGroundColorUnchecked(
              std::any_cast<MFColor>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DBackgroundNode &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DBackgroundNode &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "isBound", X3DFieldType::SFBool, AccessType::OutputOnly, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DBackgroundNode &>(n)
                              .X3DBindableNode::getIsBound());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DBackgroundNode &>(n).X3DBindableNode::emitIsBound(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const X3DBackgroundNode &>(n)
                              .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DBackgroundNode &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "set_bind", X3DFieldType::SFBool, AccessType::InputOnly, "",

        nullptr,

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DBackgroundNode &>(n).X3DBindableNode::onSet_bind(
              std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "skyAngle", X3DFieldType::MFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DBackgroundNode &>(n).getSkyAngle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DBackgroundNode &>(n).setSkyAngleUnchecked(
              std::any_cast<MFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "skyColor", X3DFieldType::MFColor, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DBackgroundNode &>(n).getSkyColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DBackgroundNode &>(n).setSkyColorUnchecked(
              std::any_cast<MFColor>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "transparency", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DBackgroundNode &>(n).getTransparency());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DBackgroundNode &>(n).setTransparencyUnchecked(
              std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DBackgroundNode &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DBackgroundNode &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DBackgroundNode &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DBackgroundNode &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DBackgroundNode &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DBackgroundNode &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DBackgroundNode &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DBackgroundNode &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const X3DBackgroundNode &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<X3DBackgroundNode &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void X3DBackgroundNode::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void X3DBackgroundNode::validateRanges(
    std::vector<RangeDiagnostic> &out) const {

  checkRangesGroundAngle(getGroundAngle(), nodeTypeName(), "", out);

  checkRangesGroundColor(getGroundColor(), nodeTypeName(), "", out);

  checkRangesSkyAngle(getSkyAngle(), nodeTypeName(), "", out);

  checkRangesSkyColor(getSkyColor(), nodeTypeName(), "", out);

  checkRangesTransparency(getTransparency(), nodeTypeName(), "", out);
}

void X3DBackgroundNode::checkRangesGroundAngle(
    const MFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  for (const auto &v : value) {

    if (v < 0)
      out.push_back(RangeDiagnostic{nodeType, defName, "groundAngle",
                                    "groundAngle below minimum of 0"});
    if (v > 1.5708)
      out.push_back(RangeDiagnostic{nodeType, defName, "groundAngle",
                                    "groundAngle above maximum of 1.5708"});
  }
}

void X3DBackgroundNode::checkRangesGroundColor(
    const MFColor &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  for (const auto &v : value) {

    if (v.r < 0)
      out.push_back(RangeDiagnostic{nodeType, defName, "groundColor",
                                    "groundColor.r below minimum of 0"});
    if (v.r > 1)
      out.push_back(RangeDiagnostic{nodeType, defName, "groundColor",
                                    "groundColor.r above maximum of 1"});

    if (v.g < 0)
      out.push_back(RangeDiagnostic{nodeType, defName, "groundColor",
                                    "groundColor.g below minimum of 0"});
    if (v.g > 1)
      out.push_back(RangeDiagnostic{nodeType, defName, "groundColor",
                                    "groundColor.g above maximum of 1"});

    if (v.b < 0)
      out.push_back(RangeDiagnostic{nodeType, defName, "groundColor",
                                    "groundColor.b below minimum of 0"});
    if (v.b > 1)
      out.push_back(RangeDiagnostic{nodeType, defName, "groundColor",
                                    "groundColor.b above maximum of 1"});
  }
}

void X3DBackgroundNode::checkRangesSkyAngle(const MFFloat &value,
                                            const std::string &nodeType,
                                            const std::string &defName,
                                            std::vector<RangeDiagnostic> &out) {
  for (const auto &v : value) {

    if (v < 0)
      out.push_back(RangeDiagnostic{nodeType, defName, "skyAngle",
                                    "skyAngle below minimum of 0"});
    if (v > 3.1416)
      out.push_back(RangeDiagnostic{nodeType, defName, "skyAngle",
                                    "skyAngle above maximum of 3.1416"});
  }
}

void X3DBackgroundNode::checkRangesSkyColor(const MFColor &value,
                                            const std::string &nodeType,
                                            const std::string &defName,
                                            std::vector<RangeDiagnostic> &out) {
  for (const auto &v : value) {

    if (v.r < 0)
      out.push_back(RangeDiagnostic{nodeType, defName, "skyColor",
                                    "skyColor.r below minimum of 0"});
    if (v.r > 1)
      out.push_back(RangeDiagnostic{nodeType, defName, "skyColor",
                                    "skyColor.r above maximum of 1"});

    if (v.g < 0)
      out.push_back(RangeDiagnostic{nodeType, defName, "skyColor",
                                    "skyColor.g below minimum of 0"});
    if (v.g > 1)
      out.push_back(RangeDiagnostic{nodeType, defName, "skyColor",
                                    "skyColor.g above maximum of 1"});

    if (v.b < 0)
      out.push_back(RangeDiagnostic{nodeType, defName, "skyColor",
                                    "skyColor.b below minimum of 0"});
    if (v.b > 1)
      out.push_back(RangeDiagnostic{nodeType, defName, "skyColor",
                                    "skyColor.b above maximum of 1"});
  }
}

void X3DBackgroundNode::checkRangesTransparency(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "transparency",
                                  "transparency below minimum of 0"});
  if (value > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "transparency",
                                  "transparency above maximum of 1"});
}

} // namespace x3d::nodes
