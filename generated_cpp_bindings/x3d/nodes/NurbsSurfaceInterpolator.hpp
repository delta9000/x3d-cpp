// NurbsSurfaceInterpolator.hpp
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
  /**
   * @brief Default constructor for NurbsSurfaceInterpolator
   */
  NurbsSurfaceInterpolator() = default;

  /**
   * @brief Destructor for NurbsSurfaceInterpolator
   */
  ~NurbsSurfaceInterpolator() = default;

  /**
   * @brief Get the default value for controlPoint
   * @return SFNode The default value
   */
  static SFNode getDefaultControlPoint() { return nullptr; }

  /**
   * @brief Get the default value for uDimension
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultUDimension() { return 0; }

  /**
   * @brief Get the default value for uOrder
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultUOrder() { return 3; }

  /**
   * @brief Get the default value for vDimension
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultVDimension() { return 0; }

  /**
   * @brief Get the default value for vOrder
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultVOrder() { return 3; }

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
   * @brief Gets the value of normal_changed. AccessType: outputOnly
   * @details Computationaly interpolated output value determined by current key
   * time and corresponding keyValue pair.
   * @return SFVec3f The current value of normal_changed.
   */
  SFVec3f getNormal_changed() const { return _normal_changed; }

  /**
   * @brief Emit an output value on normal_changed. AccessType: outputOnly
   * @details Computationaly interpolated output value determined by current key
   * time and corresponding keyValue pair. outputOnly fields have no
   * author-facing setter; a node's behavior or the runtime calls this to
   * produce an output event. The event cascade reaches it through the reflected
   * field table so producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitNormal_changed(const SFVec3f &value) { _normal_changed = value; }

  /**
   * @brief Gets the value of position_changed. AccessType: outputOnly
   * @details Computationaly interpolated output value determined by current key
   * time and corresponding keyValue pair.
   * @return SFVec3f The current value of position_changed.
   */
  SFVec3f getPosition_changed() const { return _position_changed; }

  /**
   * @brief Emit an output value on position_changed. AccessType: outputOnly
   * @details Computationaly interpolated output value determined by current key
   * time and corresponding keyValue pair. outputOnly fields have no
   * author-facing setter; a node's behavior or the runtime calls this to
   * produce an output event. The event cascade reaches it through the reflected
   * field table so producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitPosition_changed(const SFVec3f &value) { _position_changed = value; }

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
  void onSet_fraction(const SFVec2f &value) {
    if (_onSet_fractionHandler)
      _onSet_fractionHandler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on set_fraction.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void setOnSet_fractionHandler(std::function<void(const SFVec2f &)> handler) {
    _onSet_fractionHandler = std::move(handler);
  }

  /**
   * @brief Gets the value of uDimension. AccessType: initializeOnly
   * @details Number of control points in u dimension.
   * @return SFInt32 The current value of uDimension.
   */
  SFInt32 getUDimension() const { return _uDimension; }
  /**
   * @brief Data-layer write of uDimension (reader/init ingest path).
   * @details uDimension is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setUDimension().
   */
  void setUDimensionUnchecked(const SFInt32 &value) { _uDimension = value; }

  /**
   * @brief Gets the value of uKnot. AccessType: initializeOnly
   * @details Knot vector, where size = number of control points + order of
   * curve.
   * @return MFDouble The current value of uKnot.
   */
  MFDouble getUKnot() const { return _uKnot; }
  /**
   * @brief Data-layer write of uKnot (reader/init ingest path).
   * @details uKnot is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setUKnot().
   */
  void setUKnotUnchecked(const MFDouble &value) { _uKnot = value; }

  /**
   * @brief Gets the value of uOrder. AccessType: initializeOnly
   * @details Define order of surface by polynomials of degree = order-1.
   * @return SFInt32 The current value of uOrder.
   */
  SFInt32 getUOrder() const { return _uOrder; }
  /**
   * @brief Data-layer write of uOrder (reader/init ingest path).
   * @details uOrder is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setUOrder().
   */
  void setUOrderUnchecked(const SFInt32 &value) { _uOrder = value; }

  /**
   * @brief Gets the value of vDimension. AccessType: initializeOnly
   * @details Number of control points in v dimension.
   * @return SFInt32 The current value of vDimension.
   */
  SFInt32 getVDimension() const { return _vDimension; }
  /**
   * @brief Data-layer write of vDimension (reader/init ingest path).
   * @details vDimension is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setVDimension().
   */
  void setVDimensionUnchecked(const SFInt32 &value) { _vDimension = value; }

  /**
   * @brief Gets the value of vKnot. AccessType: initializeOnly
   * @details Knot vector, where size = number of control points + order of
   * curve.
   * @return MFDouble The current value of vKnot.
   */
  MFDouble getVKnot() const { return _vKnot; }
  /**
   * @brief Data-layer write of vKnot (reader/init ingest path).
   * @details vKnot is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setVKnot().
   */
  void setVKnotUnchecked(const MFDouble &value) { _vKnot = value; }

  /**
   * @brief Gets the value of vOrder. AccessType: initializeOnly
   * @details Define order of surface by polynomials of degree = order-1.
   * @return SFInt32 The current value of vOrder.
   */
  SFInt32 getVOrder() const { return _vOrder; }
  /**
   * @brief Data-layer write of vOrder (reader/init ingest path).
   * @details vOrder is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setVOrder().
   */
  void setVOrderUnchecked(const SFInt32 &value) { _vOrder = value; }

  /**
   * @brief Gets the value of weight. AccessType: inputOutput
   * @details Output values for linear interpolation, each corresponding to
   * knots.
   * @return MFDouble The current value of weight.
   */
  MFDouble getWeight() const { return _weight; }

  /**
   * @brief Sets the value of weight. AccessType: inputOutput
   * @details Output values for linear interpolation, each corresponding to
   * knots.
   * @param value The new value for weight.
   */
  void setWeight(const MFDouble &value) { _weight = value; }

  void setWeight(MFDouble &&value) { _weight = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "NurbsSurfaceInterpolator").
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

  SFNode _controlPoint{nullptr};

  /**
   * @brief Member variable for normal_changed.
   */

  SFVec3f _normal_changed{};

  /**
   * @brief Member variable for position_changed.
   */

  SFVec3f _position_changed{};

  /**
   * @brief Registered event handler for set_fraction (inputOnly); empty until
   * set.
   */
  std::function<void(const SFVec2f &)> _onSet_fractionHandler{};

  /**
   * @brief Member variable for uDimension.
   */

  SFInt32 _uDimension{0};

  /**
   * @brief Member variable for uKnot.
   */

  MFDouble _uKnot{};

  /**
   * @brief Member variable for uOrder.
   */

  SFInt32 _uOrder{3};

  /**
   * @brief Member variable for vDimension.
   */

  SFInt32 _vDimension{0};

  /**
   * @brief Member variable for vKnot.
   */

  MFDouble _vKnot{};

  /**
   * @brief Member variable for vOrder.
   */

  SFInt32 _vOrder{3};

  /**
   * @brief Member variable for weight.
   */

  MFDouble _weight{};
};

} // namespace x3d::nodes
