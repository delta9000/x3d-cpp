// BoundedPhysicsModel.hpp
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
 * @class BoundedPhysicsModel
 * @brief BoundedPhysicsModel provides user-defined geometrical boundaries for
 * particle motion.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/particleSystems.html#BoundedPhysicsModel
 */
class BoundedPhysicsModel : public virtual X3DParticlePhysicsModelNode {
public:
  /**
   * @brief Default constructor for BoundedPhysicsModel
   */
  BoundedPhysicsModel() = default;

  /**
   * @brief Destructor for BoundedPhysicsModel
   */
  ~BoundedPhysicsModel() = default;

  /**
   * @brief Get the default value for geometry
   * @return SFNode The default value
   */
  static SFNode getDefaultGeometry() { return nullptr; }

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
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of geometry. AccessType: inputOutput
   * @details Single contained geometry node provides the geometry used for each
   * particle when the parent ParticleSystem node has geometryType=GEOMETRY.
   * @return SFNode The current value of geometry.
   */
  SFNode getGeometry() const { return _geometry; }

  /**
   * @brief Acceptable node types for the geometry field.
   * @details Permitted X3D node types: X3DGeometryNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableGeometryNodeTypes() {
    static const std::vector<std::string> types = {"X3DGeometryNode"};
    return types;
  }

  /**
   * @brief Sets the value of geometry. AccessType: inputOutput
   * @details Single contained geometry node provides the geometry used for each
   * particle when the parent ParticleSystem node has geometryType=GEOMETRY.
   * @param value The new value for geometry.
   */
  void setGeometry(const SFNode &value) { _geometry = value; }

  void setGeometry(SFNode &&value) { _geometry = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "BoundedPhysicsModel").
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
   * @brief Member variable for geometry.
   */

  SFNode _geometry{nullptr};
};

} // namespace x3d::nodes
