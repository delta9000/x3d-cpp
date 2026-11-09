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
 * @class NurbsPositionInterpolator
 * @brief NurbsPositionInterpolator describes a 3D NURBS curve and outputs
 * interpolated position values.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/nurbs.html#NurbsPositionInterpolator
 */
class NurbsPositionInterpolator : public virtual X3DChildNode {
public:
  NurbsPositionInterpolator() = default;

  ~NurbsPositionInterpolator() = default;

  static SFNode getDefaultControlPoint() { return nullptr; }

  static SFInt32 getDefaultOrder() { return 3; }

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
   * MFDouble; access: inputOutput. Borrows the stored value.
   * @details knot vector, where size = number of control points + order of
   * curve.
   */
  const MFDouble &getKnot() const { return _knot; }

  /**
   * Access: inputOutput.
   * @details knot vector, where size = number of control points + order of
   * curve.
   */
  void setKnot(const MFDouble &value) { _knot = value; }

  void setKnot(MFDouble &&value) { _knot = std::move(value); }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details define order of surface by polynomials of degree = order-1.
   */
  const SFInt32 &getOrder() const { return _order; }

  /**
   * Access: inputOutput.
   * @details define order of surface by polynomials of degree = order-1.
   */
  void setOrder(const SFInt32 &value) {

    validateOrder(value);

    _order = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setOrder() for range validation.
   */
  void setOrderUnchecked(const SFInt32 &value) { _order = value; }

  /**
   * SFFloat; access: inputOnly.
   * @details setting fraction in range [0,1] selects input key for
   * corresponding keyValue output, computing a 3D position on the curve.
   *          Dispatches to the handler registered via
   * setOnSet_fractionHandler(); a no-op if none is set.
   */
  void onSet_fraction(const SFFloat &value) {
    if (_onSet_fractionHandler)
      _onSet_fractionHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnSet_fractionHandler(std::function<void(const SFFloat &)> handler) {
    _onSet_fractionHandler = std::move(handler);
  }

  /**
   * SFVec3f; access: outputOnly. Borrows the stored value.
   * @details Computationaly interpolated output value determined by current key
   * time and corresponding keyValue pair.
   */
  const SFVec3f &getValue_changed() const { return _value_changed; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Computationaly interpolated output value determined by current key
   * time and corresponding keyValue pair.
   */
  void emitValue_changed(const SFVec3f &value) { _value_changed = value; }

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
  static void checkRangesOrder(const SFInt32 &value,
                               const std::string &nodeType,
                               const std::string &defName,
                               std::vector<RangeDiagnostic> &out);

private:
  static void validateOrder(const SFInt32 &value) {

    if (value < 2)
      throw std::out_of_range("order below minimum of 2");
  }

  SFNode _controlPoint{nullptr};

  MFDouble _knot{};

  SFInt32 _order{3};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const SFFloat &)> _onSet_fractionHandler{};

  SFVec3f _value_changed{};

  MFDouble _weight{};
};

} // namespace x3d::nodes
