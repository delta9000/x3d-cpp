// ForcePhysicsModel.hpp
#ifndef FORCEPHYSICSMODEL_HPP
#define FORCEPHYSICSMODEL_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DParticlePhysicsModelNode.hpp"

/**
 * @class ForcePhysicsModel
 * @brief ForcePhysicsModel applies a constant force value to the particles.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/particleSystems.html#ForcePhysicsModel
 */
class ForcePhysicsModel : public virtual X3DParticlePhysicsModelNode {
public:
  /**
   * @brief Default constructor for ForcePhysicsModel
   */
  ForcePhysicsModel() = default;

  /**
   * @brief Destructor for ForcePhysicsModel
   */
  ~ForcePhysicsModel() = default;

  /**
   * @brief Get the default value for force
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultForce() { return SFVec3f{0, -9.8, 0}; }

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
   * @brief Gets the value of force. AccessType: inputOutput
   * @details force field indicates strength and direction of the propelling
   * force on the particles (for example, default is Earth's gravity).
   * @return SFVec3f The current value of force.
   */
  SFVec3f getForce() const { return _force; }

  /**
   * @brief Sets the value of force. AccessType: inputOutput
   * @details force field indicates strength and direction of the propelling
   * force on the particles (for example, default is Earth's gravity).
   * @param value The new value for force.
   */
  void setForce(const SFVec3f &value) { _force = value; }

  void setForce(SFVec3f &&value) { _force = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "ForcePhysicsModel").
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

private:
  /**
   * @brief Member variable for force.
   */

  SFVec3f _force{SFVec3f{0, -9.8, 0}};
};

#endif // FORCEPHYSICSMODEL_HPP