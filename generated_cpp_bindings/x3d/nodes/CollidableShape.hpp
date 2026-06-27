// CollidableShape.hpp
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
  /**
   * @brief Default constructor for CollidableShape
   */
  CollidableShape() = default;

  /**
   * @brief Destructor for CollidableShape
   */
  ~CollidableShape() = default;

  /**
   * @brief Get the default value for shape
   * @return SFNode The default value
   */
  static SFNode getDefaultShape() { return nullptr; }

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
   * @brief Gets the value of shape. AccessType: initializeOnly
   * @details The shape field provides a geometry proxy for specifying which
   * geometry best represents the collidable object.
   * @return SFNode The current value of shape.
   */
  SFNode getShape() const { return _shape; }

  /**
   * @brief Acceptable node types for the shape field.
   * @details Permitted X3D node types: Shape
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableShapeNodeTypes() {
    static const std::vector<std::string> types = {"Shape"};
    return types;
  }
  /**
   * @brief Data-layer write of shape (reader/init ingest path).
   * @details shape is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setShape().
   */
  void setShapeUnchecked(const SFNode &value) { _shape = value; }

  /**
   * @brief The X3D type name of this node (e.g. "CollidableShape").
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
   * @brief Member variable for shape.
   */

  SFNode _shape{nullptr};
};

} // namespace x3d::nodes
