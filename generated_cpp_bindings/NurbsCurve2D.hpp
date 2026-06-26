// NurbsCurve2D.hpp
#ifndef NURBSCURVE2D_HPP
#define NURBSCURVE2D_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DNurbsControlCurveNode.hpp"

/**
 * @class NurbsCurve2D
 * @brief NurbsCurve2D defines a trimming segment that is part of a trimming
 * contour in the u-v domain of a surface.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/nurbs.html#NurbsCurve2D
 */
class NurbsCurve2D : public virtual X3DNurbsControlCurveNode {
public:
  /**
   * @brief Default constructor for NurbsCurve2D
   */
  NurbsCurve2D() = default;

  /**
   * @brief Destructor for NurbsCurve2D
   */
  ~NurbsCurve2D() = default;

  /**
   * @brief Get the default value for closed
   * @return SFBool The default value
   */
  static SFBool getDefaultClosed() { return false; }

  /**
   * @brief Get the default value for order
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultOrder() { return 3; }

  /**
   * @brief Get the default value for tessellation
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultTessellation() { return 0; }

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
  static std::string componentName() { return "NURBS"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 3; }

  /**
   * @brief Gets the value of closed. AccessType: initializeOnly
   * @details Whether or not the curve is closed (i.
   * @return SFBool The current value of closed.
   */
  SFBool getClosed() const { return _closed; }
  /**
   * @brief Data-layer write of closed (reader/init ingest path).
   * @details closed is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setClosed().
   */
  void setClosedUnchecked(const SFBool &value) { _closed = value; }

  /**
   * @brief Gets the value of knot. AccessType: initializeOnly
   * @details knot vector, where size = number of control points + order of
   * curve.
   * @return MFDouble The current value of knot.
   */
  MFDouble getKnot() const { return _knot; }
  /**
   * @brief Data-layer write of knot (reader/init ingest path).
   * @details knot is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setKnot().
   */
  void setKnotUnchecked(const MFDouble &value) { _knot = value; }

  /**
   * @brief Gets the value of order. AccessType: initializeOnly
   * @details define order of surface by polynomials of degree = order-1.
   * @return SFInt32 The current value of order.
   */
  SFInt32 getOrder() const { return _order; }
  /**
   * @brief Data-layer write of order (reader/init ingest path).
   * @details order is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setOrder().
   */
  void setOrderUnchecked(const SFInt32 &value) { _order = value; }

  /**
   * @brief Gets the value of tessellation. AccessType: inputOutput
   * @details hint for surface tessellation.
   * @return SFInt32 The current value of tessellation.
   */
  SFInt32 getTessellation() const { return _tessellation; }

  /**
   * @brief Sets the value of tessellation. AccessType: inputOutput
   * @details hint for surface tessellation.
   * @param value The new value for tessellation.
   */
  void setTessellation(const SFInt32 &value) { _tessellation = value; }

  /**
   * @brief Gets the value of weight. AccessType: inputOutput
   * @details Vector assigning relative weight value to each control point.
   * @return MFDouble The current value of weight.
   */
  MFDouble getWeight() const { return _weight; }

  /**
   * @brief Sets the value of weight. AccessType: inputOutput
   * @details Vector assigning relative weight value to each control point.
   * @param value The new value for weight.
   */
  void setWeight(const MFDouble &value) { _weight = value; }

  void setWeight(MFDouble &&value) { _weight = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "NurbsCurve2D").
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
   * @brief Member variable for closed.
   */

  SFBool _closed{false};

  /**
   * @brief Member variable for knot.
   */

  MFDouble _knot{};

  /**
   * @brief Member variable for order.
   */

  SFInt32 _order{3};

  /**
   * @brief Member variable for tessellation.
   */

  SFInt32 _tessellation{0};

  /**
   * @brief Member variable for weight.
   */

  MFDouble _weight{};
};

#endif // NURBSCURVE2D_HPP