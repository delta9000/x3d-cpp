#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DParticleEmitterNode
 * @brief The X3DParticleEmitterNode abstract type represents any node that is
 * an emitter of particles.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/particleSystems.html#X3DParticleEmitterNode
 */
class X3DParticleEmitterNode : public virtual X3DNode {
public:
  X3DParticleEmitterNode() = default;

  virtual ~X3DParticleEmitterNode() = default;

  static SFFloat getDefaultMass() { return 0; }

  static SFBool getDefaultOn() { return true; }

  static SFFloat getDefaultSpeed() { return 0; }

  static SFFloat getDefaultSurfaceArea() { return 0; }

  static SFFloat getDefaultVariation() { return 0.25; }

  /// Missing containerField defaults to "children".
  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "ParticleSystems"; }

  static int componentLevel() { return 1; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFFloat &getMass() const { return _mass; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setMass(const SFFloat &value) {

    validateMass(value);

    _mass = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setMass() for range validation.
   */
  void setMassUnchecked(const SFFloat &value) { _mass = value; }

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
  const SFFloat &getSpeed() const { return _speed; }

  /**
   * Access: inputOutput.
   * @details
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
   * @details
   */
  const SFFloat &getSurfaceArea() const { return _surfaceArea; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setSurfaceArea(const SFFloat &value) {

    validateSurfaceArea(value);

    _surfaceArea = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setSurfaceArea() for range validation.
   */
  void setSurfaceAreaUnchecked(const SFFloat &value) { _surfaceArea = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details
   */
  const SFFloat &getVariation() const { return _variation; }

  /**
   * Access: inputOutput.
   * @details
   */
  void setVariation(const SFFloat &value) {

    validateVariation(value);

    _variation = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setVariation() for range validation.
   */
  void setVariationUnchecked(const SFFloat &value) { _variation = value; }

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
  static void checkRangesMass(const SFFloat &value, const std::string &nodeType,
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
  static void checkRangesSurfaceArea(const SFFloat &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesVariation(const SFFloat &value,
                                   const std::string &nodeType,
                                   const std::string &defName,
                                   std::vector<RangeDiagnostic> &out);

private:
  static void validateMass(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("mass below minimum of 0");
  }

  static void validateSpeed(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("speed below minimum of 0");
  }

  static void validateSurfaceArea(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("surfaceArea below minimum of 0");
  }

  static void validateVariation(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("variation below minimum of 0");
  }

  SFFloat _mass{0};

  SFBool _on{true};

  SFFloat _speed{0};

  SFFloat _surfaceArea{0};

  SFFloat _variation{0.25};
};

} // namespace x3d::nodes
