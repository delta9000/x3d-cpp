// X3DFogObject.hpp
#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DFogObject
 * @brief Abstract type describing a node that influences the lighting equation
 * through the use of fog semantics.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/environmentalEffects.html#X3DFogOjbect
 */
class X3DFogObject {
public:
  /**
   * @brief Default constructor for X3DFogObject
   */
  X3DFogObject() = default;

  /**
   * @brief Virtual destructor for X3DFogObject
   */
  virtual ~X3DFogObject() = default;

  /**
   * @brief Get the default value for color
   * @return SFColor The default value
   */
  static SFColor getDefaultColor() { return SFColor{1, 1, 1}; }

  /**
   * @brief Get the default value for fogType
   * @return FogTypeChoices The default value
   */
  static FogTypeChoices getDefaultFogType() { return FogTypeChoices::LINEAR; }

  /**
   * @brief Get the default value for visibilityRange
   * @return SFFloat The default value
   */
  static SFFloat getDefaultVisibilityRange() { return 0; }

  /**
   * @brief Get the default container field value
   * @details No explicit containerField in the spec for this node; falls back
   *          to the X3D default field "children".
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "children"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "EnvironmentalEffects"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of color. AccessType: inputOutput
   * @details
   * @return SFColor The current value of color.
   */
  SFColor getColor() const { return _color; }

  /**
   * @brief Sets the value of color. AccessType: inputOutput
   * @details
   * @param value The new value for color.
   */
  void setColor(const SFColor &value) {

    validateColor(value);

    _color = value;
  }

  void setColor(SFColor &&value) {

    validateColor(value);

    _color = std::move(value);
  }

  /**
   * @brief Non-validating write of color (runtime/reader ingest path).
   * @details Assigns without the range check setColor() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setColor() stays the
   *          enforcement point for programmatic callers.
   */
  void setColorUnchecked(const SFColor &value) { _color = value; }

  /**
   * @brief Gets the value of fogType. AccessType: inputOutput
   * @details
   * @return FogTypeChoices The current value of fogType.
   */
  FogTypeChoices getFogType() const { return _fogType; }

  /**
   * @brief Sets the value of fogType. AccessType: inputOutput
   * @details
   * @param value The new value for fogType.
   */
  void setFogType(const FogTypeChoices &value) { _fogType = value; }

  /**
   * @brief Gets the value of visibilityRange. AccessType: inputOutput
   * @details
   * @return SFFloat The current value of visibilityRange.
   */
  SFFloat getVisibilityRange() const { return _visibilityRange; }

  /**
   * @brief Sets the value of visibilityRange. AccessType: inputOutput
   * @details
   * @param value The new value for visibilityRange.
   */
  void setVisibilityRange(const SFFloat &value) {

    validateVisibilityRange(value);

    _visibilityRange = value;
  }

  /**
   * @brief Non-validating write of visibilityRange (runtime/reader ingest
   * path).
   * @details Assigns without the range check setVisibilityRange() enforces, so
   * a permissive reader keeps an out-of-range authored value rather than reject
   * the whole document. setVisibilityRange() stays the enforcement point for
   * programmatic callers.
   */
  void setVisibilityRangeUnchecked(const SFFloat &value) {
    _visibilityRange = value;
  }

protected:
  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesColor(const SFColor &value,
                               const std::string &nodeType,
                               const std::string &defName,
                               std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesVisibilityRange(const SFFloat &value,
                                         const std::string &nodeType,
                                         const std::string &defName,
                                         std::vector<RangeDiagnostic> &out);

private:
  static void validateColor(const SFColor &value) {

    if (value.r < 0.0f)
      throw std::out_of_range("color.r below minimum of 0");
    if (value.r > 1.0f)
      throw std::out_of_range("color.r above maximum of 1");

    if (value.g < 0.0f)
      throw std::out_of_range("color.g below minimum of 0");
    if (value.g > 1.0f)
      throw std::out_of_range("color.g above maximum of 1");

    if (value.b < 0.0f)
      throw std::out_of_range("color.b below minimum of 0");
    if (value.b > 1.0f)
      throw std::out_of_range("color.b above maximum of 1");
  }

  static void validateVisibilityRange(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("visibilityRange below minimum of 0");
  }

  /**
   * @brief Member variable for color.
   */

  SFColor _color{SFColor{1, 1, 1}};

  /**
   * @brief Member variable for fogType.
   */

  FogTypeChoices _fogType{FogTypeChoices::LINEAR};

  /**
   * @brief Member variable for visibilityRange.
   */

  SFFloat _visibilityRange{0};
};

} // namespace x3d::nodes
