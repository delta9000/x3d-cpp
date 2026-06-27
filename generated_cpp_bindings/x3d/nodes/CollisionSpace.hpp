// CollisionSpace.hpp
#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DBoundedObject.hpp"

#include "x3d/nodes/X3DNBodyCollisionSpaceNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class CollisionSpace
 * @brief CollisionSpace holds collection of objects considered together for
 * resolution of inter-object collisions.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rigidBodyPhysics.html#CollisionSpace
 */
class CollisionSpace : public virtual X3DNBodyCollisionSpaceNode {
public:
  /**
   * @brief Default constructor for CollisionSpace
   */
  CollisionSpace() = default;

  /**
   * @brief Destructor for CollisionSpace
   */
  ~CollisionSpace() = default;

  /**
   * @brief Get the default value for useGeometry
   * @return SFBool The default value
   */
  static SFBool getDefaultUseGeometry() { return false; }

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
  static std::string componentName() { return "RigidBodyPhysics"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of collidables. AccessType: inputOutput
   * @details Collection of collidable objects as well as nested CollisionSpace
   * collections.
   * @return MFNode The current value of collidables.
   */
  MFNode getCollidables() const { return _collidables; }

  /**
   * @brief Acceptable node types for the collidables field.
   * @details Permitted X3D node types: X3DNBodyCollisionSpaceNode,
   * X3DNBodyCollidableNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableCollidablesNodeTypes() {
    static const std::vector<std::string> types = {"X3DNBodyCollisionSpaceNode",
                                                   "X3DNBodyCollidableNode"};
    return types;
  }

  /**
   * @brief Sets the value of collidables. AccessType: inputOutput
   * @details Collection of collidable objects as well as nested CollisionSpace
   * collections.
   * @param value The new value for collidables.
   */
  void setCollidables(const MFNode &value) { _collidables = value; }

  void setCollidables(MFNode &&value) { _collidables = std::move(value); }

  /**
   * @brief Gets the value of useGeometry. AccessType: inputOutput
   * @details useGeometry indicates whether collision-detection code checks down
   * to level of geometry, or only make approximations using geometry bounds.
   * @return SFBool The current value of useGeometry.
   */
  SFBool getUseGeometry() const { return _useGeometry; }

  /**
   * @brief Sets the value of useGeometry. AccessType: inputOutput
   * @details useGeometry indicates whether collision-detection code checks down
   * to level of geometry, or only make approximations using geometry bounds.
   * @param value The new value for useGeometry.
   */
  void setUseGeometry(const SFBool &value) { _useGeometry = value; }

  /**
   * @brief The X3D type name of this node (e.g. "CollisionSpace").
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
   * @brief Member variable for collidables.
   */

  MFNode _collidables{};

  /**
   * @brief Member variable for useGeometry.
   */

  SFBool _useGeometry{false};
};

} // namespace x3d::nodes
