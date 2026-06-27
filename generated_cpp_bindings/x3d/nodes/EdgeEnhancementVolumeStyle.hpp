// EdgeEnhancementVolumeStyle.hpp
#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DVolumeRenderStyleNode.hpp"

#include "x3d/nodes/X3DComposableVolumeRenderStyleNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class EdgeEnhancementVolumeStyle
 * @brief EdgeEnhancementVolumeStyle specifies edge enhancement for the volume
 * rendering style.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/volume.html#EdgeEnhancementVolumeStyle
 */
class EdgeEnhancementVolumeStyle
    : public virtual X3DComposableVolumeRenderStyleNode {
public:
  /**
   * @brief Default constructor for EdgeEnhancementVolumeStyle
   */
  EdgeEnhancementVolumeStyle() = default;

  /**
   * @brief Destructor for EdgeEnhancementVolumeStyle
   */
  ~EdgeEnhancementVolumeStyle() = default;

  /**
   * @brief Get the default value for edgeColor
   * @return SFColorRGBA The default value
   */
  static SFColorRGBA getDefaultEdgeColor() { return SFColorRGBA{0, 0, 0, 1}; }

  /**
   * @brief Get the default value for gradientThreshold
   * @return SFFloat The default value
   */
  static SFFloat getDefaultGradientThreshold() { return 0.4; }

  /**
   * @brief Get the default value for surfaceNormals
   * @return SFNode The default value
   */
  static SFNode getDefaultSurfaceNormals() { return nullptr; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "renderStyle"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "VolumeRendering"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of edgeColor. AccessType: inputOutput
   * @details color used to highlight edges.
   * @return SFColorRGBA The current value of edgeColor.
   */
  SFColorRGBA getEdgeColor() const { return _edgeColor; }

  /**
   * @brief Sets the value of edgeColor. AccessType: inputOutput
   * @details color used to highlight edges.
   * @param value The new value for edgeColor.
   */
  void setEdgeColor(const SFColorRGBA &value) {

    validateEdgeColor(value);

    _edgeColor = value;
  }

  void setEdgeColor(SFColorRGBA &&value) {

    validateEdgeColor(value);

    _edgeColor = std::move(value);
  }

  /**
   * @brief Non-validating write of edgeColor (runtime/reader ingest path).
   * @details Assigns without the range check setEdgeColor() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setEdgeColor() stays the
   *          enforcement point for programmatic callers.
   */
  void setEdgeColorUnchecked(const SFColorRGBA &value) { _edgeColor = value; }

  /**
   * @brief Gets the value of gradientThreshold. AccessType: inputOutput
   * @details minimum angle (in radians) away from view-direction vector for
   * surface normal before applying enhancement.
   * @return SFFloat The current value of gradientThreshold.
   */
  SFFloat getGradientThreshold() const { return _gradientThreshold; }

  /**
   * @brief Sets the value of gradientThreshold. AccessType: inputOutput
   * @details minimum angle (in radians) away from view-direction vector for
   * surface normal before applying enhancement.
   * @param value The new value for gradientThreshold.
   */
  void setGradientThreshold(const SFFloat &value) {

    validateGradientThreshold(value);

    _gradientThreshold = value;
  }

  /**
   * @brief Non-validating write of gradientThreshold (runtime/reader ingest
   * path).
   * @details Assigns without the range check setGradientThreshold() enforces,
   * so a permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setGradientThreshold() stays the
   *          enforcement point for programmatic callers.
   */
  void setGradientThresholdUnchecked(const SFFloat &value) {
    _gradientThreshold = value;
  }

  /**
   * @brief Gets the value of surfaceNormals. AccessType: inputOutput
   * @details The surfaceNormals field contains a 3D texture with at least three
   * component values.
   * @return SFNode The current value of surfaceNormals.
   */
  SFNode getSurfaceNormals() const { return _surfaceNormals; }

  /**
   * @brief Acceptable node types for the surfaceNormals field.
   * @details Permitted X3D node types: X3DTexture3DNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableSurfaceNormalsNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture3DNode"};
    return types;
  }

  /**
   * @brief Sets the value of surfaceNormals. AccessType: inputOutput
   * @details The surfaceNormals field contains a 3D texture with at least three
   * component values.
   * @param value The new value for surfaceNormals.
   */
  void setSurfaceNormals(const SFNode &value) { _surfaceNormals = value; }

  void setSurfaceNormals(SFNode &&value) { _surfaceNormals = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "EdgeEnhancementVolumeStyle").
   */
  std::string nodeTypeName() const override;

  /**
   * @brief This node's default containerField: the parent field it attaches
   *        to when an X3D-XML element gives no explicit containerField. Virtual
   *        so codecs can resolve it polymorphically through an X3DNode base
   *        pointer (the static getDefaultContainerField() is not reachable that
   *        way). Mirrors getDefaultContainerField().
   */
  std::string defaultContainerField() const override;

  /**
   * @brief Reflected field table for this node (own + inherited fields).
   * @details Built once (function-local static) from this node's descriptors.
   *          Each FieldInfo carries type-erased get/set thunks bound to this
   *          node's strongly-typed accessors so codecs need no per-node code.
   */
  const FieldTable &fields() const override;

  /**
   * @brief Visitor double-dispatch entry point.
   */
  void accept(NodeVisitor &visitor) const override;

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

protected:
  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesEdgeColor(const SFColorRGBA &value,
                                   const std::string &nodeType,
                                   const std::string &defName,
                                   std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesGradientThreshold(const SFFloat &value,
                                           const std::string &nodeType,
                                           const std::string &defName,
                                           std::vector<RangeDiagnostic> &out);

private:
  static void validateEdgeColor(const SFColorRGBA &value) {

    if (value.r < 0)
      throw std::out_of_range("edgeColor.r below minimum of 0");
    if (value.r > 1)
      throw std::out_of_range("edgeColor.r above maximum of 1");

    if (value.g < 0)
      throw std::out_of_range("edgeColor.g below minimum of 0");
    if (value.g > 1)
      throw std::out_of_range("edgeColor.g above maximum of 1");

    if (value.b < 0)
      throw std::out_of_range("edgeColor.b below minimum of 0");
    if (value.b > 1)
      throw std::out_of_range("edgeColor.b above maximum of 1");

    if (value.a < 0)
      throw std::out_of_range("edgeColor.a below minimum of 0");
    if (value.a > 1)
      throw std::out_of_range("edgeColor.a above maximum of 1");
  }

  static void validateGradientThreshold(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("gradientThreshold below minimum of 0");
    if (value > 3.1416)
      throw std::out_of_range("gradientThreshold above maximum of 3.1416");
  }

  /**
   * @brief Member variable for edgeColor.
   */

  SFColorRGBA _edgeColor{SFColorRGBA{0, 0, 0, 1}};

  /**
   * @brief Member variable for gradientThreshold.
   */

  SFFloat _gradientThreshold{0.4};

  /**
   * @brief Member variable for surfaceNormals.
   */

  SFNode _surfaceNormals{nullptr};
};

} // namespace x3d::nodes
