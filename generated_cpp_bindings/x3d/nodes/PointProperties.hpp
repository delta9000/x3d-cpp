// PointProperties.hpp
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
  /**
   * @brief Default constructor for PointProperties
   */
  PointProperties() = default;

  /**
   * @brief Destructor for PointProperties
   */
  ~PointProperties() = default;

  /**
   * @brief Get the default value for attenuation
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultAttenuation() { return SFVec3f{1, 0, 0}; }

  /**
   * @brief Get the default value for pointSizeMaxValue
   * @return SFFloat The default value
   */
  static SFFloat getDefaultPointSizeMaxValue() { return 1; }

  /**
   * @brief Get the default value for pointSizeMinValue
   * @return SFFloat The default value
   */
  static SFFloat getDefaultPointSizeMinValue() { return 1; }

  /**
   * @brief Get the default value for pointSizeScaleFactor
   * @return SFFloat The default value
   */
  static SFFloat getDefaultPointSizeScaleFactor() { return 1; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "pointProperties"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Shape"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 5; }

  /**
   * @brief Gets the value of attenuation. AccessType: inputOutput
   * @details attenuation array values [a, b, c] are set to default values if
   * undefined.
   * @return SFVec3f The current value of attenuation.
   */
  SFVec3f getAttenuation() const { return _attenuation; }

  /**
   * @brief Sets the value of attenuation. AccessType: inputOutput
   * @details attenuation array values [a, b, c] are set to default values if
   * undefined.
   * @param value The new value for attenuation.
   */
  void setAttenuation(const SFVec3f &value) { _attenuation = value; }

  void setAttenuation(SFVec3f &&value) { _attenuation = std::move(value); }

  /**
   * @brief Gets the value of pointSizeMaxValue. AccessType: inputOutput
   * @details pointSizeMaxValue is maximum allowed scaling factor on nominal
   * browser point scaling.
   * @return SFFloat The current value of pointSizeMaxValue.
   */
  SFFloat getPointSizeMaxValue() const { return _pointSizeMaxValue; }

  /**
   * @brief Sets the value of pointSizeMaxValue. AccessType: inputOutput
   * @details pointSizeMaxValue is maximum allowed scaling factor on nominal
   * browser point scaling.
   * @param value The new value for pointSizeMaxValue.
   */
  void setPointSizeMaxValue(const SFFloat &value) {

    validatePointSizeMaxValue(value);

    _pointSizeMaxValue = value;
  }

  /**
   * @brief Non-validating write of pointSizeMaxValue (runtime/reader ingest
   * path).
   * @details Assigns without the range check setPointSizeMaxValue() enforces,
   * so a permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setPointSizeMaxValue() stays the
   *          enforcement point for programmatic callers.
   */
  void setPointSizeMaxValueUnchecked(const SFFloat &value) {
    _pointSizeMaxValue = value;
  }

  /**
   * @brief Gets the value of pointSizeMinValue. AccessType: inputOutput
   * @details pointSizeMinValue is minimum allowed scaling factor on nominal
   * browser point scaling.
   * @return SFFloat The current value of pointSizeMinValue.
   */
  SFFloat getPointSizeMinValue() const { return _pointSizeMinValue; }

  /**
   * @brief Sets the value of pointSizeMinValue. AccessType: inputOutput
   * @details pointSizeMinValue is minimum allowed scaling factor on nominal
   * browser point scaling.
   * @param value The new value for pointSizeMinValue.
   */
  void setPointSizeMinValue(const SFFloat &value) {

    validatePointSizeMinValue(value);

    _pointSizeMinValue = value;
  }

  /**
   * @brief Non-validating write of pointSizeMinValue (runtime/reader ingest
   * path).
   * @details Assigns without the range check setPointSizeMinValue() enforces,
   * so a permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setPointSizeMinValue() stays the
   *          enforcement point for programmatic callers.
   */
  void setPointSizeMinValueUnchecked(const SFFloat &value) {
    _pointSizeMinValue = value;
  }

  /**
   * @brief Gets the value of pointSizeScaleFactor. AccessType: inputOutput
   * @details Nominal rendered point size is a browser-dependent minimum
   * renderable point size, which is then multiplied by an additional
   * pointSizeScaleFactor (which is greater than or equal to 1).
   * @return SFFloat The current value of pointSizeScaleFactor.
   */
  SFFloat getPointSizeScaleFactor() const { return _pointSizeScaleFactor; }

  /**
   * @brief Sets the value of pointSizeScaleFactor. AccessType: inputOutput
   * @details Nominal rendered point size is a browser-dependent minimum
   * renderable point size, which is then multiplied by an additional
   * pointSizeScaleFactor (which is greater than or equal to 1).
   * @param value The new value for pointSizeScaleFactor.
   */
  void setPointSizeScaleFactor(const SFFloat &value) {

    validatePointSizeScaleFactor(value);

    _pointSizeScaleFactor = value;
  }

  /**
   * @brief Non-validating write of pointSizeScaleFactor (runtime/reader ingest
   * path).
   * @details Assigns without the range check setPointSizeScaleFactor()
   * enforces, so a permissive reader keeps an out-of-range authored value
   * rather than reject the whole document. setPointSizeScaleFactor() stays the
   *          enforcement point for programmatic callers.
   */
  void setPointSizeScaleFactorUnchecked(const SFFloat &value) {
    _pointSizeScaleFactor = value;
  }

  /**
   * @brief The X3D type name of this node (e.g. "PointProperties").
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

    if (value < 0)
      throw std::out_of_range("pointSizeMaxValue below minimum of 0");
  }

  static void validatePointSizeMinValue(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("pointSizeMinValue below minimum of 0");
  }

  static void validatePointSizeScaleFactor(const SFFloat &value) {

    if (value < 1)
      throw std::out_of_range("pointSizeScaleFactor below minimum of 1");
  }

  /**
   * @brief Member variable for attenuation.
   */

  SFVec3f _attenuation{SFVec3f{1, 0, 0}};

  /**
   * @brief Member variable for pointSizeMaxValue.
   */

  SFFloat _pointSizeMaxValue{1};

  /**
   * @brief Member variable for pointSizeMinValue.
   */

  SFFloat _pointSizeMinValue{1};

  /**
   * @brief Member variable for pointSizeScaleFactor.
   */

  SFFloat _pointSizeScaleFactor{1};
};

} // namespace x3d::nodes
