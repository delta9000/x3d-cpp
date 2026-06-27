// SpotLight.hpp
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

#include "x3d/nodes/X3DLightNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class SpotLight
 * @brief Linear attenuation may occur at level 2, full support at level 3.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/lighting.html#SpotLight
 */
class SpotLight : public virtual X3DLightNode {
public:
  /**
   * @brief Default constructor for SpotLight
   */
  SpotLight() = default;

  /**
   * @brief Destructor for SpotLight
   */
  ~SpotLight() = default;

  /**
   * @brief Get the default value for attenuation
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultAttenuation() { return SFVec3f{1, 0, 0}; }

  /**
   * @brief Get the default value for beamWidth
   * @return SFFloat The default value
   */
  static SFFloat getDefaultBeamWidth() { return 0.589049; }

  /**
   * @brief Get the default value for cutOffAngle
   * @return SFFloat The default value
   */
  static SFFloat getDefaultCutOffAngle() { return 1.570796; }

  /**
   * @brief Get the default value for direction
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultDirection() { return SFVec3f{0, 0, -1}; }

  /**
   * @brief Get the default value for global
   * @return SFBool The default value
   */
  static SFBool getDefaultGlobal() { return true; }

  /**
   * @brief Get the default value for location
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultLocation() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the default value for radius
   * @return SFFloat The default value
   */
  static SFFloat getDefaultRadius() { return 100; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
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
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of attenuation. AccessType: inputOutput
   * @details Constant, linear-distance and squared-distance dropoff factors as
   * radial distance increases from the source.
   * @return SFVec3f The current value of attenuation.
   */
  SFVec3f getAttenuation() const { return _attenuation; }

  /**
   * @brief Sets the value of attenuation. AccessType: inputOutput
   * @details Constant, linear-distance and squared-distance dropoff factors as
   * radial distance increases from the source.
   * @param value The new value for attenuation.
   */
  void setAttenuation(const SFVec3f &value) {

    validateAttenuation(value);

    _attenuation = value;
  }

  void setAttenuation(SFVec3f &&value) {

    validateAttenuation(value);

    _attenuation = std::move(value);
  }

  /**
   * @brief Non-validating write of attenuation (runtime/reader ingest path).
   * @details Assigns without the range check setAttenuation() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setAttenuation() stays the
   *          enforcement point for programmatic callers.
   */
  void setAttenuationUnchecked(const SFVec3f &value) { _attenuation = value; }

  /**
   * @brief Gets the value of beamWidth. AccessType: inputOutput
   * @details Inner conical solid angle (in radians) where light source has
   * uniform full intensity.
   * @return SFFloat The current value of beamWidth.
   */
  SFFloat getBeamWidth() const { return _beamWidth; }

  /**
   * @brief Sets the value of beamWidth. AccessType: inputOutput
   * @details Inner conical solid angle (in radians) where light source has
   * uniform full intensity.
   * @param value The new value for beamWidth.
   */
  void setBeamWidth(const SFFloat &value) {

    validateBeamWidth(value);

    _beamWidth = value;
  }

  /**
   * @brief Non-validating write of beamWidth (runtime/reader ingest path).
   * @details Assigns without the range check setBeamWidth() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setBeamWidth() stays the
   *          enforcement point for programmatic callers.
   */
  void setBeamWidthUnchecked(const SFFloat &value) { _beamWidth = value; }

  /**
   * @brief Gets the value of cutOffAngle. AccessType: inputOutput
   * @details Outer conical solid angle (in radians) where light source
   * intensity becomes zero.
   * @return SFFloat The current value of cutOffAngle.
   */
  SFFloat getCutOffAngle() const { return _cutOffAngle; }

  /**
   * @brief Sets the value of cutOffAngle. AccessType: inputOutput
   * @details Outer conical solid angle (in radians) where light source
   * intensity becomes zero.
   * @param value The new value for cutOffAngle.
   */
  void setCutOffAngle(const SFFloat &value) {

    validateCutOffAngle(value);

    _cutOffAngle = value;
  }

  /**
   * @brief Non-validating write of cutOffAngle (runtime/reader ingest path).
   * @details Assigns without the range check setCutOffAngle() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setCutOffAngle() stays the
   *          enforcement point for programmatic callers.
   */
  void setCutOffAngleUnchecked(const SFFloat &value) { _cutOffAngle = value; }

  /**
   * @brief Gets the value of direction. AccessType: inputOutput
   * @details Orientation vector of light relative to local coordinate system.
   * @return SFVec3f The current value of direction.
   */
  SFVec3f getDirection() const { return _direction; }

  /**
   * @brief Sets the value of direction. AccessType: inputOutput
   * @details Orientation vector of light relative to local coordinate system.
   * @param value The new value for direction.
   */
  void setDirection(const SFVec3f &value) { _direction = value; }

  void setDirection(SFVec3f &&value) { _direction = std::move(value); }

  /**
   * @brief Gets the value of global. AccessType: inputOutput
   * @details Global lights illuminate all objects within their volume of
   * lighting influence.
   * @return SFBool The current value of global.
   */
  SFBool getGlobal() const { return _global; }

  /**
   * @brief Sets the value of global. AccessType: inputOutput
   * @details Global lights illuminate all objects within their volume of
   * lighting influence.
   * @param value The new value for global.
   */
  void setGlobal(const SFBool &value) { _global = value; }

  /**
   * @brief Gets the value of location. AccessType: inputOutput
   * @details Position of light relative to local coordinate system.
   * @return SFVec3f The current value of location.
   */
  SFVec3f getLocation() const { return _location; }

  /**
   * @brief Sets the value of location. AccessType: inputOutput
   * @details Position of light relative to local coordinate system.
   * @param value The new value for location.
   */
  void setLocation(const SFVec3f &value) { _location = value; }

  void setLocation(SFVec3f &&value) { _location = std::move(value); }

  /**
   * @brief Gets the value of radius. AccessType: inputOutput
   * @details Maximum effective distance of light relative to local light
   * position, affected by ancestor scaling.
   * @return SFFloat The current value of radius.
   */
  SFFloat getRadius() const { return _radius; }

  /**
   * @brief Sets the value of radius. AccessType: inputOutput
   * @details Maximum effective distance of light relative to local light
   * position, affected by ancestor scaling.
   * @param value The new value for radius.
   */
  void setRadius(const SFFloat &value) {

    validateRadius(value);

    _radius = value;
  }

  /**
   * @brief Non-validating write of radius (runtime/reader ingest path).
   * @details Assigns without the range check setRadius() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setRadius() stays the
   *          enforcement point for programmatic callers.
   */
  void setRadiusUnchecked(const SFFloat &value) { _radius = value; }

  /**
   * @brief The X3D type name of this node (e.g. "SpotLight").
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
  static void checkRangesAttenuation(const SFVec3f &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesBeamWidth(const SFFloat &value,
                                   const std::string &nodeType,
                                   const std::string &defName,
                                   std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesCutOffAngle(const SFFloat &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesRadius(const SFFloat &value,
                                const std::string &nodeType,
                                const std::string &defName,
                                std::vector<RangeDiagnostic> &out);

private:
  static void validateAttenuation(const SFVec3f &value) {

    if (value.x < 0)
      throw std::out_of_range("attenuation.x below minimum of 0");

    if (value.y < 0)
      throw std::out_of_range("attenuation.y below minimum of 0");

    if (value.z < 0)
      throw std::out_of_range("attenuation.z below minimum of 0");
  }

  static void validateBeamWidth(const SFFloat &value) {

    if (value > 1.570796)
      throw std::out_of_range("beamWidth above maximum of 1.570796");
  }

  static void validateCutOffAngle(const SFFloat &value) {

    if (value > 1.570796)
      throw std::out_of_range("cutOffAngle above maximum of 1.570796");
  }

  static void validateRadius(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("radius below minimum of 0");
  }

  /**
   * @brief Member variable for attenuation.
   */

  SFVec3f _attenuation{SFVec3f{1, 0, 0}};

  /**
   * @brief Member variable for beamWidth.
   */

  SFFloat _beamWidth{0.589049};

  /**
   * @brief Member variable for cutOffAngle.
   */

  SFFloat _cutOffAngle{1.570796};

  /**
   * @brief Member variable for direction.
   */

  SFVec3f _direction{SFVec3f{0, 0, -1}};

  /**
   * @brief Member variable for global.
   */

  SFBool _global{true};

  /**
   * @brief Member variable for location.
   */

  SFVec3f _location{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for radius.
   */

  SFFloat _radius{100};
};

} // namespace x3d::nodes
