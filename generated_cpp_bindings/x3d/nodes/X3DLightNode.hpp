#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DChildNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DLightNode
 * @brief Light nodes provide illumination for rendering geometry in the scene.
 * Implementing nodes must include a global field with type SFBool and
 * accessType inputOutput.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/lighting.html#X3DLightNode
 */
class X3DLightNode : public virtual X3DChildNode {
public:
  X3DLightNode() = default;

  virtual ~X3DLightNode() = default;

  static SFFloat getDefaultAmbientIntensity() { return 0; }

  static SFColor getDefaultColor() { return SFColor{1, 1, 1}; }

  static SFFloat getDefaultIntensity() { return 1; }

  static SFBool getDefaultOn() { return true; }

  static SFFloat getDefaultShadowIntensity() { return 1; }

  static SFBool getDefaultShadows() { return false; }

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Lighting"; }

  static int componentLevel() { return 1; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFFloat &getAmbientIntensity() const { return _ambientIntensity; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setAmbientIntensity(const SFFloat &value) {

    validateAmbientIntensity(value);

    _ambientIntensity = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setAmbientIntensity() for range validation.
   */
  void setAmbientIntensityUnchecked(const SFFloat &value) {
    _ambientIntensity = value;
  }

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
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFFloat &getIntensity() const { return _intensity; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setIntensity(const SFFloat &value) {

    validateIntensity(value);

    _intensity = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setIntensity() for range validation.
   */
  void setIntensityUnchecked(const SFFloat &value) { _intensity = value; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFBool &getOn() const { return _on; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setOn(const SFBool &value) { _on = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFFloat &getShadowIntensity() const { return _shadowIntensity; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setShadowIntensity(const SFFloat &value) {

    validateShadowIntensity(value);

    _shadowIntensity = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setShadowIntensity() for range validation.
   */
  void setShadowIntensityUnchecked(const SFFloat &value) {
    _shadowIntensity = value;
  }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFBool &getShadows() const { return _shadows; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setShadows(const SFBool &value) { _shadows = value; }

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
  static void checkRangesAmbientIntensity(const SFFloat &value,
                                          const std::string &nodeType,
                                          const std::string &defName,
                                          std::vector<RangeDiagnostic> &out);

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
  static void checkRangesIntensity(const SFFloat &value,
                                   const std::string &nodeType,
                                   const std::string &defName,
                                   std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesShadowIntensity(const SFFloat &value,
                                         const std::string &nodeType,
                                         const std::string &defName,
                                         std::vector<RangeDiagnostic> &out);

private:
  static void validateAmbientIntensity(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("ambientIntensity below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("ambientIntensity above maximum of 1");
  }

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

  static void validateIntensity(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("intensity below minimum of 0");
  }

  static void validateShadowIntensity(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("shadowIntensity below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("shadowIntensity above maximum of 1");
  }

  SFFloat _ambientIntensity{0};

  SFColor _color{SFColor{1, 1, 1}};

  SFFloat _intensity{1};

  SFBool _on{true};

  SFFloat _shadowIntensity{1};

  SFBool _shadows{false};
};

} // namespace x3d::nodes
