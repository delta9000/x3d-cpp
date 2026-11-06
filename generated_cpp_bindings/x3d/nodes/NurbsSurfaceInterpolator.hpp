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

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class NurbsSurfaceInterpolator
 * @brief NurbsSurfaceInterpolator describes a 3D NURBS curve and outputs
 * interpolated position and normal values.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/nurbs.html#NurbsSurfaceInterpolator
 */
class NurbsSurfaceInterpolator : public virtual X3DChildNode {
public:
  NurbsSurfaceInterpolator() = default;

  ~NurbsSurfaceInterpolator() = default;

  static SFNode getDefaultControlPoint() { return nullptr; }

  static SFInt32 getDefaultUDimension() { return 0; }

  static SFInt32 getDefaultUOrder() { return 3; }

  static SFInt32 getDefaultVDimension() { return 0; }

  static SFInt32 getDefaultVOrder() { return 3; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "NURBS"; }

  static int componentLevel() { return 1; }

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
   * SFVec3f; access: outputOnly. Borrows the stored value.
   * @details Computationaly interpolated output value determined by current key
   * time and corresponding keyValue pair.
   */
  const SFVec3f &getNormal_changed() const { return _normal_changed; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Computationaly interpolated output value determined by current key
   * time and corresponding keyValue pair.
   */
  void emitNormal_changed(const SFVec3f &value) { _normal_changed = value; }

  /**
   * SFVec3f; access: outputOnly. Borrows the stored value.
   * @details Computationaly interpolated output value determined by current key
   * time and corresponding keyValue pair.
   */
  const SFVec3f &getPosition_changed() const { return _position_changed; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Computationaly interpolated output value determined by current key
   * time and corresponding keyValue pair.
   */
  void emitPosition_changed(const SFVec3f &value) { _position_changed = value; }

  /**
   * SFVec2f; access: inputOnly.
   * @details setting fraction in range [0,1] selects input key for
   * corresponding keyValue output, computing a 3D position on the curve.
   *          Dispatches to the handler registered via
   * setOnSet_fractionHandler(); a no-op if none is set.
   */
  void onSet_fraction(const SFVec2f &value) {
    if (_onSet_fractionHandler)
      _onSet_fractionHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnSet_fractionHandler(std::function<void(const SFVec2f &)> handler) {
    _onSet_fractionHandler = std::move(handler);
  }

  /**
   * SFInt32; access: initializeOnly. Borrows the stored value.
   * @details Number of control points in u dimension.
   */
  const SFInt32 &getUDimension() const { return _uDimension; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setUDimensionUnchecked(const SFInt32 &value) { _uDimension = value; }

  /**
   * MFDouble; access: initializeOnly. Borrows the stored value.
   * @details Knot vector, where size = number of control points + order of
   * curve.
   */
  const MFDouble &getUKnot() const { return _uKnot; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setUKnotUnchecked(const MFDouble &value) { _uKnot = value; }

  /**
   * SFInt32; access: initializeOnly. Borrows the stored value.
   * @details Define order of surface by polynomials of degree = order-1.
   */
  const SFInt32 &getUOrder() const { return _uOrder; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setUOrderUnchecked(const SFInt32 &value) { _uOrder = value; }

  /**
   * SFInt32; access: initializeOnly. Borrows the stored value.
   * @details Number of control points in v dimension.
   */
  const SFInt32 &getVDimension() const { return _vDimension; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setVDimensionUnchecked(const SFInt32 &value) { _vDimension = value; }

  /**
   * MFDouble; access: initializeOnly. Borrows the stored value.
   * @details Knot vector, where size = number of control points + order of
   * curve.
   */
  const MFDouble &getVKnot() const { return _vKnot; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setVKnotUnchecked(const MFDouble &value) { _vKnot = value; }

  /**
   * SFInt32; access: initializeOnly. Borrows the stored value.
   * @details Define order of surface by polynomials of degree = order-1.
   */
  const SFInt32 &getVOrder() const { return _vOrder; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setVOrderUnchecked(const SFInt32 &value) { _vOrder = value; }

  /**
   * MFDouble; access: inputOutput. Borrows the stored value.
   * @details Output values for linear interpolation, each corresponding to
   * knots.
   */
  const MFDouble &getWeight() const { return _weight; }

  /**
   * Access: inputOutput.
   * @details Output values for linear interpolation, each corresponding to
   * knots.
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
  static void checkRangesUDimension(const SFInt32 &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesUOrder(const SFInt32 &value,
                                const std::string &nodeType,
                                const std::string &defName,
                                std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesVDimension(const SFInt32 &value,
                                    const std::string &nodeType,
                                    const std::string &defName,
                                    std::vector<RangeDiagnostic> &out);

  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesVOrder(const SFInt32 &value,
                                const std::string &nodeType,
                                const std::string &defName,
                                std::vector<RangeDiagnostic> &out);

private:
  SFNode _controlPoint{nullptr};

  SFVec3f _normal_changed{};

  SFVec3f _position_changed{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const SFVec2f &)> _onSet_fractionHandler{};

  SFInt32 _uDimension{0};

  MFDouble _uKnot{};

  SFInt32 _uOrder{3};

  SFInt32 _vDimension{0};

  MFDouble _vKnot{};

  SFInt32 _vOrder{3};

  MFDouble _weight{};
};

} // namespace x3d::nodes
