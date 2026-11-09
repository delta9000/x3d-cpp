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

#include "x3d/nodes/X3DBoundedObject.hpp"

#include "x3d/nodes/X3DNBodyCollidableNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class CollidableOffset
 * @brief CollidableOffset repositions geometry relative to center of owning
 * body.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rigidBodyPhysics.html#CollidableOffset
 */
class CollidableOffset : public virtual X3DNBodyCollidableNode {
public:
  CollidableOffset() = default;

  ~CollidableOffset() = default;

  static SFNode getDefaultCollidable() { return nullptr; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "RigidBodyPhysics"; }

  static int componentLevel() { return 1; }

  /**
   * SFNode; access: initializeOnly. Borrows the stored value.
   * @details The collidable field holds a reference to a single nested item of
   * a collidable scene graph.
   */
  const SFNode &getCollidable() const { return _collidable; }

  /**
   * @details Permitted X3D node types: X3DNBodyCollidableNode
   */
  static const std::vector<std::string> &acceptableCollidableNodeTypes() {
    static const std::vector<std::string> types = {"X3DNBodyCollidableNode"};
    return types;
  }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setCollidableUnchecked(const SFNode &value) { _collidable = value; }

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
  SFNode _collidable{nullptr};
};

} // namespace x3d::nodes
