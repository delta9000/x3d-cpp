// EdgeEnhancementVolumeStyle.cpp
#include "x3d/nodes/EdgeEnhancementVolumeStyle.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string EdgeEnhancementVolumeStyle::nodeTypeName() const {
  return "EdgeEnhancementVolumeStyle";
}

std::string EdgeEnhancementVolumeStyle::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &EdgeEnhancementVolumeStyle::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "edgeColor", X3DFieldType::SFColorRGBA, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EdgeEnhancementVolumeStyle &>(n)
                              .getEdgeColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EdgeEnhancementVolumeStyle &>(n).setEdgeColorUnchecked(
              std::any_cast<SFColorRGBA>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EdgeEnhancementVolumeStyle &>(n)
                              .X3DVolumeRenderStyleNode::getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EdgeEnhancementVolumeStyle &>(n)
              .X3DVolumeRenderStyleNode::setEnabled(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "gradientThreshold", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EdgeEnhancementVolumeStyle &>(n)
                              .getGradientThreshold());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EdgeEnhancementVolumeStyle &>(n)
              .setGradientThresholdUnchecked(std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EdgeEnhancementVolumeStyle &>(n)
                              .X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EdgeEnhancementVolumeStyle &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EdgeEnhancementVolumeStyle &>(n)
                              .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EdgeEnhancementVolumeStyle &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "surfaceNormals", X3DFieldType::SFNode, AccessType::InputOutput,
        "surfaceNormals",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EdgeEnhancementVolumeStyle &>(n)
                              .getSurfaceNormals());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EdgeEnhancementVolumeStyle &>(n).setSurfaceNormals(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EdgeEnhancementVolumeStyle &>(n)
                              .X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EdgeEnhancementVolumeStyle &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EdgeEnhancementVolumeStyle &>(n)
                              .X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EdgeEnhancementVolumeStyle &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EdgeEnhancementVolumeStyle &>(n)
                              .X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EdgeEnhancementVolumeStyle &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EdgeEnhancementVolumeStyle &>(n)
                              .X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EdgeEnhancementVolumeStyle &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const EdgeEnhancementVolumeStyle &>(n)
                              .X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<EdgeEnhancementVolumeStyle &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void EdgeEnhancementVolumeStyle::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void EdgeEnhancementVolumeStyle::validateRanges(
    std::vector<RangeDiagnostic> &out) const {

  checkRangesEdgeColor(getEdgeColor(), nodeTypeName(), "", out);

  checkRangesGradientThreshold(getGradientThreshold(), nodeTypeName(), "", out);
}

void EdgeEnhancementVolumeStyle::checkRangesEdgeColor(
    const SFColorRGBA &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value.r < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "edgeColor",
                                  "edgeColor.r below minimum of 0"});
  if (value.r > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "edgeColor",
                                  "edgeColor.r above maximum of 1"});

  if (value.g < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "edgeColor",
                                  "edgeColor.g below minimum of 0"});
  if (value.g > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "edgeColor",
                                  "edgeColor.g above maximum of 1"});

  if (value.b < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "edgeColor",
                                  "edgeColor.b below minimum of 0"});
  if (value.b > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "edgeColor",
                                  "edgeColor.b above maximum of 1"});

  if (value.a < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "edgeColor",
                                  "edgeColor.a below minimum of 0"});
  if (value.a > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "edgeColor",
                                  "edgeColor.a above maximum of 1"});
}

void EdgeEnhancementVolumeStyle::checkRangesGradientThreshold(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "gradientThreshold",
                                  "gradientThreshold below minimum of 0"});
  if (value > 3.1416)
    out.push_back(RangeDiagnostic{nodeType, defName, "gradientThreshold",
                                  "gradientThreshold above maximum of 3.1416"});
}

namespace factory_detail {
std::shared_ptr<X3DNode> createEdgeEnhancementVolumeStyle() {
  return std::make_shared<EdgeEnhancementVolumeStyle>();
}
} // namespace factory_detail

} // namespace x3d::nodes
