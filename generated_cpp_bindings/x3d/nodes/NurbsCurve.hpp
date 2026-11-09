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
  NurbsCurve() = default;

  ~NurbsCurve() = default;

  static SFBool getDefaultClosed() { return false; }

  static SFNode getDefaultControlPoint() { return nullptr; }

  static SFInt32 getDefaultOrder() { return 3; }

  static SFInt32 getDefaultTessellation() { return 0; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "geometry"; }

  static std::string componentName() { return "NURBS"; }

  static int componentLevel() { return 1; }

  /**
   * SFBool; access: initializeOnly. Borrows the stored value.
   * @details Whether or not the curve is closed (i.
   */
  const SFBool &getClosed() const { return _closed; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setClosedUnchecked(const SFBool &value) { _closed = value; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details Single contained Coordinate or CoordinateDouble node that can
   * specify control points for NURBS geometry definitions.
   */
  const SFNode &getControlPoint() const { return _controlPoint; }

  /**
   * @details Permitted X3D node types: Coordinate, CoordinateDouble
   */
  static const std::vector<std::string> &acceptableControlPointNodeTypes() {
    static const std::vector<std::string> types = {"Coordinate",
                                                   "CoordinateDouble"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details Single contained Coordinate or CoordinateDouble node that can
   * specify control points for NURBS geometry definitions.
   */
  void setControlPoint(const SFNode &value) { _controlPoint = value; }

  void setControlPoint(SFNode &&value) { _controlPoint = std::move(value); }

  /**
   * MFDouble; access: initializeOnly. Borrows the stored value.
   * @details knot vector, where size = number of control points + order of
   * curve.
   */
  const MFDouble &getKnot() const { return _knot; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setKnotUnchecked(const MFDouble &value) { _knot = value; }

  /**
   * SFInt32; access: initializeOnly. Borrows the stored value.
   * @details define order of surface by polynomials of degree = order-1.
   */
  const SFInt32 &getOrder() const { return _order; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setOrderUnchecked(const SFInt32 &value) { _order = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details hint for surface tessellation.
   */
  const SFInt32 &getTessellation() const { return _tessellation; }

  /**
   * Access: inputOutput.
   * @details hint for surface tessellation.
   */
  void setTessellation(const SFInt32 &value) { _tessellation = value; }

  /**
   * MFDouble; access: inputOutput. Borrows the stored value.
   * @details Vector assigning relative weight value to each control point.
   */
  const MFDouble &getWeight() const { return _weight; }

  /**
   * Access: inputOutput.
   * @details Vector assigning relative weight value to each control point.
   */
  void setWeight(const MFDouble &value) { _weight = value; }

  void setWeight(MFDouble &&value) { _weight = std::move(value); }

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
  SFBool _closed{false};

  SFNode _controlPoint{nullptr};

  MFDouble _knot{};

  SFInt32 _order{3};

  SFInt32 _tessellation{0};

  MFDouble _weight{};
};

} // namespace x3d::nodes
