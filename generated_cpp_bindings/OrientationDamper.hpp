// OrientationDamper.hpp
#ifndef ORIENTATIONDAMPER_HPP
#define ORIENTATIONDAMPER_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

#include "X3DFollowerNode.hpp"

#include "X3DDamperNode.hpp"

/**
 * @class OrientationDamper
 * @brief OrientationDamper generates a series of 4-tuple axis-angle SFRotation
 * values that progressively change from initial value to destination value.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/followers.html#OrientationDamper
 */
class OrientationDamper : public virtual X3DDamperNode {
public:
  /**
   * @brief Default constructor for OrientationDamper
   */
  OrientationDamper() = default;

  /**
   * @brief Destructor for OrientationDamper
   */
  ~OrientationDamper() = default;

  /**
   * @brief Get the default value for initialDestination
   * @return SFRotation The default value
   */
  static SFRotation getDefaultInitialDestination() {
    return SFRotation{0, 1, 0, 0};
  }

  /**
   * @brief Get the default value for initialValue
   * @return SFRotation The default value
   */
  static SFRotation getDefaultInitialValue() { return SFRotation{0, 1, 0, 0}; }

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
  static std::string componentName() { return "Followers"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of initialDestination. AccessType: initializeOnly
   * @details Initial destination value for this node.
   * @return SFRotation The current value of initialDestination.
   */
  SFRotation getInitialDestination() const { return _initialDestination; }
  /**
   * @brief Data-layer write of initialDestination (reader/init ingest path).
   * @details initialDestination is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public
   * setInitialDestination().
   */
  void setInitialDestinationUnchecked(const SFRotation &value) {
    _initialDestination = value;
  }

  /**
   * @brief Gets the value of initialValue. AccessType: initializeOnly
   * @details Initial starting value for this node.
   * @return SFRotation The current value of initialValue.
   */
  SFRotation getInitialValue() const { return _initialValue; }
  /**
   * @brief Data-layer write of initialValue (reader/init ingest path).
   * @details initialValue is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setInitialValue().
   */
  void setInitialValueUnchecked(const SFRotation &value) {
    _initialValue = value;
  }

  /**
   * @brief Event handler invoked when an event is received on set_destination.
   * AccessType: inputOnly
   * @details set_destination resets destination value of this node.
   *          Dispatches to the handler registered via
   * setOnSet_destinationHandler(); a no-op if none is set. The event cascade
   * reaches this through the node's reflected field table, so routing is
   * node-agnostic.
   * @param value The value of the received event.
   */
  void onSet_destination(const SFRotation &value) {
    if (_onSet_destinationHandler)
      _onSet_destinationHandler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on
   * set_destination.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void
  setOnSet_destinationHandler(std::function<void(const SFRotation &)> handler) {
    _onSet_destinationHandler = std::move(handler);
  }

  /**
   * @brief Event handler invoked when an event is received on set_value.
   * AccessType: inputOnly
   * @details set_value resets current value of this node.
   *          Dispatches to the handler registered via setOnSet_valueHandler();
   *          a no-op if none is set. The event cascade reaches this through the
   *          node's reflected field table, so routing is node-agnostic.
   * @param value The value of the received event.
   */
  void onSet_value(const SFRotation &value) {
    if (_onSet_valueHandler)
      _onSet_valueHandler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on set_value.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void setOnSet_valueHandler(std::function<void(const SFRotation &)> handler) {
    _onSet_valueHandler = std::move(handler);
  }

  /**
   * @brief Gets the value of value_changed. AccessType: outputOnly
   * @details Computed output value that approaches within tolerance of
   * destination value, as determined by elapsed time, order and tau.
   * @return SFRotation The current value of value_changed.
   */
  SFRotation getValue_changed() const { return _value_changed; }

  /**
   * @brief Emit an output value on value_changed. AccessType: outputOnly
   * @details Computed output value that approaches within tolerance of
   * destination value, as determined by elapsed time, order and tau. outputOnly
   * fields have no author-facing setter; a node's behavior or the runtime calls
   * this to produce an output event. The event cascade reaches it through the
   * reflected field table so producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitValue_changed(const SFRotation &value) { _value_changed = value; }

  /**
   * @brief The X3D type name of this node (e.g. "OrientationDamper").
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

private:
  /**
   * @brief Member variable for initialDestination.
   */

  SFRotation _initialDestination{SFRotation{0, 1, 0, 0}};

  /**
   * @brief Member variable for initialValue.
   */

  SFRotation _initialValue{SFRotation{0, 1, 0, 0}};

  /**
   * @brief Registered event handler for set_destination (inputOnly); empty
   * until set.
   */
  std::function<void(const SFRotation &)> _onSet_destinationHandler{};

  /**
   * @brief Registered event handler for set_value (inputOnly); empty until set.
   */
  std::function<void(const SFRotation &)> _onSet_valueHandler{};

  /**
   * @brief Member variable for value_changed.
   */

  SFRotation _value_changed{};
};

#endif // ORIENTATIONDAMPER_HPP