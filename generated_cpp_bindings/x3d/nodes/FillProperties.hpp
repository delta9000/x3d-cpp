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
  FillProperties() = default;

  ~FillProperties() = default;

  static SFBool getDefaultFilled() { return true; }

  static SFColor getDefaultHatchColor() { return SFColor{1, 1, 1}; }

  static SFBool getDefaultHatched() { return true; }

  static SFInt32 getDefaultHatchStyle() { return 1; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "fillProperties"; }

  static std::string componentName() { return "Shape"; }

  static int componentLevel() { return 3; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details Whether or not associated geometry is filled.
   */
  const SFBool &getFilled() const { return _filled; }

  /**
   * Access: inputOutput.
   * @details Whether or not associated geometry is filled.
   */
  void setFilled(const SFBool &value) { _filled = value; }

  /**
   * SFColor; access: inputOutput. Borrows the stored value.
   * @details Color of the hatch pattern.
   */
  const SFColor &getHatchColor() const { return _hatchColor; }

  /**
   * Access: inputOutput.
   * @details Color of the hatch pattern.
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
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setHatchColor() for range validation.
   */
  void setHatchColorUnchecked(const SFColor &value) { _hatchColor = value; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details Whether or not associated geometry is hatched.
   */
  const SFBool &getHatched() const { return _hatched; }

  /**
   * Access: inputOutput.
   * @details Whether or not associated geometry is hatched.
   */
  void setHatched(const SFBool &value) { _hatched = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details hatchStyle selects a hatch pattern from ISO/IEC 9973 International
   * Register of Graphical Items.
   */
  const SFInt32 &getHatchStyle() const { return _hatchStyle; }

  /**
   * Access: inputOutput.
   * @details hatchStyle selects a hatch pattern from ISO/IEC 9973 International
   * Register of Graphical Items.
   */
  void setHatchStyle(const SFInt32 &value) {

    validateHatchStyle(value);

    _hatchStyle = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setHatchStyle() for range validation.
   */
  void setHatchStyleUnchecked(const SFInt32 &value) { _hatchStyle = value; }

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

  SFBool _filled{true};

  SFColor _hatchColor{SFColor{1, 1, 1}};

  SFBool _hatched{true};

  SFInt32 _hatchStyle{1};
};

} // namespace x3d::nodes
