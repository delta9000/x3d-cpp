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
  X3DFogObject() = default;

  virtual ~X3DFogObject() = default;

  static SFColor getDefaultColor() { return SFColor{1, 1, 1}; }

  static FogTypeChoices getDefaultFogType() { return FogTypeChoices::LINEAR; }

  static SFFloat getDefaultVisibilityRange() { return 0; }

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "EnvironmentalEffects"; }

  static int componentLevel() { return 2; }

  /**
   * SFColor; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFColor &getColor() const { return _color; }

  /**
   * Access: inputOutput.
   * @details
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
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setColor() for range validation.
   */
  void setColorUnchecked(const SFColor &value) { _color = value; }

  /**
   * FogTypeChoices; access: inputOutput. Borrows the stored value.
   * @details
   */
  const FogTypeChoices &getFogType() const { return _fogType; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setFogType(const FogTypeChoices &value) { _fogType = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFFloat &getVisibilityRange() const { return _visibilityRange; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setVisibilityRange(const SFFloat &value) {

    validateVisibilityRange(value);

    _visibilityRange = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setVisibilityRange() for range validation.
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

  SFColor _color{SFColor{1, 1, 1}};

  FogTypeChoices _fogType{FogTypeChoices::LINEAR};

  SFFloat _visibilityRange{0};
};

} // namespace x3d::nodes
