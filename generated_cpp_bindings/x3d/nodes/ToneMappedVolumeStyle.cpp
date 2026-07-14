// ToneMappedVolumeStyle.cpp
#include "x3d/nodes/ToneMappedVolumeStyle.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string ToneMappedVolumeStyle::nodeTypeName() const {
  return "ToneMappedVolumeStyle";
}

std::string ToneMappedVolumeStyle::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &ToneMappedVolumeStyle::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "coolColor", X3DFieldType::SFColorRGBA, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ToneMappedVolumeStyle &>(n).getCoolColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ToneMappedVolumeStyle &>(n).setCoolColorUnchecked(
              std::any_cast<SFColorRGBA>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ToneMappedVolumeStyle &>(n)
                              .X3DVolumeRenderStyleNode::getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ToneMappedVolumeStyle &>(n)
              .X3DVolumeRenderStyleNode::setEnabled(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ToneMappedVolumeStyle &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ToneMappedVolumeStyle &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ToneMappedVolumeStyle &>(n)
                              .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ToneMappedVolumeStyle &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "surfaceNormals", X3DFieldType::SFNode, AccessType::InputOutput,
        "surfaceNormals",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ToneMappedVolumeStyle &>(n)
                              .getSurfaceNormals());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ToneMappedVolumeStyle &>(n).setSurfaceNormals(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "warmColor", X3DFieldType::SFColorRGBA, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ToneMappedVolumeStyle &>(n).getWarmColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ToneMappedVolumeStyle &>(n).setWarmColorUnchecked(
              std::any_cast<SFColorRGBA>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ToneMappedVolumeStyle &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ToneMappedVolumeStyle &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ToneMappedVolumeStyle &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ToneMappedVolumeStyle &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ToneMappedVolumeStyle &>(n)
                              .X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ToneMappedVolumeStyle &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const ToneMappedVolumeStyle &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ToneMappedVolumeStyle &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const ToneMappedVolumeStyle &>(n)
                              .X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<ToneMappedVolumeStyle &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void ToneMappedVolumeStyle::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void ToneMappedVolumeStyle::validateRanges(
    std::vector<RangeDiagnostic> &out) const {

  checkRangesCoolColor(getCoolColor(), nodeTypeName(), "", out);

  checkRangesWarmColor(getWarmColor(), nodeTypeName(), "", out);
}

void ToneMappedVolumeStyle::checkRangesCoolColor(
    const SFColorRGBA &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value.r < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "coolColor",
                                  "coolColor.r below minimum of 0"});
  if (value.r > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "coolColor",
                                  "coolColor.r above maximum of 1"});

  if (value.g < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "coolColor",
                                  "coolColor.g below minimum of 0"});
  if (value.g > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "coolColor",
                                  "coolColor.g above maximum of 1"});

  if (value.b < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "coolColor",
                                  "coolColor.b below minimum of 0"});
  if (value.b > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "coolColor",
                                  "coolColor.b above maximum of 1"});

  if (value.a < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "coolColor",
                                  "coolColor.a below minimum of 0"});
  if (value.a > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "coolColor",
                                  "coolColor.a above maximum of 1"});
}

void ToneMappedVolumeStyle::checkRangesWarmColor(
    const SFColorRGBA &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value.r < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "warmColor",
                                  "warmColor.r below minimum of 0"});
  if (value.r > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "warmColor",
                                  "warmColor.r above maximum of 1"});

  if (value.g < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "warmColor",
                                  "warmColor.g below minimum of 0"});
  if (value.g > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "warmColor",
                                  "warmColor.g above maximum of 1"});

  if (value.b < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "warmColor",
                                  "warmColor.b below minimum of 0"});
  if (value.b > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "warmColor",
                                  "warmColor.b above maximum of 1"});

  if (value.a < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "warmColor",
                                  "warmColor.a below minimum of 0"});
  if (value.a > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "warmColor",
                                  "warmColor.a above maximum of 1"});
}

namespace factory_detail {
std::shared_ptr<X3DNode> createToneMappedVolumeStyle() {
  return std::make_shared<ToneMappedVolumeStyle>();
}
} // namespace factory_detail

} // namespace x3d::nodes
