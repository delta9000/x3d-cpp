// WindPhysicsModel.hpp
#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DParticlePhysicsModelNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class WindPhysicsModel
 * @brief WindPhysicsModel applies a wind effect to the particles.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/particleSystems.html#WindPhysicsModel
 */
class WindPhysicsModel : public virtual X3DParticlePhysicsModelNode {
public:
  /**
   * @brief Default constructor for WindPhysicsModel
   */
  WindPhysicsModel() = default;

  /**
   * @brief Destructor for WindPhysicsModel
   */
  ~WindPhysicsModel() = default;

  /**
   * @brief Get the default value for direction
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultDirection() { return SFVec3f{1, 0, 0}; }

  /**
   * @brief Get the default value for gustiness
   * @return SFFloat The default value
   */
  static SFFloat getDefaultGustiness() { return 0.1; }

  /**
   * @brief Get the default value for speed
   * @return SFFloat The default value
   */
  static SFFloat getDefaultSpeed() { return 0.1; }

  /**
   * @brief Get the default value for turbulence
   * @return SFFloat The default value
   */
  static SFFloat getDefaultTurbulence() { return 0; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "physics"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "ParticleSystems"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of direction. AccessType: inputOutput
   * @details direction in which wind is travelling in the form of a normalized,
   * unit vector.
   * @return SFVec3f The current value of direction.
   */
  SFVec3f getDirection() const { return _direction; }

  /**
   * @brief Sets the value of direction. AccessType: inputOutput
   * @details direction in which wind is travelling in the form of a normalized,
   * unit vector.
   * @param value The new value for direction.
   */
  void setDirection(const SFVec3f &value) { _direction = value; }

  void setDirection(SFVec3f &&value) { _direction = std::move(value); }

  /**
   * @brief Gets the value of gustiness. AccessType: inputOutput
   * @details gustiness specifies how much wind speed varies from the average
   * speed.
   * @return SFFloat The current value of gustiness.
   */
  SFFloat getGustiness() const { return _gustiness; }

  /**
   * @brief Sets the value of gustiness. AccessType: inputOutput
   * @details gustiness specifies how much wind speed varies from the average
   * speed.
   * @param value The new value for gustiness.
   */
  void setGustiness(const SFFloat &value) {

    validateGustiness(value);

    _gustiness = value;
  }

  /**
   * @brief Non-validating write of gustiness (runtime/reader ingest path).
   * @details Assigns without the range check setGustiness() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setGustiness() stays the
   *          enforcement point for programmatic callers.
   */
  void setGustinessUnchecked(const SFFloat &value) { _gustiness = value; }

  /**
   * @brief Gets the value of speed. AccessType: inputOutput
   * @details Initial linear speed (default is m/s) imparted to all particles
   * along their direction of movement.
   * @return SFFloat The current value of speed.
   */
  SFFloat getSpeed() const { return _speed; }

  /**
   * @brief Sets the value of speed. AccessType: inputOutput
   * @details Initial linear speed (default is m/s) imparted to all particles
   * along their direction of movement.
   * @param value The new value for speed.
   */
  void setSpeed(const SFFloat &value) {

    validateSpeed(value);

    _speed = value;
  }

  /**
   * @brief Non-validating write of speed (runtime/reader ingest path).
   * @details Assigns without the range check setSpeed() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setSpeed() stays the
   *          enforcement point for programmatic callers.
   */
  void setSpeedUnchecked(const SFFloat &value) { _speed = value; }

  /**
   * @brief Gets the value of turbulence. AccessType: inputOutput
   * @details turbulence field specifies how much the wind acts directly in line
   * with the direction, and how much variation is applied in directions other
   * than the wind direction.
   * @return SFFloat The current value of turbulence.
   */
  SFFloat getTurbulence() const { return _turbulence; }

  /**
   * @brief Sets the value of turbulence. AccessType: inputOutput
   * @details turbulence field specifies how much the wind acts directly in line
   * with the direction, and how much variation is applied in directions other
   * than the wind direction.
   * @param value The new value for turbulence.
   */
  void setTurbulence(const SFFloat &value) {

    validateTurbulence(value);

    _turbulence = value;
  }

  /**
   * @brief Non-validating write of turbulence (runtime/reader ingest path).
   * @details Assigns without the range check setTurbulence() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setTurbulence() stays the
   *          enforcement point for programmatic callers.
   */
  void setTurbulenceUnchecked(const SFFloat &value) { _turbulence = value; }

  /**
   * @brief The X3D type name of this node (e.g. "WindPhysicsModel").
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
  static void checkRangesGustiness(const SFFloat &value,
                                   const std::string &nodeType,
                                   const std::string &defName,
                                   std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesSpeed(const SFFloat &value,
                               const std::string &nodeType,
                               const std::string &defName,
                               std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesTurbulence(const SFFloat &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out);

private:
  static void validateGustiness(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("gustiness below minimum of 0");
  }

  static void validateSpeed(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("speed below minimum of 0");
  }

  static void validateTurbulence(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("turbulence below minimum of 0");
    if (value > 1.0f)
      throw std::out_of_range("turbulence above maximum of 1");
  }

  /**
   * @brief Member variable for direction.
   */

  SFVec3f _direction{SFVec3f{1, 0, 0}};

  /**
   * @brief Member variable for gustiness.
   */

  SFFloat _gustiness{0.1};

  /**
   * @brief Member variable for speed.
   */

  SFFloat _speed{0.1};

  /**
   * @brief Member variable for turbulence.
   */

  SFFloat _turbulence{0};
};

} // namespace x3d::nodes
