// BooleanFilter.hpp
#ifndef BOOLEANFILTER_HPP
#define BOOLEANFILTER_HPP

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
 * @class BooleanFilter
 * @brief BooleanFilter selectively passes true, false or negated events.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/eventUtilities.html#BooleanFilter
 */
class BooleanFilter : public virtual X3DChildNode {
public:
  /**
   * @brief Default constructor for BooleanFilter
   */
  BooleanFilter() = default;

  /**
   * @brief Destructor for BooleanFilter
   */
  ~BooleanFilter() = default;

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
  static std::string componentName() { return "EventUtilities"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of inputFalse. AccessType: outputOnly
   * @details inputFalse only passes a false value, which occurs when
   * set_boolean is false.
   * @return SFBool The current value of inputFalse.
   */
  SFBool getInputFalse() const { return _inputFalse; }

  /**
   * @brief Emit an output value on inputFalse. AccessType: outputOnly
   * @details inputFalse only passes a false value, which occurs when
   * set_boolean is false. outputOnly fields have no author-facing setter; a
   * node's behavior or the runtime calls this to produce an output event. The
   * event cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitInputFalse(const SFBool &value) { _inputFalse = value; }

  /**
   * @brief Gets the value of inputNegate. AccessType: outputOnly
   * @details inputNegate is an output event that provides an opposite value by
   * negating set_boolean input.
   * @return SFBool The current value of inputNegate.
   */
  SFBool getInputNegate() const { return _inputNegate; }

  /**
   * @brief Emit an output value on inputNegate. AccessType: outputOnly
   * @details inputNegate is an output event that provides an opposite value by
   * negating set_boolean input. outputOnly fields have no author-facing setter;
   * a node's behavior or the runtime calls this to produce an output event. The
   * event cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitInputNegate(const SFBool &value) { _inputNegate = value; }

  /**
   * @brief Gets the value of inputTrue. AccessType: outputOnly
   * @details inputTrue only passes a true value, which occurs when set_boolean
   * input is true.
   * @return SFBool The current value of inputTrue.
   */
  SFBool getInputTrue() const { return _inputTrue; }

  /**
   * @brief Emit an output value on inputTrue. AccessType: outputOnly
   * @details inputTrue only passes a true value, which occurs when set_boolean
   * input is true. outputOnly fields have no author-facing setter; a node's
   * behavior or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitInputTrue(const SFBool &value) { _inputTrue = value; }

  /**
   * @brief Event handler invoked when an event is received on set_boolean.
   * AccessType: inputOnly
   * @details set_boolean is the input value to be filtered.
   *          Dispatches to the handler registered via
   * setOnSet_booleanHandler(); a no-op if none is set. The event cascade
   * reaches this through the node's reflected field table, so routing is
   * node-agnostic.
   * @param value The value of the received event.
   */
  void onSet_boolean(const SFBool &value) {
    if (_onSet_booleanHandler)
      _onSet_booleanHandler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on set_boolean.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void setOnSet_booleanHandler(std::function<void(const SFBool &)> handler) {
    _onSet_booleanHandler = std::move(handler);
  }

  /**
   * @brief The X3D type name of this node (e.g. "BooleanFilter").
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
   * @brief Member variable for inputFalse.
   */

  SFBool _inputFalse{};

  /**
   * @brief Member variable for inputNegate.
   */

  SFBool _inputNegate{};

  /**
   * @brief Member variable for inputTrue.
   */

  SFBool _inputTrue{};

  /**
   * @brief Registered event handler for set_boolean (inputOnly); empty until
   * set.
   */
  std::function<void(const SFBool &)> _onSet_booleanHandler{};
};

#endif // BOOLEANFILTER_HPP