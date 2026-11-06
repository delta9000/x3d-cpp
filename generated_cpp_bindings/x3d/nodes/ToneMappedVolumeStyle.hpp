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
 * @class ToneMappedVolumeStyle
 * @brief ToneMappedVolumeStyle specifies that volumetric data is rendered with
 * Gooch shading model of two-toned warm/cool coloring.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/volume.html#ToneMappedVolumeStyle
 */
class ToneMappedVolumeStyle
    : public virtual X3DComposableVolumeRenderStyleNode {
public:
  ToneMappedVolumeStyle() = default;

  ~ToneMappedVolumeStyle() = default;

  static SFColorRGBA getDefaultCoolColor() { return SFColorRGBA{0, 0, 1, 0}; }

  static SFNode getDefaultSurfaceNormals() { return nullptr; }

  static SFColorRGBA getDefaultWarmColor() { return SFColorRGBA{1, 1, 0, 0}; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "renderStyle"; }

  static std::string componentName() { return "VolumeRendering"; }

  static int componentLevel() { return 2; }

  /**
   * SFColorRGBA; access: inputOutput. Borrows the stored value.
   * @details coolColor is used for surfaces facing away from the light
   * direction.
   */
  const SFColorRGBA &getCoolColor() const { return _coolColor; }

  /**
   * Access: inputOutput.
   * @details coolColor is used for surfaces facing away from the light
   * direction.
   */
  void setCoolColor(const SFColorRGBA &value) {

    validateCoolColor(value);

    _coolColor = value;
  }

  void setCoolColor(SFColorRGBA &&value) {

    validateCoolColor(value);

    _coolColor = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setCoolColor() for range validation.
   */
  void setCoolColorUnchecked(const SFColorRGBA &value) { _coolColor = value; }

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

  /**
   * SFColorRGBA; access: inputOutput. Borrows the stored value.
   * @details warmColor is used for surfaces facing towards the light.
   */
  const SFColorRGBA &getWarmColor() const { return _warmColor; }

  /**
   * Access: inputOutput.
   * @details warmColor is used for surfaces facing towards the light.
   */
  void setWarmColor(const SFColorRGBA &value) {

    validateWarmColor(value);

    _warmColor = value;
  }

  void setWarmColor(SFColorRGBA &&value) {

    validateWarmColor(value);

    _warmColor = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setWarmColor() for range validation.
   */
  void setWarmColorUnchecked(const SFColorRGBA &value) { _warmColor = value; }

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
  static void checkRangesCoolColor(const SFColorRGBA &value,
                                   const std::string &nodeType,
                                   const std::string &defName,
                                   std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesWarmColor(const SFColorRGBA &value,
                                   const std::string &nodeType,
                                   const std::string &defName,
                                   std::vector<RangeDiagnostic> &out);

private:
  static void validateCoolColor(const SFColorRGBA &value) {

    if (value.r < 0.0f)
      throw std::out_of_range("coolColor.r below minimum of 0");
    if (value.r > 1.0f)
      throw std::out_of_range("coolColor.r above maximum of 1");

    if (value.g < 0.0f)
      throw std::out_of_range("coolColor.g below minimum of 0");
    if (value.g > 1.0f)
      throw std::out_of_range("coolColor.g above maximum of 1");

    if (value.b < 0.0f)
      throw std::out_of_range("coolColor.b below minimum of 0");
    if (value.b > 1.0f)
      throw std::out_of_range("coolColor.b above maximum of 1");

    if (value.a < 0.0f)
      throw std::out_of_range("coolColor.a below minimum of 0");
    if (value.a > 1.0f)
      throw std::out_of_range("coolColor.a above maximum of 1");
  }

  static void validateWarmColor(const SFColorRGBA &value) {

    if (value.r < 0.0f)
      throw std::out_of_range("warmColor.r below minimum of 0");
    if (value.r > 1.0f)
      throw std::out_of_range("warmColor.r above maximum of 1");

    if (value.g < 0.0f)
      throw std::out_of_range("warmColor.g below minimum of 0");
    if (value.g > 1.0f)
      throw std::out_of_range("warmColor.g above maximum of 1");

    if (value.b < 0.0f)
      throw std::out_of_range("warmColor.b below minimum of 0");
    if (value.b > 1.0f)
      throw std::out_of_range("warmColor.b above maximum of 1");

    if (value.a < 0.0f)
      throw std::out_of_range("warmColor.a below minimum of 0");
    if (value.a > 1.0f)
      throw std::out_of_range("warmColor.a above maximum of 1");
  }

  SFColorRGBA _coolColor{SFColorRGBA{0, 0, 1, 0}};

  SFNode _surfaceNormals{nullptr};

  SFColorRGBA _warmColor{SFColorRGBA{1, 1, 0, 0}};
};

} // namespace x3d::nodes
