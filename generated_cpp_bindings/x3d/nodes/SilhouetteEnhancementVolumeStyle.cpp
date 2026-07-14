// SilhouetteEnhancementVolumeStyle.cpp
#include "x3d/nodes/SilhouetteEnhancementVolumeStyle.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string SilhouetteEnhancementVolumeStyle::nodeTypeName() const {
  return "SilhouetteEnhancementVolumeStyle";
}

std::string SilhouetteEnhancementVolumeStyle::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &SilhouetteEnhancementVolumeStyle::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SilhouetteEnhancementVolumeStyle &>(n)
                  .X3DVolumeRenderStyleNode::getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SilhouetteEnhancementVolumeStyle &>(n)
              .X3DVolumeRenderStyleNode::setEnabled(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SilhouetteEnhancementVolumeStyle &>(n)
                  .X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SilhouetteEnhancementVolumeStyle &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SilhouetteEnhancementVolumeStyle &>(n)
                  .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SilhouetteEnhancementVolumeStyle &>(n)
              .X3DNode::setMetadata(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "silhouetteBoundaryOpacity", X3DFieldType::SFFloat,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SilhouetteEnhancementVolumeStyle &>(n)
                  .getSilhouetteBoundaryOpacity());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SilhouetteEnhancementVolumeStyle &>(n)
              .setSilhouetteBoundaryOpacityUnchecked(std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "silhouetteRetainedOpacity", X3DFieldType::SFFloat,
        AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SilhouetteEnhancementVolumeStyle &>(n)
                  .getSilhouetteRetainedOpacity());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SilhouetteEnhancementVolumeStyle &>(n)
              .setSilhouetteRetainedOpacityUnchecked(std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "silhouetteSharpness", X3DFieldType::SFFloat, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SilhouetteEnhancementVolumeStyle &>(n)
                  .getSilhouetteSharpness());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SilhouetteEnhancementVolumeStyle &>(n)
              .setSilhouetteSharpnessUnchecked(std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "surfaceNormals", X3DFieldType::SFNode, AccessType::InputOutput,
        "surfaceNormals",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SilhouetteEnhancementVolumeStyle &>(n)
                  .getSurfaceNormals());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SilhouetteEnhancementVolumeStyle &>(n).setSurfaceNormals(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SilhouetteEnhancementVolumeStyle &>(n)
                  .X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SilhouetteEnhancementVolumeStyle &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SilhouetteEnhancementVolumeStyle &>(n)
                  .X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SilhouetteEnhancementVolumeStyle &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SilhouetteEnhancementVolumeStyle &>(n)
                  .X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SilhouetteEnhancementVolumeStyle &>(n)
              .X3DNode::setClass_(std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SilhouetteEnhancementVolumeStyle &>(n)
                  .X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SilhouetteEnhancementVolumeStyle &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const SilhouetteEnhancementVolumeStyle &>(n)
                  .X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<SilhouetteEnhancementVolumeStyle &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void SilhouetteEnhancementVolumeStyle::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void SilhouetteEnhancementVolumeStyle::validateRanges(
    std::vector<RangeDiagnostic> &out) const {

  checkRangesSilhouetteBoundaryOpacity(getSilhouetteBoundaryOpacity(),
                                       nodeTypeName(), "", out);

  checkRangesSilhouetteRetainedOpacity(getSilhouetteRetainedOpacity(),
                                       nodeTypeName(), "", out);

  checkRangesSilhouetteSharpness(getSilhouetteSharpness(), nodeTypeName(), "",
                                 out);
}

void SilhouetteEnhancementVolumeStyle::checkRangesSilhouetteBoundaryOpacity(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(
        RangeDiagnostic{nodeType, defName, "silhouetteBoundaryOpacity",
                        "silhouetteBoundaryOpacity below minimum of 0"});
  if (value > 1)
    out.push_back(
        RangeDiagnostic{nodeType, defName, "silhouetteBoundaryOpacity",
                        "silhouetteBoundaryOpacity above maximum of 1"});
}

void SilhouetteEnhancementVolumeStyle::checkRangesSilhouetteRetainedOpacity(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(
        RangeDiagnostic{nodeType, defName, "silhouetteRetainedOpacity",
                        "silhouetteRetainedOpacity below minimum of 0"});
  if (value > 1)
    out.push_back(
        RangeDiagnostic{nodeType, defName, "silhouetteRetainedOpacity",
                        "silhouetteRetainedOpacity above maximum of 1"});
}

void SilhouetteEnhancementVolumeStyle::checkRangesSilhouetteSharpness(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "silhouetteSharpness",
                                  "silhouetteSharpness below minimum of 0"});
}

namespace factory_detail {
std::shared_ptr<X3DNode> createSilhouetteEnhancementVolumeStyle() {
  return std::make_shared<SilhouetteEnhancementVolumeStyle>();
}
} // namespace factory_detail

} // namespace x3d::nodes
