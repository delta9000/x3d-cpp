// X3DLightNode.hpp
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
  /**
   * @brief Default constructor for X3DLightNode
   */
  X3DLightNode() = default;

  /**
   * @brief Virtual destructor for X3DLightNode
   */
  virtual ~X3DLightNode() = default;

  /**
   * @brief Get the default value for ambientIntensity
   * @return SFFloat The default value
   */
  static SFFloat getDefaultAmbientIntensity() { return 0; }

  /**
   * @brief Get the default value for color
   * @return SFColor The default value
   */
  static SFColor getDefaultColor() { return SFColor{1, 1, 1}; }

  /**
   * @brief Get the default value for intensity
   * @return SFFloat The default value
   */
  static SFFloat getDefaultIntensity() { return 1; }

  /**
   * @brief Get the default value for on
   * @return SFBool The default value
   */
  static SFBool getDefaultOn() { return true; }

  /**
   * @brief Get the default value for shadowIntensity
   * @return SFFloat The default value
   */
  static SFFloat getDefaultShadowIntensity() { return 1; }

  /**
   * @brief Get the default value for shadows
   * @return SFBool The default value
   */
  static SFBool getDefaultShadows() { return false; }

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
  static std::string componentName() { return "Lighting"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of ambientIntensity. AccessType: inputOutput
   * @details
   * @return SFFloat The current value of ambientIntensity.
   */
  SFFloat getAmbientIntensity() const { return _ambientIntensity; }

  /**
   * @brief Sets the value of ambientIntensity. AccessType: inputOutput
   * @details
   * @param value The new value for ambientIntensity.
   */
  void setAmbientIntensity(const SFFloat &value) {

    validateAmbientIntensity(value);

    _ambientIntensity = value;
  }

  /**
   * @brief Non-validating write of ambientIntensity (runtime/reader ingest
   * path).
   * @details Assigns without the range check setAmbientIntensity() enforces, so
   * a permissive reader keeps an out-of-range authored value rather than reject
   * the whole document. setAmbientIntensity() stays the enforcement point for
   * programmatic callers.
   */
  void setAmbientIntensityUnchecked(const SFFloat &value) {
    _ambientIntensity = value;
  }

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
   * @brief Gets the value of intensity. AccessType: inputOutput
   * @details
   * @return SFFloat The current value of intensity.
   */
  SFFloat getIntensity() const { return _intensity; }

  /**
   * @brief Sets the value of intensity. AccessType: inputOutput
   * @details
   * @param value The new value for intensity.
   */
  void setIntensity(const SFFloat &value) {

    validateIntensity(value);

    _intensity = value;
  }

  /**
   * @brief Non-validating write of intensity (runtime/reader ingest path).
   * @details Assigns without the range check setIntensity() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setIntensity() stays the
   *          enforcement point for programmatic callers.
   */
  void setIntensityUnchecked(const SFFloat &value) { _intensity = value; }

  /**
   * @brief Gets the value of on. AccessType: inputOutput
   * @details
   * @return SFBool The current value of on.
   */
  SFBool getOn() const { return _on; }

  /**
   * @brief Sets the value of on. AccessType: inputOutput
   * @details
   * @param value The new value for on.
   */
  void setOn(const SFBool &value) { _on = value; }

  /**
   * @brief Gets the value of shadowIntensity. AccessType: inputOutput
   * @details
   * @return SFFloat The current value of shadowIntensity.
   */
  SFFloat getShadowIntensity() const { return _shadowIntensity; }

  /**
   * @brief Sets the value of shadowIntensity. AccessType: inputOutput
   * @details
   * @param value The new value for shadowIntensity.
   */
  void setShadowIntensity(const SFFloat &value) {

    validateShadowIntensity(value);

    _shadowIntensity = value;
  }

  /**
   * @brief Non-validating write of shadowIntensity (runtime/reader ingest
   * path).
   * @details Assigns without the range check setShadowIntensity() enforces, so
   * a permissive reader keeps an out-of-range authored value rather than reject
   * the whole document. setShadowIntensity() stays the enforcement point for
   * programmatic callers.
   */
  void setShadowIntensityUnchecked(const SFFloat &value) {
    _shadowIntensity = value;
  }

  /**
   * @brief Gets the value of shadows. AccessType: inputOutput
   * @details
   * @return SFBool The current value of shadows.
   */
  SFBool getShadows() const { return _shadows; }

  /**
   * @brief Sets the value of shadows. AccessType: inputOutput
   * @details
   * @param value The new value for shadows.
   */
  void setShadows(const SFBool &value) { _shadows = value; }

  /**
   * @brief The X3D type name of this node (e.g. "X3DLightNode").
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

    if (value < 0)
      throw std::out_of_range("ambientIntensity below minimum of 0");
    if (value > 1)
      throw std::out_of_range("ambientIntensity above maximum of 1");
  }

  static void validateColor(const SFColor &value) {

    if (value.r < 0)
      throw std::out_of_range("color.r below minimum of 0");
    if (value.r > 1)
      throw std::out_of_range("color.r above maximum of 1");

    if (value.g < 0)
      throw std::out_of_range("color.g below minimum of 0");
    if (value.g > 1)
      throw std::out_of_range("color.g above maximum of 1");

    if (value.b < 0)
      throw std::out_of_range("color.b below minimum of 0");
    if (value.b > 1)
      throw std::out_of_range("color.b above maximum of 1");
  }

  static void validateIntensity(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("intensity below minimum of 0");
  }

  static void validateShadowIntensity(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("shadowIntensity below minimum of 0");
    if (value > 1)
      throw std::out_of_range("shadowIntensity above maximum of 1");
  }

  /**
   * @brief Member variable for ambientIntensity.
   */

  SFFloat _ambientIntensity{0};

  /**
   * @brief Member variable for color.
   */

  SFColor _color{SFColor{1, 1, 1}};

  /**
   * @brief Member variable for intensity.
   */

  SFFloat _intensity{1};

  /**
   * @brief Member variable for on.
   */

  SFBool _on{true};

  /**
   * @brief Member variable for shadowIntensity.
   */

  SFFloat _shadowIntensity{1};

  /**
   * @brief Member variable for shadows.
   */

  SFBool _shadows{false};
};

} // namespace x3d::nodes
