// BoundaryEnhancementVolumeStyle.cpp
#include "BoundaryEnhancementVolumeStyle.hpp"

std::string BoundaryEnhancementVolumeStyle::nodeTypeName() const {
  return "BoundaryEnhancementVolumeStyle";
}

std::string BoundaryEnhancementVolumeStyle::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &BoundaryEnhancementVolumeStyle::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "boundaryOpacity", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BoundaryEnhancementVolumeStyle &>(n)
                  .getBoundaryOpacity());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BoundaryEnhancementVolumeStyle &>(n)
              .setBoundaryOpacityUnchecked(std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BoundaryEnhancementVolumeStyle &>(n)
                  .X3DVolumeRenderStyleNode::getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BoundaryEnhancementVolumeStyle &>(n)
              .X3DVolumeRenderStyleNode::setEnabled(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BoundaryEnhancementVolumeStyle &>(n)
                  .X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BoundaryEnhancementVolumeStyle &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BoundaryEnhancementVolumeStyle &>(n)
                  .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BoundaryEnhancementVolumeStyle &>(n)
              .X3DNode::setMetadata(std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "opacityFactor", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BoundaryEnhancementVolumeStyle &>(n)
                  .getOpacityFactor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BoundaryEnhancementVolumeStyle &>(n)
              .setOpacityFactorUnchecked(std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "retainedOpacity", X3DFieldType::SFFloat, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BoundaryEnhancementVolumeStyle &>(n)
                  .getRetainedOpacity());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BoundaryEnhancementVolumeStyle &>(n)
              .setRetainedOpacityUnchecked(std::any_cast<SFFloat>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BoundaryEnhancementVolumeStyle &>(n)
                  .X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BoundaryEnhancementVolumeStyle &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BoundaryEnhancementVolumeStyle &>(n)
                  .X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BoundaryEnhancementVolumeStyle &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BoundaryEnhancementVolumeStyle &>(n)
                  .X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BoundaryEnhancementVolumeStyle &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BoundaryEnhancementVolumeStyle &>(n)
                  .X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BoundaryEnhancementVolumeStyle &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const BoundaryEnhancementVolumeStyle &>(n)
                  .X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<BoundaryEnhancementVolumeStyle &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void BoundaryEnhancementVolumeStyle::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void BoundaryEnhancementVolumeStyle::validateRanges(
    std::vector<RangeDiagnostic> &out) const {

  checkRangesBoundaryOpacity(getBoundaryOpacity(), nodeTypeName(), "", out);

  checkRangesOpacityFactor(getOpacityFactor(), nodeTypeName(), "", out);

  checkRangesRetainedOpacity(getRetainedOpacity(), nodeTypeName(), "", out);
}

void BoundaryEnhancementVolumeStyle::checkRangesBoundaryOpacity(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "boundaryOpacity",
                                  "boundaryOpacity below minimum of 0"});
  if (value > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "boundaryOpacity",
                                  "boundaryOpacity above maximum of 1"});
}

void BoundaryEnhancementVolumeStyle::checkRangesOpacityFactor(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "opacityFactor",
                                  "opacityFactor below minimum of 0"});
}

void BoundaryEnhancementVolumeStyle::checkRangesRetainedOpacity(
    const SFFloat &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 0)
    out.push_back(RangeDiagnostic{nodeType, defName, "retainedOpacity",
                                  "retainedOpacity below minimum of 0"});
  if (value > 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "retainedOpacity",
                                  "retainedOpacity above maximum of 1"});
}
