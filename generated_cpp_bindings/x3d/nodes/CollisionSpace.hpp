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
  CollisionSpace() = default;

  ~CollisionSpace() = default;

  static SFBool getDefaultUseGeometry() { return false; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "RigidBodyPhysics"; }

  static int componentLevel() { return 1; }

  /**
   * MFNode; access: inputOutput. Borrows the stored value.
   * @details Collection of collidable objects as well as nested CollisionSpace
   * collections.
   */
  const MFNode &getCollidables() const { return _collidables; }

  /**
   * @details Permitted X3D node types: X3DNBodyCollisionSpaceNode,
   * X3DNBodyCollidableNode
   */
  static const std::vector<std::string> &acceptableCollidablesNodeTypes() {
    static const std::vector<std::string> types = {"X3DNBodyCollisionSpaceNode",
                                                   "X3DNBodyCollidableNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Collection of collidable objects as well as nested CollisionSpace
   * collections.
   */
  void setCollidables(const MFNode &value) { _collidables = value; }

  void setCollidables(MFNode &&value) { _collidables = std::move(value); }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details useGeometry indicates whether collision-detection code checks down
   * to level of geometry, or only make approximations using geometry bounds.
   */
  const SFBool &getUseGeometry() const { return _useGeometry; }

  /**
   * Access: inputOutput.
   * @details useGeometry indicates whether collision-detection code checks down
   * to level of geometry, or only make approximations using geometry bounds.
   */
  void setUseGeometry(const SFBool &value) { _useGeometry = value; }

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
  MFNode _collidables{};

  SFBool _useGeometry{false};
};

} // namespace x3d::nodes
