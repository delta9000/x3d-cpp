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
  EdgeEnhancementVolumeStyle() = default;

  ~EdgeEnhancementVolumeStyle() = default;

  static SFColorRGBA getDefaultEdgeColor() { return SFColorRGBA{0, 0, 0, 1}; }

  static SFFloat getDefaultGradientThreshold() { return 0.4; }

  static SFNode getDefaultSurfaceNormals() { return nullptr; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "renderStyle"; }

  static std::string componentName() { return "VolumeRendering"; }

  static int componentLevel() { return 2; }

  /**
   * SFColorRGBA; access: inputOutput. Borrows the stored value.
   * @details color used to highlight edges.
   */
  const SFColorRGBA &getEdgeColor() const { return _edgeColor; }

  /**
   * Access: inputOutput.
   * @details color used to highlight edges.
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
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setEdgeColor() for range validation.
   */
  void setEdgeColorUnchecked(const SFColorRGBA &value) { _edgeColor = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details minimum angle (in radians) away from view-direction vector for
   * surface normal before applying enhancement.
   */
  const SFFloat &getGradientThreshold() const { return _gradientThreshold; }

  /**
   * Access: inputOutput.
   * @details minimum angle (in radians) away from view-direction vector for
   * surface normal before applying enhancement.
   */
  void setGradientThreshold(const SFFloat &value) {

    validateGradientThreshold(value);

    _gradientThreshold = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setGradientThreshold() for range validation.
   */
  void setGradientThresholdUnchecked(const SFFloat &value) {
    _gradientThreshold = value;
  }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details The surfaceNormals field contains a 3D texture with at least three
   * component values.
   */
  const SFNode &getSurfaceNormals() const { return _surfaceNormals; }

  /**
   * @details Permitted X3D node types: X3DTexture3DNode
   */
  static const std::vector<std::string> &acceptableSurfaceNormalsNodeTypes() {
    static const std::vector<std::string> types = {"X3DTexture3DNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details The surfaceNormals field contains a 3D texture with at least three
   * component values.
   */
  void setSurfaceNormals(const SFNode &value) { _surfaceNormals = value; }

  void setSurfaceNormals(SFNode &&value) { _surfaceNormals = std::move(value); }

  std::string nodeTypeName() const override;

  /**
   * Parent field used when an X3D-XML element omits containerField.
   */
  std::string defaultContainerField() const override;

  /**
   * Own and inherited fields; the table lives for the lifetime of the program.
   */
  const FieldTable &fields() const override;

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

    if (value.r < 0.0f)
      throw std::out_of_range("edgeColor.r below minimum of 0");
    if (value.r > 1.0f)
      throw std::out_of_range("edgeColor.r above maximum of 1");

    if (value.g < 0.0f)
      throw std::out_of_range("edgeColor.g below minimum of 0");
    if (value.g > 1.0f)
      throw std::out_of_range("edgeColor.g above maximum of 1");

    if (value.b < 0.0f)
      throw std::out_of_range("edgeColor.b below minimum of 0");
    if (value.b > 1.0f)
      throw std::out_of_range("edgeColor.b above maximum of 1");

    if (value.a < 0.0f)
      throw std::out_of_range("edgeColor.a below minimum of 0");
    if (value.a > 1.0f)
      throw std::out_of_range("edgeColor.a above maximum of 1");
  }

  static void validateGradientThreshold(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("gradientThreshold below minimum of 0");
    if (value > 3.1416f)
      throw std::out_of_range("gradientThreshold above maximum of 3.1416");
  }

  SFColorRGBA _edgeColor{SFColorRGBA{0, 0, 0, 1}};

  SFFloat _gradientThreshold{0.4};

  SFNode _surfaceNormals{nullptr};
};

} // namespace x3d::nodes
