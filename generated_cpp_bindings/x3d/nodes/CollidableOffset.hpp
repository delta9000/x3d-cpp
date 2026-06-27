// CollidableOffset.hpp
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
  /**
   * @brief Default constructor for CollidableOffset
   */
  CollidableOffset() = default;

  /**
   * @brief Destructor for CollidableOffset
   */
  ~CollidableOffset() = default;

  /**
   * @brief Get the default value for collidable
   * @return SFNode The default value
   */
  static SFNode getDefaultCollidable() { return nullptr; }

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
   * @brief Gets the value of collidable. AccessType: initializeOnly
   * @details The collidable field holds a reference to a single nested item of
   * a collidable scene graph.
   * @return SFNode The current value of collidable.
   */
  SFNode getCollidable() const { return _collidable; }

  /**
   * @brief Acceptable node types for the collidable field.
   * @details Permitted X3D node types: X3DNBodyCollidableNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableCollidableNodeTypes() {
    static const std::vector<std::string> types = {"X3DNBodyCollidableNode"};
    return types;
  }
  /**
   * @brief Data-layer write of collidable (reader/init ingest path).
   * @details collidable is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setCollidable().
   */
  void setCollidableUnchecked(const SFNode &value) { _collidable = value; }

  /**
   * @brief The X3D type name of this node (e.g. "CollidableOffset").
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
   * @brief Member variable for collidable.
   */

  SFNode _collidable{nullptr};
};

} // namespace x3d::nodes
