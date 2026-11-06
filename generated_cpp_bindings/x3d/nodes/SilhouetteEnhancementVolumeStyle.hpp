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
  SilhouetteEnhancementVolumeStyle() = default;

  ~SilhouetteEnhancementVolumeStyle() = default;

  static SFFloat getDefaultSilhouetteBoundaryOpacity() { return 0; }

  static SFFloat getDefaultSilhouetteRetainedOpacity() { return 1; }

  static SFFloat getDefaultSilhouetteSharpness() { return 0.5; }

  static SFNode getDefaultSurfaceNormals() { return nullptr; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "renderStyle"; }

  static std::string componentName() { return "VolumeRendering"; }

  static int componentLevel() { return 2; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details amount of the silhouette enhancement to use.
   */
  const SFFloat &getSilhouetteBoundaryOpacity() const {
    return _silhouetteBoundaryOpacity;
  }

  /**
   * Access: inputOutput.
   * @details amount of the silhouette enhancement to use.
   */
  void setSilhouetteBoundaryOpacity(const SFFloat &value) {

    validateSilhouetteBoundaryOpacity(value);

    _silhouetteBoundaryOpacity = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setSilhouetteBoundaryOpacity() for range
   * validation.
   */
  void setSilhouetteBoundaryOpacityUnchecked(const SFFloat &value) {
    _silhouetteBoundaryOpacity = value;
  }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details scaling of non-silhouette regions.
   */
  const SFFloat &getSilhouetteRetainedOpacity() const {
    return _silhouetteRetainedOpacity;
  }

  /**
   * Access: inputOutput.
   * @details scaling of non-silhouette regions.
   */
  void setSilhouetteRetainedOpacity(const SFFloat &value) {

    validateSilhouetteRetainedOpacity(value);

    _silhouetteRetainedOpacity = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setSilhouetteRetainedOpacity() for range
   * validation.
   */
  void setSilhouetteRetainedOpacityUnchecked(const SFFloat &value) {
    _silhouetteRetainedOpacity = value;
  }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details power function to control sharpness of the silhouette.
   */
  const SFFloat &getSilhouetteSharpness() const { return _silhouetteSharpness; }

  /**
   * Access: inputOutput.
   * @details power function to control sharpness of the silhouette.
   */
  void setSilhouetteSharpness(const SFFloat &value) {

    validateSilhouetteSharpness(value);

    _silhouetteSharpness = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setSilhouetteSharpness() for range validation.
   */
  void setSilhouetteSharpnessUnchecked(const SFFloat &value) {
    _silhouetteSharpness = value;
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

  SFFloat _silhouetteBoundaryOpacity{0};

  SFFloat _silhouetteRetainedOpacity{1};

  SFFloat _silhouetteSharpness{0.5};

  SFNode _surfaceNormals{nullptr};
};

} // namespace x3d::nodes
