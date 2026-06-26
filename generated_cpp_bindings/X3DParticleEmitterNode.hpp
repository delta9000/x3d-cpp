// X3DParticleEmitterNode.hpp
#ifndef X3DPARTICLEEMITTERNODE_HPP
#define X3DPARTICLEEMITTERNODE_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

/**
 * @class X3DParticleEmitterNode
 * @brief The X3DParticleEmitterNode abstract type represents any node that is
 * an emitter of particles.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/particleSystems.html#X3DParticleEmitterNode
 */
class X3DParticleEmitterNode : public virtual X3DNode {
public:
  /**
   * @brief Default constructor for X3DParticleEmitterNode
   */
  X3DParticleEmitterNode() = default;

  /**
   * @brief Virtual destructor for X3DParticleEmitterNode
   */
  virtual ~X3DParticleEmitterNode() = default;

  /**
   * @brief Get the default value for mass
   * @return SFFloat The default value
   */
  static SFFloat getDefaultMass() { return 0; }

  /**
   * @brief Get the default value for on
   * @return SFBool The default value
   */
  static SFBool getDefaultOn() { return true; }

  /**
   * @brief Get the default value for speed
   * @return SFFloat The default value
   */
  static SFFloat getDefaultSpeed() { return 0; }

  /**
   * @brief Get the default value for surfaceArea
   * @return SFFloat The default value
   */
  static SFFloat getDefaultSurfaceArea() { return 0; }

  /**
   * @brief Get the default value for variation
   * @return SFFloat The default value
   */
  static SFFloat getDefaultVariation() { return 0.25; }

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
  static std::string componentName() { return "ParticleSystems"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of mass. AccessType: inputOutput
   * @details
   * @return SFFloat The current value of mass.
   */
  SFFloat getMass() const { return _mass; }

  /**
   * @brief Sets the value of mass. AccessType: inputOutput
   * @details
   * @param value The new value for mass.
   */
  void setMass(const SFFloat &value) {

    validateMass(value);

    _mass = value;
  }

  /**
   * @brief Non-validating write of mass (runtime/reader ingest path).
   * @details Assigns without the range check setMass() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setMass() stays the
   *          enforcement point for programmatic callers.
   */
  void setMassUnchecked(const SFFloat &value) { _mass = value; }

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
   * @brief Gets the value of speed. AccessType: inputOutput
   * @details
   * @return SFFloat The current value of speed.
   */
  SFFloat getSpeed() const { return _speed; }

  /**
   * @brief Sets the value of speed. AccessType: inputOutput
   * @details
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
   * @brief Gets the value of surfaceArea. AccessType: inputOutput
   * @details
   * @return SFFloat The current value of surfaceArea.
   */
  SFFloat getSurfaceArea() const { return _surfaceArea; }

  /**
   * @brief Sets the value of surfaceArea. AccessType: inputOutput
   * @details
   * @param value The new value for surfaceArea.
   */
  void setSurfaceArea(const SFFloat &value) {

    validateSurfaceArea(value);

    _surfaceArea = value;
  }

  /**
   * @brief Non-validating write of surfaceArea (runtime/reader ingest path).
   * @details Assigns without the range check setSurfaceArea() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setSurfaceArea() stays the
   *          enforcement point for programmatic callers.
   */
  void setSurfaceAreaUnchecked(const SFFloat &value) { _surfaceArea = value; }

  /**
   * @brief Gets the value of variation. AccessType: inputOutput
   * @details
   * @return SFFloat The current value of variation.
   */
  SFFloat getVariation() const { return _variation; }

  /**
   * @brief Sets the value of variation. AccessType: inputOutput
   * @details
   * @param value The new value for variation.
   */
  void setVariation(const SFFloat &value) {

    validateVariation(value);

    _variation = value;
  }

  /**
   * @brief Non-validating write of variation (runtime/reader ingest path).
   * @details Assigns without the range check setVariation() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setVariation() stays the
   *          enforcement point for programmatic callers.
   */
  void setVariationUnchecked(const SFFloat &value) { _variation = value; }

  /**
   * @brief The X3D type name of this node (e.g. "X3DParticleEmitterNode").
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

    if (value < 0)
      throw std::out_of_range("mass below minimum of 0");
  }

  static void validateSpeed(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("speed below minimum of 0");
  }

  static void validateSurfaceArea(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("surfaceArea below minimum of 0");
  }

  static void validateVariation(const SFFloat &value) {

    if (value < 0)
      throw std::out_of_range("variation below minimum of 0");
  }

  /**
   * @brief Member variable for mass.
   */

  SFFloat _mass{0};

  /**
   * @brief Member variable for on.
   */

  SFBool _on{true};

  /**
   * @brief Member variable for speed.
   */

  SFFloat _speed{0};

  /**
   * @brief Member variable for surfaceArea.
   */

  SFFloat _surfaceArea{0};

  /**
   * @brief Member variable for variation.
   */

  SFFloat _variation{0.25};
};

#endif // X3DPARTICLEEMITTERNODE_HPP