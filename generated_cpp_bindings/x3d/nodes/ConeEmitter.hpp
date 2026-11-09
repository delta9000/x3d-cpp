#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DParticleEmitterNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class ConeEmitter
 * @brief ConeEmitter generates all available particles from a specific point in
 * space.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/particleSystems.html#ConeEmitter
 */
class ConeEmitter : public virtual X3DParticleEmitterNode {
public:
  ConeEmitter() = default;

  ~ConeEmitter() = default;

  static SFFloat getDefaultAngle() { return 0.7854; }

  static SFVec3f getDefaultDirection() { return SFVec3f{0, 1, 0}; }

  static SFVec3f getDefaultPosition() { return SFVec3f{0, 0, 0}; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "emitter"; }

  static std::string componentName() { return "ParticleSystems"; }

  static int componentLevel() { return 1; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details Cone boundary for random distribution of particles about initial
   * direction.
   */
  const SFFloat &getAngle() const { return _angle; }

  /**
   * Access: inputOutput.
   * @details Cone boundary for random distribution of particles about initial
   * direction.
   */
  void setAngle(const SFFloat &value) {

    validateAngle(value);

    _angle = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setAngle() for range validation.
   */
  void setAngleUnchecked(const SFFloat &value) { _angle = value; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Initial direction from which particles emanate.
   */
  const SFVec3f &getDirection() const { return _direction; }

  /**
   * Access: inputOutput.
   * @details Initial direction from which particles emanate.
   */
  void setDirection(const SFVec3f &value) {

    validateDirection(value);

    _direction = value;
  }

  void setDirection(SFVec3f &&value) {

    validateDirection(value);

    _direction = std::move(value);
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setDirection() for range validation.
   */
  void setDirectionUnchecked(const SFVec3f &value) { _direction = value; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Point from which particles emanate.
   */
  const SFVec3f &getPosition() const { return _position; }

  /**
   * Access: inputOutput.
   * @details Point from which particles emanate.
   */
  void setPosition(const SFVec3f &value) { _position = value; }

  void setPosition(SFVec3f &&value) { _position = std::move(value); }

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
  static void checkRangesAngle(const SFFloat &value,
                               const std::string &nodeType,
                               const std::string &defName,
                               std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesDirection(const SFVec3f &value,
                                   const std::string &nodeType,
                                   const std::string &defName,
                                   std::vector<RangeDiagnostic> &out);

private:
  static void validateAngle(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("angle below minimum of 0");
    if (value > 3.1416f)
      throw std::out_of_range("angle above maximum of 3.1416");
  }

  static void validateDirection(const SFVec3f &value) {

    if (value.x < -1.0f)
      throw std::out_of_range("direction.x below minimum of -1");
    if (value.x > 1.0f)
      throw std::out_of_range("direction.x above maximum of 1");

    if (value.y < -1.0f)
      throw std::out_of_range("direction.y below minimum of -1");
    if (value.y > 1.0f)
      throw std::out_of_range("direction.y above maximum of 1");

    if (value.z < -1.0f)
      throw std::out_of_range("direction.z below minimum of -1");
    if (value.z > 1.0f)
      throw std::out_of_range("direction.z above maximum of 1");
  }

  SFFloat _angle{0.7854};

  SFVec3f _direction{SFVec3f{0, 1, 0}};

  SFVec3f _position{SFVec3f{0, 0, 0}};
};

} // namespace x3d::nodes
