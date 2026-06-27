// CollisionSensor.hpp
#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DChildNode.hpp"

#include "x3d/nodes/X3DSensorNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class CollisionSensor
 * @brief CollisionSensor generates collision-detection events.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rigidBodyPhysics.html#CollisionSensor
 */
class CollisionSensor : public virtual X3DSensorNode {
public:
  /**
   * @brief Default constructor for CollisionSensor
   */
  CollisionSensor() = default;

  /**
   * @brief Destructor for CollisionSensor
   */
  ~CollisionSensor() = default;

  /**
   * @brief Get the default value for collider
   * @return SFNode The default value
   */
  static SFNode getDefaultCollider() { return nullptr; }

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
   * @brief Gets the value of collider. AccessType: inputOutput
   * @details The collider field specifies a CollisionCollection node that holds
   * a collidables field of nodes and spaces that are to be included in
   * collision-detection computations.
   * @return SFNode The current value of collider.
   */
  SFNode getCollider() const { return _collider; }

  /**
   * @brief Acceptable node types for the collider field.
   * @details Permitted X3D node types: CollisionCollection
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableColliderNodeTypes() {
    static const std::vector<std::string> types = {"CollisionCollection"};
    return types;
  }

  /**
   * @brief Sets the value of collider. AccessType: inputOutput
   * @details The collider field specifies a CollisionCollection node that holds
   * a collidables field of nodes and spaces that are to be included in
   * collision-detection computations.
   * @param value The new value for collider.
   */
  void setCollider(const SFNode &value) { _collider = value; }

  void setCollider(SFNode &&value) { _collider = std::move(value); }

  /**
   * @brief Gets the value of contacts. AccessType: outputOnly
   * @details
   * @return MFNode The current value of contacts.
   */
  MFNode getContacts() const { return _contacts; }

  /**
   * @brief Emit an output value on contacts. AccessType: outputOnly
   * @details
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitContacts(const MFNode &value) { _contacts = value; }

  /**
   * @brief Acceptable node types for the contacts field.
   * @details Permitted X3D node types: Contact
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableContactsNodeTypes() {
    static const std::vector<std::string> types = {"Contact"};
    return types;
  }

  /**
   * @brief Gets the value of intersections. AccessType: outputOnly
   * @details
   * @return MFNode The current value of intersections.
   */
  MFNode getIntersections() const { return _intersections; }

  /**
   * @brief Emit an output value on intersections. AccessType: outputOnly
   * @details
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitIntersections(const MFNode &value) { _intersections = value; }

  /**
   * @brief Acceptable node types for the intersections field.
   * @details Permitted X3D node types: X3DNBodyCollidableNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableIntersectionsNodeTypes() {
    static const std::vector<std::string> types = {"X3DNBodyCollidableNode"};
    return types;
  }

  /**
   * @brief The X3D type name of this node (e.g. "CollisionSensor").
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
   * @brief Member variable for collider.
   */

  SFNode _collider{nullptr};

  /**
   * @brief Member variable for contacts.
   */

  MFNode _contacts{};

  /**
   * @brief Member variable for intersections.
   */

  MFNode _intersections{};
};

} // namespace x3d::nodes
