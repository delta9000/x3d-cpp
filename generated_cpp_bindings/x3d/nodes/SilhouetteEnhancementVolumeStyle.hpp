// SilhouetteEnhancementVolumeStyle.hpp
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
 * @class SilhouetteEnhancementVolumeStyle
 * @brief SilhouetteEnhancementVolumeStyle specifies that volumetric data is
 * rendered with silhouette enhancement.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/volume.html#SilhouetteEnhancementVolumeStyle
 */
class SilhouetteEnhancementVolumeStyle
    : public virtual X3DComposableVolumeRenderStyleNode {
public:
  /**
   * @brief Default constructor for SilhouetteEnhancementVolumeStyle
   */
  SilhouetteEnhancementVolumeStyle() = default;

  /**
   * @brief Destructor for SilhouetteEnhancementVolumeStyle
   */
  ~SilhouetteEnhancementVolumeStyle() = default;

  /**
   * @brief Get the default value for silhouetteBoundaryOpacity
   * @return SFFloat The default value
   */
  static SFFloat getDefaultSilhouetteBoundaryOpacity() { return 0; }

  /**
   * @brief Get the default value for silhouetteRetainedOpacity
   * @return SFFloat The default value
   */
  static SFFloat getDefaultSilhouetteRetainedOpacity() { return 1; }

  /**
   * @brief Get the default value for silhouetteSharpness
   * @return SFFloat The default value
   */
  static SFFloat getDefaultSilhouetteSharpness() { return 0.5; }

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
   * @brief Gets the value of silhouetteBoundaryOpacity. AccessType: inputOutput
   * @details amount of the silhouette enhancement to use.
   * @return SFFloat The current value of silhouetteBoundaryOpacity.
   */
  SFFloat getSilhouetteBoundaryOpacity() const {
    return _silhouetteBoundaryOpacity;
  }

  /**
   * @brief Sets the value of silhouetteBoundaryOpacity. AccessType: inputOutput
   * @details amount of the silhouette enhancement to use.
   * @param value The new value for silhouetteBoundaryOpacity.
   */
  void setSilhouetteBoundaryOpacity(const SFFloat &value) {

    validateSilhouetteBoundaryOpacity(value);

    _silhouetteBoundaryOpacity = value;
  }

  /**
   * @brief Non-validating write of silhouetteBoundaryOpacity (runtime/reader
   * ingest path).
   * @details Assigns without the range check setSilhouetteBoundaryOpacity()
   * enforces, so a permissive reader keeps an out-of-range authored value
   * rather than reject the whole document. setSilhouetteBoundaryOpacity() stays
   * the enforcement point for programmatic callers.
   */
  void setSilhouetteBoundaryOpacityUnchecked(const SFFloat &value) {
    _silhouetteBoundaryOpacity = value;
  }

  /**
   * @brief Gets the value of silhouetteRetainedOpacity. AccessType: inputOutput
   * @details scaling of non-silhouette regions.
   * @return SFFloat The current value of silhouetteRetainedOpacity.
   */
  SFFloat getSilhouetteRetainedOpacity() const {
    return _silhouetteRetainedOpacity;
  }

  /**
   * @brief Sets the value of silhouetteRetainedOpacity. AccessType: inputOutput
   * @details scaling of non-silhouette regions.
   * @param value The new value for silhouetteRetainedOpacity.
   */
  void setSilhouetteRetainedOpacity(const SFFloat &value) {

    validateSilhouetteRetainedOpacity(value);

    _silhouetteRetainedOpacity = value;
  }

  /**
   * @brief Non-validating write of silhouetteRetainedOpacity (runtime/reader
   * ingest path).
   * @details Assigns without the range check setSilhouetteRetainedOpacity()
   * enforces, so a permissive reader keeps an out-of-range authored value
   * rather than reject the whole document. setSilhouetteRetainedOpacity() stays
   * the enforcement point for programmatic callers.
   */
  void setSilhouetteRetainedOpacityUnchecked(const SFFloat &value) {
    _silhouetteRetainedOpacity = value;
  }

  /**
   * @brief Gets the value of silhouetteSharpness. AccessType: inputOutput
   * @details power function to control sharpness of the silhouette.
   * @return SFFloat The current value of silhouetteSharpness.
   */
  SFFloat getSilhouetteSharpness() const { return _silhouetteSharpness; }

  /**
   * @brief Sets the value of silhouetteSharpness. AccessType: inputOutput
   * @details power function to control sharpness of the silhouette.
   * @param value The new value for silhouetteSharpness.
   */
  void setSilhouetteSharpness(const SFFloat &value) {

    validateSilhouetteSharpness(value);

    _silhouetteSharpness = value;
  }

  /**
   * @brief Non-validating write of silhouetteSharpness (runtime/reader ingest
   * path).
   * @details Assigns without the range check setSilhouetteSharpness() enforces,
   * so a permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setSilhouetteSharpness() stays the
   *          enforcement point for programmatic callers.
   */
  void setSilhouetteSharpnessUnchecked(const SFFloat &value) {
    _silhouetteSharpness = value;
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
   * @brief The X3D type name of this node (e.g.
   * "SilhouetteEnhancementVolumeStyle").
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
  static void checkRangesSilhouetteBoundaryOpacity(
      const SFFloat &value, const std::string &nodeType,
      const std::string &defName, std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesSilhouetteRetainedOpacity(
      const SFFloat &value, const std::string &nodeType,
      const std::string &defName, std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesSilhouetteSharpness(const SFFloat &value,
                                             const std::string &nodeType,
                                             const std::string &defName,
                                             std::vector<RangeDiagnostic> &out);

private:
  static void validateSilhouetteBoundaryOpacity(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("silhouetteBoundaryOpacity below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("silhouetteBoundaryOpacity above maximum of 1");
  }

  static void validateSilhouetteRetainedOpacity(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("silhouetteRetainedOpacity below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("silhouetteRetainedOpacity above maximum of 1");
  }

  static void validateSilhouetteSharpness(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("silhouetteSharpness below minimum of 0");
  }

  /**
   * @brief Member variable for silhouetteBoundaryOpacity.
   */

  SFFloat _silhouetteBoundaryOpacity{0};

  /**
   * @brief Member variable for silhouetteRetainedOpacity.
   */

  SFFloat _silhouetteRetainedOpacity{1};

  /**
   * @brief Member variable for silhouetteSharpness.
   */

  SFFloat _silhouetteSharpness{0.5};

  /**
   * @brief Member variable for surfaceNormals.
   */

  SFNode _surfaceNormals{nullptr};
};

} // namespace x3d::nodes
