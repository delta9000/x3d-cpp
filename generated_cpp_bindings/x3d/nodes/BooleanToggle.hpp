// BooleanToggle.hpp
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
 * @class BooleanToggle
 * @brief BooleanToggle maintains state and negates output when a true input is
 * provided.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/eventUtilities.html#BooleanToggle
 */
class BooleanToggle : public virtual X3DChildNode {
public:
  /**
   * @brief Default constructor for BooleanToggle
   */
  BooleanToggle() = default;

  /**
   * @brief Destructor for BooleanToggle
   */
  ~BooleanToggle() = default;

  /**
   * @brief Get the default value for toggle
   * @return SFBool The default value
   */
  static SFBool getDefaultToggle() { return false; }

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
   * @brief Event handler invoked when an event is received on set_boolean.
   * AccessType: inputOnly
   * @details If input event set_boolean is true, flip state by negating current
   * value of the toggle field Hint: for logical consistency, input event
   * set_boolean false has no effect (under review as part of Mantis issue 519).
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
   * @brief Gets the value of toggle. AccessType: inputOutput
   * @details Persistent state value that gets toggled or reset.
   * @return SFBool The current value of toggle.
   */
  SFBool getToggle() const { return _toggle; }

  /**
   * @brief Sets the value of toggle. AccessType: inputOutput
   * @details Persistent state value that gets toggled or reset.
   * @param value The new value for toggle.
   */
  void setToggle(const SFBool &value) { _toggle = value; }

  /**
   * @brief The X3D type name of this node (e.g. "BooleanToggle").
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
   * @brief Registered event handler for set_boolean (inputOnly); empty until
   * set.
   */
  std::function<void(const SFBool &)> _onSet_booleanHandler{};

  /**
   * @brief Member variable for toggle.
   */

  SFBool _toggle{false};
};

} // namespace x3d::nodes
