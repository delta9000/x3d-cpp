// FillProperties.hpp
#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DAppearanceChildNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class FillProperties
 * @brief FillProperties indicates whether appearance is filled or hatched for
 * associated geometry nodes inside the same Shape.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shape.html#FillProperties
 */
class FillProperties : public virtual X3DAppearanceChildNode {
public:
  /**
   * @brief Default constructor for FillProperties
   */
  FillProperties() = default;

  /**
   * @brief Destructor for FillProperties
   */
  ~FillProperties() = default;

  /**
   * @brief Get the default value for filled
   * @return SFBool The default value
   */
  static SFBool getDefaultFilled() { return true; }

  /**
   * @brief Get the default value for hatchColor
   * @return SFColor The default value
   */
  static SFColor getDefaultHatchColor() { return SFColor{1, 1, 1}; }

  /**
   * @brief Get the default value for hatched
   * @return SFBool The default value
   */
  static SFBool getDefaultHatched() { return true; }

  /**
   * @brief Get the default value for hatchStyle
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultHatchStyle() { return 1; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "fillProperties"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Shape"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 3; }

  /**
   * @brief Gets the value of filled. AccessType: inputOutput
   * @details Whether or not associated geometry is filled.
   * @return SFBool The current value of filled.
   */
  SFBool getFilled() const { return _filled; }

  /**
   * @brief Sets the value of filled. AccessType: inputOutput
   * @details Whether or not associated geometry is filled.
   * @param value The new value for filled.
   */
  void setFilled(const SFBool &value) { _filled = value; }

  /**
   * @brief Gets the value of hatchColor. AccessType: inputOutput
   * @details Color of the hatch pattern.
   * @return SFColor The current value of hatchColor.
   */
  SFColor getHatchColor() const { return _hatchColor; }

  /**
   * @brief Sets the value of hatchColor. AccessType: inputOutput
   * @details Color of the hatch pattern.
   * @param value The new value for hatchColor.
   */
  void setHatchColor(const SFColor &value) {

    validateHatchColor(value);

    _hatchColor = value;
  }

  void setHatchColor(SFColor &&value) {

    validateHatchColor(value);

    _hatchColor = std::move(value);
  }

  /**
   * @brief Non-validating write of hatchColor (runtime/reader ingest path).
   * @details Assigns without the range check setHatchColor() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setHatchColor() stays the
   *          enforcement point for programmatic callers.
   */
  void setHatchColorUnchecked(const SFColor &value) { _hatchColor = value; }

  /**
   * @brief Gets the value of hatched. AccessType: inputOutput
   * @details Whether or not associated geometry is hatched.
   * @return SFBool The current value of hatched.
   */
  SFBool getHatched() const { return _hatched; }

  /**
   * @brief Sets the value of hatched. AccessType: inputOutput
   * @details Whether or not associated geometry is hatched.
   * @param value The new value for hatched.
   */
  void setHatched(const SFBool &value) { _hatched = value; }

  /**
   * @brief Gets the value of hatchStyle. AccessType: inputOutput
   * @details hatchStyle selects a hatch pattern from ISO/IEC 9973 International
   * Register of Graphical Items.
   * @return SFInt32 The current value of hatchStyle.
   */
  SFInt32 getHatchStyle() const { return _hatchStyle; }

  /**
   * @brief Sets the value of hatchStyle. AccessType: inputOutput
   * @details hatchStyle selects a hatch pattern from ISO/IEC 9973 International
   * Register of Graphical Items.
   * @param value The new value for hatchStyle.
   */
  void setHatchStyle(const SFInt32 &value) {

    validateHatchStyle(value);

    _hatchStyle = value;
  }

  /**
   * @brief Non-validating write of hatchStyle (runtime/reader ingest path).
   * @details Assigns without the range check setHatchStyle() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setHatchStyle() stays the
   *          enforcement point for programmatic callers.
   */
  void setHatchStyleUnchecked(const SFInt32 &value) { _hatchStyle = value; }

  /**
   * @brief The X3D type name of this node (e.g. "FillProperties").
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
  static void checkRangesHatchColor(const SFColor &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesHatchStyle(const SFInt32 &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out);

private:
  static void validateHatchColor(const SFColor &value) {

    if (value.r < 0.0f)
      throw std::out_of_range("hatchColor.r below minimum of 0");
    if (value.r > 1.0f)
      throw std::out_of_range("hatchColor.r above maximum of 1");

    if (value.g < 0.0f)
      throw std::out_of_range("hatchColor.g below minimum of 0");
    if (value.g > 1.0f)
      throw std::out_of_range("hatchColor.g above maximum of 1");

    if (value.b < 0.0f)
      throw std::out_of_range("hatchColor.b below minimum of 0");
    if (value.b > 1.0f)
      throw std::out_of_range("hatchColor.b above maximum of 1");
  }

  static void validateHatchStyle(const SFInt32 &value) {

    if (value < 0)
      throw std::out_of_range("hatchStyle below minimum of 0");
  }

  /**
   * @brief Member variable for filled.
   */

  SFBool _filled{true};

  /**
   * @brief Member variable for hatchColor.
   */

  SFColor _hatchColor{SFColor{1, 1, 1}};

  /**
   * @brief Member variable for hatched.
   */

  SFBool _hatched{true};

  /**
   * @brief Member variable for hatchStyle.
   */

  SFInt32 _hatchStyle{1};
};

} // namespace x3d::nodes
