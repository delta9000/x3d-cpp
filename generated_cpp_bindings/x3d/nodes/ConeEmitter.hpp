// ConeEmitter.hpp
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
  /**
   * @brief Default constructor for ConeEmitter
   */
  ConeEmitter() = default;

  /**
   * @brief Destructor for ConeEmitter
   */
  ~ConeEmitter() = default;

  /**
   * @brief Get the default value for angle
   * @return SFFloat The default value
   */
  static SFFloat getDefaultAngle() { return 0.7854; }

  /**
   * @brief Get the default value for direction
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultDirection() { return SFVec3f{0, 1, 0}; }

  /**
   * @brief Get the default value for position
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultPosition() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "emitter"; }

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
   * @brief Gets the value of angle. AccessType: inputOutput
   * @details Cone boundary for random distribution of particles about initial
   * direction.
   * @return SFFloat The current value of angle.
   */
  SFFloat getAngle() const { return _angle; }

  /**
   * @brief Sets the value of angle. AccessType: inputOutput
   * @details Cone boundary for random distribution of particles about initial
   * direction.
   * @param value The new value for angle.
   */
  void setAngle(const SFFloat &value) {

    validateAngle(value);

    _angle = value;
  }

  /**
   * @brief Non-validating write of angle (runtime/reader ingest path).
   * @details Assigns without the range check setAngle() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setAngle() stays the
   *          enforcement point for programmatic callers.
   */
  void setAngleUnchecked(const SFFloat &value) { _angle = value; }

  /**
   * @brief Gets the value of direction. AccessType: inputOutput
   * @details Initial direction from which particles emanate.
   * @return SFVec3f The current value of direction.
   */
  SFVec3f getDirection() const { return _direction; }

  /**
   * @brief Sets the value of direction. AccessType: inputOutput
   * @details Initial direction from which particles emanate.
   * @param value The new value for direction.
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
   * @brief Non-validating write of direction (runtime/reader ingest path).
   * @details Assigns without the range check setDirection() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setDirection() stays the
   *          enforcement point for programmatic callers.
   */
  void setDirectionUnchecked(const SFVec3f &value) { _direction = value; }

  /**
   * @brief Gets the value of position. AccessType: inputOutput
   * @details Point from which particles emanate.
   * @return SFVec3f The current value of position.
   */
  SFVec3f getPosition() const { return _position; }

  /**
   * @brief Sets the value of position. AccessType: inputOutput
   * @details Point from which particles emanate.
   * @param value The new value for position.
   */
  void setPosition(const SFVec3f &value) { _position = value; }

  void setPosition(SFVec3f &&value) { _position = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "ConeEmitter").
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

  /**
   * @brief Member variable for angle.
   */

  SFFloat _angle{0.7854};

  /**
   * @brief Member variable for direction.
   */

  SFVec3f _direction{SFVec3f{0, 1, 0}};

  /**
   * @brief Member variable for position.
   */

  SFVec3f _position{SFVec3f{0, 0, 0}};
};

} // namespace x3d::nodes
