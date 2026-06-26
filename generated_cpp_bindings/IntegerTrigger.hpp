// IntegerTrigger.hpp
#ifndef INTEGERTRIGGER_HPP
#define INTEGERTRIGGER_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

#include "X3DTriggerNode.hpp"

/**
 * @class IntegerTrigger
 * @brief IntegerTrigger converts set_boolean true input events to an integer
 * value (for example, useful when animating whichChoice in a Switch node).
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/eventUtilities.html#IntegerTrigger
 */
class IntegerTrigger : public virtual X3DTriggerNode {
public:
  /**
   * @brief Default constructor for IntegerTrigger
   */
  IntegerTrigger() = default;

  /**
   * @brief Destructor for IntegerTrigger
   */
  ~IntegerTrigger() = default;

  /**
   * @brief Get the default value for integerKey
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultIntegerKey() { return -1; }

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
   * @brief Gets the value of integerKey. AccessType: inputOutput
   * @details integerKey is value for output when triggered.
   * @return SFInt32 The current value of integerKey.
   */
  SFInt32 getIntegerKey() const { return _integerKey; }

  /**
   * @brief Sets the value of integerKey. AccessType: inputOutput
   * @details integerKey is value for output when triggered.
   * @param value The new value for integerKey.
   */
  void setIntegerKey(const SFInt32 &value) { _integerKey = value; }

  /**
   * @brief Event handler invoked when an event is received on set_boolean.
   * AccessType: inputOnly
   * @details If input event set_boolean is true, trigger output of integer
   * value. Dispatches to the handler registered via setOnSet_booleanHandler();
   *          a no-op if none is set. The event cascade reaches this through the
   *          node's reflected field table, so routing is node-agnostic.
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
   * @brief Gets the value of triggerValue. AccessType: outputOnly
   * @details triggerValue provides integer event output matching integerKey
   * when true set_boolean received.
   * @return SFInt32 The current value of triggerValue.
   */
  SFInt32 getTriggerValue() const { return _triggerValue; }

  /**
   * @brief Emit an output value on triggerValue. AccessType: outputOnly
   * @details triggerValue provides integer event output matching integerKey
   * when true set_boolean received. outputOnly fields have no author-facing
   * setter; a node's behavior or the runtime calls this to produce an output
   * event. The event cascade reaches it through the reflected field table so
   * producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitTriggerValue(const SFInt32 &value) { _triggerValue = value; }

  /**
   * @brief The X3D type name of this node (e.g. "IntegerTrigger").
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
   * @brief Member variable for integerKey.
   */

  SFInt32 _integerKey{-1};

  /**
   * @brief Registered event handler for set_boolean (inputOnly); empty until
   * set.
   */
  std::function<void(const SFBool &)> _onSet_booleanHandler{};

  /**
   * @brief Member variable for triggerValue.
   */

  SFInt32 _triggerValue{};
};

#endif // INTEGERTRIGGER_HPP