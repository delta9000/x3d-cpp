// CartoonVolumeStyle.cpp
#include "x3d/nodes/CartoonVolumeStyle.hpp"

namespace x3d::nodes {
using namespace x3d::core;

std::string CartoonVolumeStyle::nodeTypeName() const {
  return "CartoonVolumeStyle";
}

std::string CartoonVolumeStyle::defaultContainerField() const {
  return getDefaultContainerField();
}

const FieldTable &CartoonVolumeStyle::fields() const {
  static const FieldTable table = [] {
    FieldTable t;

    t.push_back(FieldInfo{
        "colorSteps", X3DFieldType::SFInt32, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CartoonVolumeStyle &>(n).getColorSteps());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CartoonVolumeStyle &>(n).setColorStepsUnchecked(
              std::any_cast<SFInt32>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "enabled", X3DFieldType::SFBool, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const CartoonVolumeStyle &>(n)
                              .X3DVolumeRenderStyleNode::getEnabled());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CartoonVolumeStyle &>(n)
              .X3DVolumeRenderStyleNode::setEnabled(std::any_cast<SFBool>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "IS", X3DFieldType::SFNode, AccessType::InputOutput, "IS",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CartoonVolumeStyle &>(n).X3DNode::getIS());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CartoonVolumeStyle &>(n).X3DNode::setIS(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "metadata", X3DFieldType::SFNode, AccessType::InputOutput, "metadata",

        [](const X3DNode &n) -> std::any {
          return std::any(dynamic_cast<const CartoonVolumeStyle &>(n)
                              .X3DNode::getMetadata());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CartoonVolumeStyle &>(n).X3DNode::setMetadata(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "orthogonalColor", X3DFieldType::SFColorRGBA, AccessType::InputOutput,
        "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CartoonVolumeStyle &>(n).getOrthogonalColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CartoonVolumeStyle &>(n).setOrthogonalColorUnchecked(
              std::any_cast<SFColorRGBA>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "parallelColor", X3DFieldType::SFColorRGBA, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CartoonVolumeStyle &>(n).getParallelColor());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CartoonVolumeStyle &>(n).setParallelColorUnchecked(
              std::any_cast<SFColorRGBA>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "surfaceNormals", X3DFieldType::SFNode, AccessType::InputOutput,
        "surfaceNormals",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CartoonVolumeStyle &>(n).getSurfaceNormals());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CartoonVolumeStyle &>(n).setSurfaceNormals(
              std::any_cast<SFNode>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "DEF", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CartoonVolumeStyle &>(n).X3DNode::getDEF());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CartoonVolumeStyle &>(n).X3DNode::setDEF(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "USE", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CartoonVolumeStyle &>(n).X3DNode::getUSE());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CartoonVolumeStyle &>(n).X3DNode::setUSE(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "class", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CartoonVolumeStyle &>(n).X3DNode::getClass_());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CartoonVolumeStyle &>(n).X3DNode::setClass_(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "id", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CartoonVolumeStyle &>(n).X3DNode::getId());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CartoonVolumeStyle &>(n).X3DNode::setId(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    t.push_back(FieldInfo{
        "style", X3DFieldType::SFString, AccessType::InputOutput, "",

        [](const X3DNode &n) -> std::any {
          return std::any(
              dynamic_cast<const CartoonVolumeStyle &>(n).X3DNode::getStyle());
        },

        [](X3DNode &n, const std::any &v) {
          dynamic_cast<CartoonVolumeStyle &>(n).X3DNode::setStyle(
              std::any_cast<SFString>(v));
        },

        nullptr, nullptr

    });

    return t;
  }();
  return table;
}

void CartoonVolumeStyle::accept(NodeVisitor &visitor) const {
  if (!visitor.enter(*this)) {
    return;
  }
  visitor.leave(*this);
}

void CartoonVolumeStyle::validateRanges(
    std::vector<RangeDiagnostic> &out) const {

  checkRangesColorSteps(getColorSteps(), nodeTypeName(), "", out);

  checkRangesOrthogonalColor(getOrthogonalColor(), nodeTypeName(), "", out);

  checkRangesParallelColor(getParallelColor(), nodeTypeName(), "", out);
}

void CartoonVolumeStyle::checkRangesColorSteps(
    const SFInt32 &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value < 1)
    out.push_back(RangeDiagnostic{nodeType, defName, "colorSteps",
                                  "colorSteps below minimum of 1"});
  if (value > 64)
    out.push_back(RangeDiagnostic{nodeType, defName, "colorSteps",
                                  "colorSteps above maximum of 64"});
}

void CartoonVolumeStyle::checkRangesOrthogonalColor(
    const SFColorRGBA &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value.r < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "orthogonalColor",
                                  "orthogonalColor.r below minimum of 0"});
  if (value.r > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "orthogonalColor",
                                  "orthogonalColor.r above maximum of 1"});

  if (value.g < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "orthogonalColor",
                                  "orthogonalColor.g below minimum of 0"});
  if (value.g > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "orthogonalColor",
                                  "orthogonalColor.g above maximum of 1"});

  if (value.b < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "orthogonalColor",
                                  "orthogonalColor.b below minimum of 0"});
  if (value.b > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "orthogonalColor",
                                  "orthogonalColor.b above maximum of 1"});

  if (value.a < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "orthogonalColor",
                                  "orthogonalColor.a below minimum of 0"});
  if (value.a > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "orthogonalColor",
                                  "orthogonalColor.a above maximum of 1"});
}

void CartoonVolumeStyle::checkRangesParallelColor(
    const SFColorRGBA &value, const std::string &nodeType,
    const std::string &defName, std::vector<RangeDiagnostic> &out) {
  if (value.r < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "parallelColor",
                                  "parallelColor.r below minimum of 0"});
  if (value.r > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "parallelColor",
                                  "parallelColor.r above maximum of 1"});

  if (value.g < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "parallelColor",
                                  "parallelColor.g below minimum of 0"});
  if (value.g > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "parallelColor",
                                  "parallelColor.g above maximum of 1"});

  if (value.b < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "parallelColor",
                                  "parallelColor.b below minimum of 0"});
  if (value.b > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "parallelColor",
                                  "parallelColor.b above maximum of 1"});

  if (value.a < 0.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "parallelColor",
                                  "parallelColor.a below minimum of 0"});
  if (value.a > 1.0f)
    out.push_back(RangeDiagnostic{nodeType, defName, "parallelColor",
                                  "parallelColor.a above maximum of 1"});
}

namespace factory_detail {
std::shared_ptr<X3DNode> createCartoonVolumeStyle() {
  return std::make_shared<CartoonVolumeStyle>();
}
} // namespace factory_detail

} // namespace x3d::nodes
