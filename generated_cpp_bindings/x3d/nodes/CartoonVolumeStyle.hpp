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
 * @class CartoonVolumeStyle
 * @brief CartoonVolumeStyle generates cartoon-style non-photorealistic
 * rendering of associated volumetric data.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/volume.html#CartoonVolumeStyle
 */
class CartoonVolumeStyle : public virtual X3DComposableVolumeRenderStyleNode {
public:
  CartoonVolumeStyle() = default;

  ~CartoonVolumeStyle() = default;

  static SFInt32 getDefaultColorSteps() { return 4; }

  static SFColorRGBA getDefaultOrthogonalColor() {
    return SFColorRGBA{1, 1, 1, 1};
  }

  static SFColorRGBA getDefaultParallelColor() {
    return SFColorRGBA{0, 0, 0, 1};
  }

  static SFNode getDefaultSurfaceNormals() { return nullptr; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "renderStyle"; }

  static std::string componentName() { return "VolumeRendering"; }

  static int componentLevel() { return 3; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details Number of distinct colors taken from interpolated colors and used
   * to render the object.
   */
  const SFInt32 &getColorSteps() const { return _colorSteps; }

  /**
   * Access: inputOutput.
   * @details Number of distinct colors taken from interpolated colors and used
   * to render the object.
   */
  void setColorSteps(const SFInt32 &value) {

    validateColorSteps(value);

    _colorSteps = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setColorSteps() for range validation.
   */
  void setColorStepsUnchecked(const SFInt32 &value) { _colorSteps = value; }

  /**
   * SFColorRGBA; access: inputOutput. Borrows the stored value.
   * @details orthogonalColor is used for surface normals that are orthogonal
   * (perpendicular) to viewer's current location.
   */
  const SFColorRGBA &getOrthogonalColor() const { return _orthogonalColor; }

  /**
   * Access: inputOutput.
   * @details orthogonalColor is used for surface normals that are orthogonal
   * (perpendicular) to viewer's current location.
   */
  void setOrthogonalColor(const SFColorRGBA &value) {

    validateOrthogonalColor(value);

    _orthogonalColor = value;
  }

  void setOrthogonalColor(SFColorRGBA &&value) {

    validateOrthogonalColor(value);

    _orthogonalColor = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setOrthogonalColor() for range validation.
   */
  void setOrthogonalColorUnchecked(const SFColorRGBA &value) {
    _orthogonalColor = value;
  }

  /**
   * SFColorRGBA; access: inputOutput. Borrows the stored value.
   * @details parallelColor is used for surface normals that are orthogonal to
   * viewer's current location.
   */
  const SFColorRGBA &getParallelColor() const { return _parallelColor; }

  /**
   * Access: inputOutput.
   * @details parallelColor is used for surface normals that are orthogonal to
   * viewer's current location.
   */
  void setParallelColor(const SFColorRGBA &value) {

    validateParallelColor(value);

    _parallelColor = value;
  }

  void setParallelColor(SFColorRGBA &&value) {

    validateParallelColor(value);

    _parallelColor = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setParallelColor() for range validation.
   */
  void setParallelColorUnchecked(const SFColorRGBA &value) {
    _parallelColor = value;
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
  static void checkRangesColorSteps(const SFInt32 &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesOrthogonalColor(const SFColorRGBA &value,
                                         const std::string &nodeType,
                                         const std::string &defName,
                                         std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesParallelColor(const SFColorRGBA &value,
                                       const std::string &nodeType,
                                       const std::string &defName,
                                       std::vector<RangeDiagnostic> &out);

private:
  static void validateColorSteps(const SFInt32 &value) {

    if (value < 1)
      throw std::out_of_range("colorSteps below minimum of 1");
    if (value > 64)
      throw std::out_of_range("colorSteps above maximum of 64");
  }

  static void validateOrthogonalColor(const SFColorRGBA &value) {

    if (value.r < 0.0f)
      throw std::out_of_range("orthogonalColor.r below minimum of 0");
    if (value.r > 1.0f)
      throw std::out_of_range("orthogonalColor.r above maximum of 1");

    if (value.g < 0.0f)
      throw std::out_of_range("orthogonalColor.g below minimum of 0");
    if (value.g > 1.0f)
      throw std::out_of_range("orthogonalColor.g above maximum of 1");

    if (value.b < 0.0f)
      throw std::out_of_range("orthogonalColor.b below minimum of 0");
    if (value.b > 1.0f)
      throw std::out_of_range("orthogonalColor.b above maximum of 1");

    if (value.a < 0.0f)
      throw std::out_of_range("orthogonalColor.a below minimum of 0");
    if (value.a > 1.0f)
      throw std::out_of_range("orthogonalColor.a above maximum of 1");
  }

  static void validateParallelColor(const SFColorRGBA &value) {

    if (value.r < 0.0f)
      throw std::out_of_range("parallelColor.r below minimum of 0");
    if (value.r > 1.0f)
      throw std::out_of_range("parallelColor.r above maximum of 1");

    if (value.g < 0.0f)
      throw std::out_of_range("parallelColor.g below minimum of 0");
    if (value.g > 1.0f)
      throw std::out_of_range("parallelColor.g above maximum of 1");

    if (value.b < 0.0f)
      throw std::out_of_range("parallelColor.b below minimum of 0");
    if (value.b > 1.0f)
      throw std::out_of_range("parallelColor.b above maximum of 1");

    if (value.a < 0.0f)
      throw std::out_of_range("parallelColor.a below minimum of 0");
    if (value.a > 1.0f)
      throw std::out_of_range("parallelColor.a above maximum of 1");
  }

  SFInt32 _colorSteps{4};

  SFColorRGBA _orthogonalColor{SFColorRGBA{1, 1, 1, 1}};

  SFColorRGBA _parallelColor{SFColorRGBA{0, 0, 0, 1}};

  SFNode _surfaceNormals{nullptr};
};

} // namespace x3d::nodes
