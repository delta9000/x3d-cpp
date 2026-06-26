// X3DNurbsControlCurveNode.hpp
#ifndef X3DNURBSCONTROLCURVENODE_HPP
#define X3DNURBSCONTROLCURVENODE_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

/**
 * @class X3DNurbsControlCurveNode
 * @brief Base type for all nodes that provide control curve information in 2D
 * space.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/nurbs.html#X3DNurbsControlCurveNode
 */
class X3DNurbsControlCurveNode : public virtual X3DNode {
public:
  /**
   * @brief Default constructor for X3DNurbsControlCurveNode
   */
  X3DNurbsControlCurveNode() = default;

  /**
   * @brief Virtual destructor for X3DNurbsControlCurveNode
   */
  virtual ~X3DNurbsControlCurveNode() = default;

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
   * @brief Gets the value of controlPoint. AccessType: inputOutput
   * @details
   * @return MFVec2d The current value of controlPoint.
   */
  MFVec2d getControlPoint() const { return _controlPoint; }

  /**
   * @brief Sets the value of controlPoint. AccessType: inputOutput
   * @details
   * @param value The new value for controlPoint.
   */
  void setControlPoint(const MFVec2d &value) { _controlPoint = value; }

  void setControlPoint(MFVec2d &&value) { _controlPoint = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "X3DNurbsControlCurveNode").
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
   * @brief Member variable for controlPoint.
   */

  MFVec2d _controlPoint{};
};

#endif // X3DNURBSCONTROLCURVENODE_HPP