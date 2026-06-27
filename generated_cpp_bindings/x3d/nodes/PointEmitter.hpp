// PointEmitter.hpp
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
 * @class PointEmitter
 * @brief PointEmitter generates particles from a specific point in space using
 * the specified direction and speed.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/particleSystems.html#PointEmitter
 */
class PointEmitter : public virtual X3DParticleEmitterNode {
public:
  /**
   * @brief Default constructor for PointEmitter
   */
  PointEmitter() = default;

  /**
   * @brief Destructor for PointEmitter
   */
  ~PointEmitter() = default;

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
  void setDirection(const SFVec3f &value) { _direction = value; }

  void setDirection(SFVec3f &&value) { _direction = std::move(value); }

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
   * @brief The X3D type name of this node (e.g. "PointEmitter").
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

private:
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
