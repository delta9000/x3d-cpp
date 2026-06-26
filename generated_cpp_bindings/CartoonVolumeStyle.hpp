// CartoonVolumeStyle.hpp
#ifndef CARTOONVOLUMESTYLE_HPP
#define CARTOONVOLUMESTYLE_HPP

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
 * @class CartoonVolumeStyle
 * @brief CartoonVolumeStyle generates cartoon-style non-photorealistic
 * rendering of associated volumetric data.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/volume.html#CartoonVolumeStyle
 */
class CartoonVolumeStyle : public virtual X3DComposableVolumeRenderStyleNode {
public:
  /**
   * @brief Default constructor for CartoonVolumeStyle
   */
  CartoonVolumeStyle() = default;

  /**
   * @brief Destructor for CartoonVolumeStyle
   */
  ~CartoonVolumeStyle() = default;

  /**
   * @brief Get the default value for colorSteps
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultColorSteps() { return 4; }

  /**
   * @brief Get the default value for orthogonalColor
   * @return SFColorRGBA The default value
   */
  static SFColorRGBA getDefaultOrthogonalColor() {
    return SFColorRGBA{1, 1, 1, 1};
  }

  /**
   * @brief Get the default value for parallelColor
   * @return SFColorRGBA The default value
   */
  static SFColorRGBA getDefaultParallelColor() {
    return SFColorRGBA{0, 0, 0, 1};
  }

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
  static int componentLevel() { return 3; }

  /**
   * @brief Gets the value of colorSteps. AccessType: inputOutput
   * @details Number of distinct colors taken from interpolated colors and used
   * to render the object.
   * @return SFInt32 The current value of colorSteps.
   */
  SFInt32 getColorSteps() const { return _colorSteps; }

  /**
   * @brief Sets the value of colorSteps. AccessType: inputOutput
   * @details Number of distinct colors taken from interpolated colors and used
   * to render the object.
   * @param value The new value for colorSteps.
   */
  void setColorSteps(const SFInt32 &value) {

    validateColorSteps(value);

    _colorSteps = value;
  }

  /**
   * @brief Non-validating write of colorSteps (runtime/reader ingest path).
   * @details Assigns without the range check setColorSteps() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setColorSteps() stays the
   *          enforcement point for programmatic callers.
   */
  void setColorStepsUnchecked(const SFInt32 &value) { _colorSteps = value; }

  /**
   * @brief Gets the value of orthogonalColor. AccessType: inputOutput
   * @details orthogonalColor is used for surface normals that are orthogonal
   * (perpendicular) to viewer's current location.
   * @return SFColorRGBA The current value of orthogonalColor.
   */
  SFColorRGBA getOrthogonalColor() const { return _orthogonalColor; }

  /**
   * @brief Sets the value of orthogonalColor. AccessType: inputOutput
   * @details orthogonalColor is used for surface normals that are orthogonal
   * (perpendicular) to viewer's current location.
   * @param value The new value for orthogonalColor.
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
   * @brief Non-validating write of orthogonalColor (runtime/reader ingest
   * path).
   * @details Assigns without the range check setOrthogonalColor() enforces, so
   * a permissive reader keeps an out-of-range authored value rather than reject
   * the whole document. setOrthogonalColor() stays the enforcement point for
   * programmatic callers.
   */
  void setOrthogonalColorUnchecked(const SFColorRGBA &value) {
    _orthogonalColor = value;
  }

  /**
   * @brief Gets the value of parallelColor. AccessType: inputOutput
   * @details parallelColor is used for surface normals that are orthogonal to
   * viewer's current location.
   * @return SFColorRGBA The current value of parallelColor.
   */
  SFColorRGBA getParallelColor() const { return _parallelColor; }

  /**
   * @brief Sets the value of parallelColor. AccessType: inputOutput
   * @details parallelColor is used for surface normals that are orthogonal to
   * viewer's current location.
   * @param value The new value for parallelColor.
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
   * @brief Non-validating write of parallelColor (runtime/reader ingest path).
   * @details Assigns without the range check setParallelColor() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setParallelColor() stays the
   *          enforcement point for programmatic callers.
   */
  void setParallelColorUnchecked(const SFColorRGBA &value) {
    _parallelColor = value;
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
   * @brief The X3D type name of this node (e.g. "CartoonVolumeStyle").
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

    if (value.r < 0)
      throw std::out_of_range("orthogonalColor.r below minimum of 0");
    if (value.r > 1)
      throw std::out_of_range("orthogonalColor.r above maximum of 1");

    if (value.g < 0)
      throw std::out_of_range("orthogonalColor.g below minimum of 0");
    if (value.g > 1)
      throw std::out_of_range("orthogonalColor.g above maximum of 1");

    if (value.b < 0)
      throw std::out_of_range("orthogonalColor.b below minimum of 0");
    if (value.b > 1)
      throw std::out_of_range("orthogonalColor.b above maximum of 1");

    if (value.a < 0)
      throw std::out_of_range("orthogonalColor.a below minimum of 0");
    if (value.a > 1)
      throw std::out_of_range("orthogonalColor.a above maximum of 1");
  }

  static void validateParallelColor(const SFColorRGBA &value) {

    if (value.r < 0)
      throw std::out_of_range("parallelColor.r below minimum of 0");
    if (value.r > 1)
      throw std::out_of_range("parallelColor.r above maximum of 1");

    if (value.g < 0)
      throw std::out_of_range("parallelColor.g below minimum of 0");
    if (value.g > 1)
      throw std::out_of_range("parallelColor.g above maximum of 1");

    if (value.b < 0)
      throw std::out_of_range("parallelColor.b below minimum of 0");
    if (value.b > 1)
      throw std::out_of_range("parallelColor.b above maximum of 1");

    if (value.a < 0)
      throw std::out_of_range("parallelColor.a below minimum of 0");
    if (value.a > 1)
      throw std::out_of_range("parallelColor.a above maximum of 1");
  }

  /**
   * @brief Member variable for colorSteps.
   */

  SFInt32 _colorSteps{4};

  /**
   * @brief Member variable for orthogonalColor.
   */

  SFColorRGBA _orthogonalColor{SFColorRGBA{1, 1, 1, 1}};

  /**
   * @brief Member variable for parallelColor.
   */

  SFColorRGBA _parallelColor{SFColorRGBA{0, 0, 0, 1}};

  /**
   * @brief Member variable for surfaceNormals.
   */

  SFNode _surfaceNormals{nullptr};
};

#endif // CARTOONVOLUMESTYLE_HPP