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
  CollisionSensor() = default;

  ~CollisionSensor() = default;

  static SFNode getDefaultCollider() { return nullptr; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "RigidBodyPhysics"; }

  static int componentLevel() { return 1; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details The collider field specifies a CollisionCollection node that holds
   * a collidables field of nodes and spaces that are to be included in
   * collision-detection computations.
   */
  const SFNode &getCollider() const { return _collider; }

  /**
   * @details Permitted X3D node types: CollisionCollection
   */
  static const std::vector<std::string> &acceptableColliderNodeTypes() {
    static const std::vector<std::string> types = {"CollisionCollection"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details The collider field specifies a CollisionCollection node that holds
   * a collidables field of nodes and spaces that are to be included in
   * collision-detection computations.
   */
  void setCollider(const SFNode &value) { _collider = value; }

  void setCollider(SFNode &&value) { _collider = std::move(value); }

  /**
   * MFNode; access: outputOnly. Borrows the stored value.
   * @details
   */
  const MFNode &getContacts() const { return _contacts; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details
   */
  void emitContacts(const MFNode &value) { _contacts = value; }

  /**
   * @details Permitted X3D node types: Contact
   */
  static const std::vector<std::string> &acceptableContactsNodeTypes() {
    static const std::vector<std::string> types = {"Contact"};
    return types;
  }

  /**
   * MFNode; access: outputOnly. Borrows the stored value.
   * @details
   */
  const MFNode &getIntersections() const { return _intersections; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details
   */
  void emitIntersections(const MFNode &value) { _intersections = value; }

  /**
   * @details Permitted X3D node types: X3DNBodyCollidableNode
   */
  static const std::vector<std::string> &acceptableIntersectionsNodeTypes() {
    static const std::vector<std::string> types = {"X3DNBodyCollidableNode"};
    return types;
  }

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

private:
  SFNode _collider{nullptr};

  MFNode _contacts{};

  MFNode _intersections{};
};

} // namespace x3d::nodes
