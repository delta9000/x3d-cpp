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
  WindPhysicsModel() = default;

  ~WindPhysicsModel() = default;

  static SFVec3f getDefaultDirection() { return SFVec3f{1, 0, 0}; }

  static SFFloat getDefaultGustiness() { return 0.1; }

  static SFFloat getDefaultSpeed() { return 0.1; }

  static SFFloat getDefaultTurbulence() { return 0; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "physics"; }

  static std::string componentName() { return "ParticleSystems"; }

  static int componentLevel() { return 1; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details direction in which wind is travelling in the form of a normalized,
   * unit vector.
   */
  const SFVec3f &getDirection() const { return _direction; }

  /**
   * Access: inputOutput.
   * @details direction in which wind is travelling in the form of a normalized,
   * unit vector.
   */
  void setDirection(const SFVec3f &value) { _direction = value; }

  void setDirection(SFVec3f &&value) { _direction = std::move(value); }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details gustiness specifies how much wind speed varies from the average
   * speed.
   */
  const SFFloat &getGustiness() const { return _gustiness; }

  /**
   * Access: inputOutput.
   * @details gustiness specifies how much wind speed varies from the average
   * speed.
   */
  void setGustiness(const SFFloat &value) {

    validateGustiness(value);

    _gustiness = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setGustiness() for range validation.
   */
  void setGustinessUnchecked(const SFFloat &value) { _gustiness = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details Initial linear speed (default is m/s) imparted to all particles
   * along their direction of movement.
   */
  const SFFloat &getSpeed() const { return _speed; }

  /**
   * Access: inputOutput.
   * @details Initial linear speed (default is m/s) imparted to all particles
   * along their direction of movement.
   */
  void setSpeed(const SFFloat &value) {

    validateSpeed(value);

    _speed = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setSpeed() for range validation.
   */
  void setSpeedUnchecked(const SFFloat &value) { _speed = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details turbulence field specifies how much the wind acts directly in line
   * with the direction, and how much variation is applied in directions other
   * than the wind direction.
   */
  const SFFloat &getTurbulence() const { return _turbulence; }

  /**
   * Access: inputOutput.
   * @details turbulence field specifies how much the wind acts directly in line
   * with the direction, and how much variation is applied in directions other
   * than the wind direction.
   */
  void setTurbulence(const SFFloat &value) {

    validateTurbulence(value);

    _turbulence = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setTurbulence() for range validation.
   */
  void setTurbulenceUnchecked(const SFFloat &value) { _turbulence = value; }

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

  SFVec3f _direction{SFVec3f{1, 0, 0}};

  SFFloat _gustiness{0.1};

  SFFloat _speed{0.1};

  SFFloat _turbulence{0};
};

} // namespace x3d::nodes
