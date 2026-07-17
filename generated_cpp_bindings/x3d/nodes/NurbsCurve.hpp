// NurbsCurve.hpp
#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DGeometryNode.hpp"

#include "x3d/nodes/X3DParametricGeometryNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class NurbsCurve
 * @brief NurbsCurve is a 3D curve analogous to NurbsPatchSurface.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/nurbs.html#NurbsCurve
 */
class NurbsCurve : public virtual X3DParametricGeometryNode {
public:
  /**
   * @brief Default constructor for NurbsCurve
   */
  NurbsCurve() = default;

  /**
   * @brief Destructor for NurbsCurve
   */
  ~NurbsCurve() = default;

  /**
   * @brief Get the default value for closed
   * @return SFBool The default value
   */
  static SFBool getDefaultClosed() { return false; }

  /**
   * @brief Get the default value for controlPoint
   * @return SFNode The default value
   */
  static SFNode getDefaultControlPoint() { return nullptr; }

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
  static std::string getDefaultContainerField() { return "geometry"; }

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
   * @brief Gets the value of controlPoint. AccessType: inputOutput
   * @details Single contained Coordinate or CoordinateDouble node that can
   * specify control points for NURBS geometry definitions.
   * @return SFNode The current value of controlPoint.
   */
  SFNode getControlPoint() const { return _controlPoint; }

  /**
   * @brief Acceptable node types for the controlPoint field.
   * @details Permitted X3D node types: Coordinate, CoordinateDouble
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableControlPointNodeTypes() {
    static const std::vector<std::string> types = {"Coordinate",
                                                   "CoordinateDouble"};
    return types;
  }

  /**
   * @brief Sets the value of controlPoint. AccessType: inputOutput
   * @details Single contained Coordinate or CoordinateDouble node that can
   * specify control points for NURBS geometry definitions.
   * @param value The new value for controlPoint.
   */
  void setControlPoint(const SFNode &value) { _controlPoint = value; }

  void setControlPoint(SFNode &&value) { _controlPoint = std::move(value); }

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
   * @brief The X3D type name of this node (e.g. "NurbsCurve").
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

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

protected:
  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesOrder(const SFInt32 &value,
                               const std::string &nodeType,
                               const std::string &defName,
                               std::vector<RangeDiagnostic> &out);

private:
  /**
   * @brief Member variable for closed.
   */

  SFBool _closed{false};

  /**
   * @brief Member variable for controlPoint.
   */

  SFNode _controlPoint{nullptr};

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

} // namespace x3d::nodes
