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
 * @class CollidableShape
 * @brief CollidableShape connects the collision detection system, the rigid
 * body model, and the renderable scene graph.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rigidBodyPhysics.html#CollidableShape
 */
class CollidableShape : public virtual X3DNBodyCollidableNode {
public:
  CollidableShape() = default;

  ~CollidableShape() = default;

  static SFNode getDefaultShape() { return nullptr; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "RigidBodyPhysics"; }

  static int componentLevel() { return 1; }

  /**
   * SFNode; access: initializeOnly. Borrows the stored value.
   * @details The shape field provides a geometry proxy for specifying which
   * geometry best represents the collidable object.
   */
  const SFNode &getShape() const { return _shape; }

  /**
   * @details Permitted X3D node types: Shape
   */
  static const std::vector<std::string> &acceptableShapeNodeTypes() {
    static const std::vector<std::string> types = {"Shape"};
    return types;
  }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setShapeUnchecked(const SFNode &value) { _shape = value; }

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
  SFNode _shape{nullptr};
};

} // namespace x3d::nodes
