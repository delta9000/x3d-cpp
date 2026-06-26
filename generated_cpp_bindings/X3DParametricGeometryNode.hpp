// X3DParametricGeometryNode.hpp
#ifndef X3DPARAMETRICGEOMETRYNODE_HPP
#define X3DPARAMETRICGEOMETRYNODE_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DGeometryNode.hpp"

/**
 * @class X3DParametricGeometryNode
 * @brief Base type for all geometry node types that are created parametrically
 * and use control points to describe the final shape of the surface.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/nurbs.html#X3DParametricGeometryNode
 */
class X3DParametricGeometryNode : public virtual X3DGeometryNode {
public:
  /**
   * @brief Default constructor for X3DParametricGeometryNode
   */
  X3DParametricGeometryNode() = default;

  /**
   * @brief Virtual destructor for X3DParametricGeometryNode
   */
  virtual ~X3DParametricGeometryNode() = default;

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
  static std::string componentName() { return "NURBS"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief The X3D type name of this node (e.g. "X3DParametricGeometryNode").
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
};

#endif // X3DPARAMETRICGEOMETRYNODE_HPP