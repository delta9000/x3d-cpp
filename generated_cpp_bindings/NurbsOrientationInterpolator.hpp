// NurbsOrientationInterpolator.hpp
#ifndef NURBSORIENTATIONINTERPOLATOR_HPP
#define NURBSORIENTATIONINTERPOLATOR_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

/**
 * @class NurbsOrientationInterpolator
 * @brief NurbsOrientationInterpolator describes a 3D NURBS curve and outputs
 * interpolated orientation values.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/nurbs.html#NurbsOrientationInterpolator
 */
class NurbsOrientationInterpolator : public virtual X3DChildNode {
public:
  /**
   * @brief Default constructor for NurbsOrientationInterpolator
   */
  NurbsOrientationInterpolator() = default;

  /**
   * @brief Destructor for NurbsOrientationInterpolator
   */
  ~NurbsOrientationInterpolator() = default;

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
  static int componentLevel() { return 1; }

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
   * @brief Gets the value of knot. AccessType: inputOutput
   * @details knot vector, where size = number of control points + order of
   * curve.
   * @return MFDouble The current value of knot.
   */
  MFDouble getKnot() const { return _knot; }

  /**
   * @brief Sets the value of knot. AccessType: inputOutput
   * @details knot vector, where size = number of control points + order of
   * curve.
   * @param value The new value for knot.
   */
  void setKnot(const MFDouble &value) { _knot = value; }

  void setKnot(MFDouble &&value) { _knot = std::move(value); }

  /**
   * @brief Gets the value of order. AccessType: inputOutput
   * @details define order of surface by polynomials of degree = order-1.
   * @return SFInt32 The current value of order.
   */
  SFInt32 getOrder() const { return _order; }

  /**
   * @brief Sets the value of order. AccessType: inputOutput
   * @details define order of surface by polynomials of degree = order-1.
   * @param value The new value for order.
   */
  void setOrder(const SFInt32 &value) {

    validateOrder(value);

    _order = value;
  }

  /**
   * @brief Non-validating write of order (runtime/reader ingest path).
   * @details Assigns without the range check setOrder() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setOrder() stays the
   *          enforcement point for programmatic callers.
   */
  void setOrderUnchecked(const SFInt32 &value) { _order = value; }

  /**
   * @brief Event handler invoked when an event is received on set_fraction.
   * AccessType: inputOnly
   * @details setting fraction in range [0,1] selects input key for
   * corresponding keyValue output, computing a 3D position on the curve.
   *          Dispatches to the handler registered via
   * setOnSet_fractionHandler(); a no-op if none is set. The event cascade
   * reaches this through the node's reflected field table, so routing is
   * node-agnostic.
   * @param value The value of the received event.
   */
  void onSet_fraction(const SFFloat &value) {
    if (_onSet_fractionHandler)
      _onSet_fractionHandler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on set_fraction.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void setOnSet_fractionHandler(std::function<void(const SFFloat &)> handler) {
    _onSet_fractionHandler = std::move(handler);
  }

  /**
   * @brief Gets the value of value_changed. AccessType: outputOnly
   * @details Computationaly interpolated output value determined by current key
   * time and corresponding keyValue pair.
   * @return SFRotation The current value of value_changed.
   */
  SFRotation getValue_changed() const { return _value_changed; }

  /**
   * @brief Emit an output value on value_changed. AccessType: outputOnly
   * @details Computationaly interpolated output value determined by current key
   * time and corresponding keyValue pair. outputOnly fields have no
   * author-facing setter; a node's behavior or the runtime calls this to
   * produce an output event. The event cascade reaches it through the reflected
   * field table so producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitValue_changed(const SFRotation &value) { _value_changed = value; }

  /**
   * @brief Gets the value of weight. AccessType: inputOutput
   * @details Output values for computational interpolation, each corresponding
   * to knots.
   * @return MFDouble The current value of weight.
   */
  MFDouble getWeight() const { return _weight; }

  /**
   * @brief Sets the value of weight. AccessType: inputOutput
   * @details Output values for computational interpolation, each corresponding
   * to knots.
   * @param value The new value for weight.
   */
  void setWeight(const MFDouble &value) { _weight = value; }

  void setWeight(MFDouble &&value) { _weight = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g.
   * "NurbsOrientationInterpolator").
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
  static void validateOrder(const SFInt32 &value) {

    if (value < 2)
      throw std::out_of_range("order below minimum of 2");
  }

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
   * @brief Registered event handler for set_fraction (inputOnly); empty until
   * set.
   */
  std::function<void(const SFFloat &)> _onSet_fractionHandler{};

  /**
   * @brief Member variable for value_changed.
   */

  SFRotation _value_changed{};

  /**
   * @brief Member variable for weight.
   */

  MFDouble _weight{};
};

#endif // NURBSORIENTATIONINTERPOLATOR_HPP