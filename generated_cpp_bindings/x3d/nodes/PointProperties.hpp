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
 * @class PointProperties
 * @brief PointProperties allows precise fine-grained control over the rendering
 * style of PointSet node points inside the same Shape.
 * @details
 * https://github.com/Web3DConsortium/X3D/blob/master/ISO-IEC19775/ISO-IEC19775-1/ISO-IEC19775-1v4.0/ISO-IEC19775-1v4-WD1/Part01/components/shape.html
 */
class PointProperties : public virtual X3DAppearanceChildNode {
public:
  PointProperties() = default;

  ~PointProperties() = default;

  static SFVec3f getDefaultAttenuation() { return SFVec3f{1, 0, 0}; }

  static SFFloat getDefaultPointSizeMaxValue() { return 1; }

  static SFFloat getDefaultPointSizeMinValue() { return 1; }

  static SFFloat getDefaultPointSizeScaleFactor() { return 1; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "pointProperties"; }

  static std::string componentName() { return "Shape"; }

  static int componentLevel() { return 5; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details attenuation array values [a, b, c] are set to default values if
   * undefined.
   */
  const SFVec3f &getAttenuation() const { return _attenuation; }

  /**
   * Access: inputOutput.
   * @details attenuation array values [a, b, c] are set to default values if
   * undefined.
   */
  void setAttenuation(const SFVec3f &value) { _attenuation = value; }

  void setAttenuation(SFVec3f &&value) { _attenuation = std::move(value); }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details pointSizeMaxValue is maximum allowed scaling factor on nominal
   * browser point scaling.
   */
  const SFFloat &getPointSizeMaxValue() const { return _pointSizeMaxValue; }

  /**
   * Access: inputOutput.
   * @details pointSizeMaxValue is maximum allowed scaling factor on nominal
   * browser point scaling.
   */
  void setPointSizeMaxValue(const SFFloat &value) {

    validatePointSizeMaxValue(value);

    _pointSizeMaxValue = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setPointSizeMaxValue() for range validation.
   */
  void setPointSizeMaxValueUnchecked(const SFFloat &value) {
    _pointSizeMaxValue = value;
  }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details pointSizeMinValue is minimum allowed scaling factor on nominal
   * browser point scaling.
   */
  const SFFloat &getPointSizeMinValue() const { return _pointSizeMinValue; }

  /**
   * Access: inputOutput.
   * @details pointSizeMinValue is minimum allowed scaling factor on nominal
   * browser point scaling.
   */
  void setPointSizeMinValue(const SFFloat &value) {

    validatePointSizeMinValue(value);

    _pointSizeMinValue = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setPointSizeMinValue() for range validation.
   */
  void setPointSizeMinValueUnchecked(const SFFloat &value) {
    _pointSizeMinValue = value;
  }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details Nominal rendered point size is a browser-dependent minimum
   * renderable point size, which is then multiplied by an additional
   * pointSizeScaleFactor (which is greater than or equal to 1).
   */
  const SFFloat &getPointSizeScaleFactor() const {
    return _pointSizeScaleFactor;
  }

  /**
   * Access: inputOutput.
   * @details Nominal rendered point size is a browser-dependent minimum
   * renderable point size, which is then multiplied by an additional
   * pointSizeScaleFactor (which is greater than or equal to 1).
   */
  void setPointSizeScaleFactor(const SFFloat &value) {

    validatePointSizeScaleFactor(value);

    _pointSizeScaleFactor = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setPointSizeScaleFactor() for range validation.
   */
  void setPointSizeScaleFactorUnchecked(const SFFloat &value) {
    _pointSizeScaleFactor = value;
  }

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
  static void checkRangesPointSizeMaxValue(const SFFloat &value,
                                           const std::string &nodeType,
                                           const std::string &defName,
                                           std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesPointSizeMinValue(const SFFloat &value,
                                           const std::string &nodeType,
                                           const std::string &defName,
                                           std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesPointSizeScaleFactor(
      const SFFloat &value, const std::string &nodeType,
      const std::string &defName, std::vector<RangeDiagnostic> &out);

private:
  static void validatePointSizeMaxValue(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("pointSizeMaxValue below minimum of 0");
  }

  static void validatePointSizeMinValue(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("pointSizeMinValue below minimum of 0");
  }

  static void validatePointSizeScaleFactor(const SFFloat &value) {

    if (value < 1.0f)
      throw std::out_of_range("pointSizeScaleFactor below minimum of 1");
  }

  SFVec3f _attenuation{SFVec3f{1, 0, 0}};

  SFFloat _pointSizeMaxValue{1};

  SFFloat _pointSizeMinValue{1};

  SFFloat _pointSizeScaleFactor{1};
};

} // namespace x3d::nodes
