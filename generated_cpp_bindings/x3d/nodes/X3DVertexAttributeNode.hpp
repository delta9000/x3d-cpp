// X3DVertexAttributeNode.hpp
#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DGeometricPropertyNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class X3DVertexAttributeNode
 * @brief Base type for all nodes that specify per-vertex attribute information
 * to the shader.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/shaders.html#X3DVertexAttributeNode
 */
class X3DVertexAttributeNode : public virtual X3DGeometricPropertyNode {
public:
  /**
   * @brief Default constructor for X3DVertexAttributeNode
   */
  X3DVertexAttributeNode() = default;

  /**
   * @brief Virtual destructor for X3DVertexAttributeNode
   */
  virtual ~X3DVertexAttributeNode() = default;

  /**
   * @brief Get the default container field value
   * @details No explicit containerField in the spec for this node; falls back
   *          to the X3D default field "children".
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "children"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Shaders"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of name. AccessType: initializeOnly
   * @details
   * @return xs_nmtoken The current value of name.
   */
  xs_nmtoken getName() const { return _name; }
  /**
   * @brief Data-layer write of name (reader/init ingest path).
   * @details name is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setName().
   */
  void setNameUnchecked(const xs_nmtoken &value) { _name = value; }

  /**
   * @brief The X3D type name of this node (e.g. "X3DVertexAttributeNode").
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
   * @brief Member variable for name.
   */

  xs_nmtoken _name{};
};

} // namespace x3d::nodes
