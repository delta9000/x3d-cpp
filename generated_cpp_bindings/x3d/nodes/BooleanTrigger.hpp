// BooleanTrigger.hpp
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

#include "x3d/nodes/X3DTriggerNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class BooleanTrigger
 * @brief BooleanTrigger converts time events to boolean true events.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/eventUtilities.html#BooleanTrigger
 */
class BooleanTrigger : public virtual X3DTriggerNode {
public:
  /**
   * @brief Default constructor for BooleanTrigger
   */
  BooleanTrigger() = default;

  /**
   * @brief Destructor for BooleanTrigger
   */
  ~BooleanTrigger() = default;

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
   * @brief Event handler invoked when an event is received on set_triggerTime.
   * AccessType: inputOnly
   * @details set_triggerTime provides input time event, typical event sent is
   * TouchSensor touchTime. Dispatches to the handler registered via
   * setOnSet_triggerTimeHandler(); a no-op if none is set. The event cascade
   * reaches this through the node's reflected field table, so routing is
   * node-agnostic.
   * @param value The value of the received event.
   */
  void onSet_triggerTime(const SFTime &value) {
    if (_onSet_triggerTimeHandler)
      _onSet_triggerTimeHandler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on
   * set_triggerTime.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void
  setOnSet_triggerTimeHandler(std::function<void(const SFTime &)> handler) {
    _onSet_triggerTimeHandler = std::move(handler);
  }

  /**
   * @brief Gets the value of triggerTrue. AccessType: outputOnly
   * @details triggerTrue outputs a true value whenever a triggerTime event is
   * received.
   * @return SFBool The current value of triggerTrue.
   */
  SFBool getTriggerTrue() const { return _triggerTrue; }

  /**
   * @brief Emit an output value on triggerTrue. AccessType: outputOnly
   * @details triggerTrue outputs a true value whenever a triggerTime event is
   * received. outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitTriggerTrue(const SFBool &value) { _triggerTrue = value; }

  /**
   * @brief The X3D type name of this node (e.g. "BooleanTrigger").
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
   * @brief Registered event handler for set_triggerTime (inputOnly); empty
   * until set.
   */
  std::function<void(const SFTime &)> _onSet_triggerTimeHandler{};

  /**
   * @brief Member variable for triggerTrue.
   */

  SFBool _triggerTrue{};
};

} // namespace x3d::nodes
