// ToneMappedVolumeStyle.hpp
#ifndef TONEMAPPEDVOLUMESTYLE_HPP
#define TONEMAPPEDVOLUMESTYLE_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DVolumeRenderStyleNode.hpp"

#include "X3DComposableVolumeRenderStyleNode.hpp"

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
  /**
   * @brief Default constructor for ToneMappedVolumeStyle
   */
  ToneMappedVolumeStyle() = default;

  /**
   * @brief Destructor for ToneMappedVolumeStyle
   */
  ~ToneMappedVolumeStyle() = default;

  /**
   * @brief Get the default value for coolColor
   * @return SFColorRGBA The default value
   */
  static SFColorRGBA getDefaultCoolColor() { return SFColorRGBA{0, 0, 1, 0}; }

  /**
   * @brief Get the default value for surfaceNormals
   * @return SFNode The default value
   */
  static SFNode getDefaultSurfaceNormals() { return nullptr; }

  /**
   * @brief Get the default value for warmColor
   * @return SFColorRGBA The default value
   */
  static SFColorRGBA getDefaultWarmColor() { return SFColorRGBA{1, 1, 0, 0}; }

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
   * @brief Gets the value of coolColor. AccessType: inputOutput
   * @details coolColor is used for surfaces facing away from the light
   * direction.
   * @return SFColorRGBA The current value of coolColor.
   */
  SFColorRGBA getCoolColor() const { return _coolColor; }

  /**
   * @brief Sets the value of coolColor. AccessType: inputOutput
   * @details coolColor is used for surfaces facing away from the light
   * direction.
   * @param value The new value for coolColor.
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
   * @brief Non-validating write of coolColor (runtime/reader ingest path).
   * @details Assigns without the range check setCoolColor() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setCoolColor() stays the
   *          enforcement point for programmatic callers.
   */
  void setCoolColorUnchecked(const SFColorRGBA &value) { _coolColor = value; }

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
   * @brief Gets the value of warmColor. AccessType: inputOutput
   * @details warmColor is used for surfaces facing towards the light.
   * @return SFColorRGBA The current value of warmColor.
   */
  SFColorRGBA getWarmColor() const { return _warmColor; }

  /**
   * @brief Sets the value of warmColor. AccessType: inputOutput
   * @details warmColor is used for surfaces facing towards the light.
   * @param value The new value for warmColor.
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
   * @brief Non-validating write of warmColor (runtime/reader ingest path).
   * @details Assigns without the range check setWarmColor() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setWarmColor() stays the
   *          enforcement point for programmatic callers.
   */
  void setWarmColorUnchecked(const SFColorRGBA &value) { _warmColor = value; }

  /**
   * @brief The X3D type name of this node (e.g. "ToneMappedVolumeStyle").
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

    if (value.r < 0)
      throw std::out_of_range("coolColor.r below minimum of 0");
    if (value.r > 1)
      throw std::out_of_range("coolColor.r above maximum of 1");

    if (value.g < 0)
      throw std::out_of_range("coolColor.g below minimum of 0");
    if (value.g > 1)
      throw std::out_of_range("coolColor.g above maximum of 1");

    if (value.b < 0)
      throw std::out_of_range("coolColor.b below minimum of 0");
    if (value.b > 1)
      throw std::out_of_range("coolColor.b above maximum of 1");

    if (value.a < 0)
      throw std::out_of_range("coolColor.a below minimum of 0");
    if (value.a > 1)
      throw std::out_of_range("coolColor.a above maximum of 1");
  }

  static void validateWarmColor(const SFColorRGBA &value) {

    if (value.r < 0)
      throw std::out_of_range("warmColor.r below minimum of 0");
    if (value.r > 1)
      throw std::out_of_range("warmColor.r above maximum of 1");

    if (value.g < 0)
      throw std::out_of_range("warmColor.g below minimum of 0");
    if (value.g > 1)
      throw std::out_of_range("warmColor.g above maximum of 1");

    if (value.b < 0)
      throw std::out_of_range("warmColor.b below minimum of 0");
    if (value.b > 1)
      throw std::out_of_range("warmColor.b above maximum of 1");

    if (value.a < 0)
      throw std::out_of_range("warmColor.a below minimum of 0");
    if (value.a > 1)
      throw std::out_of_range("warmColor.a above maximum of 1");
  }

  /**
   * @brief Member variable for coolColor.
   */

  SFColorRGBA _coolColor{SFColorRGBA{0, 0, 1, 0}};

  /**
   * @brief Member variable for surfaceNormals.
   */

  SFNode _surfaceNormals{nullptr};

  /**
   * @brief Member variable for warmColor.
   */

  SFColorRGBA _warmColor{SFColorRGBA{1, 1, 0, 0}};
};

#endif // TONEMAPPEDVOLUMESTYLE_HPP