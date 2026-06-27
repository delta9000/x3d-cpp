// X3DBindableNode.hpp
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
 * @class X3DBindableNode
 * @brief Bindable nodes implement the binding stack, so that only one of each
 * node type is active at a given time.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/core.html#X3DBindableNode
 */
class X3DBindableNode : public virtual X3DChildNode {
public:
  /**
   * @brief Default constructor for X3DBindableNode
   */
  X3DBindableNode() = default;

  /**
   * @brief Virtual destructor for X3DBindableNode
   */
  virtual ~X3DBindableNode() = default;

  /**
   * @brief Get the default container field value
   * @details No explicit containerField in the spec for this node; falls back
   *          to the X3D default field "children".
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "children"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "Core"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of bindTime. AccessType: outputOnly
   * @details
   * @return SFTime The current value of bindTime.
   */
  SFTime getBindTime() const { return _bindTime; }

  /**
   * @brief Emit an output value on bindTime. AccessType: outputOnly
   * @details
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitBindTime(const SFTime &value) { _bindTime = value; }

  /**
   * @brief Gets the value of isBound. AccessType: outputOnly
   * @details
   * @return SFBool The current value of isBound.
   */
  SFBool getIsBound() const { return _isBound; }

  /**
   * @brief Emit an output value on isBound. AccessType: outputOnly
   * @details
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitIsBound(const SFBool &value) { _isBound = value; }

  /**
   * @brief Event handler invoked when an event is received on set_bind.
   * AccessType: inputOnly
   * @details
   *          Dispatches to the handler registered via setOnSet_bindHandler();
   *          a no-op if none is set. The event cascade reaches this through the
   *          node's reflected field table, so routing is node-agnostic.
   * @param value The value of the received event.
   */
  void onSet_bind(const SFBool &value) {
    if (_onSet_bindHandler)
      _onSet_bindHandler(value);
  }

  /**
   * @brief Register the handler invoked when an event arrives on set_bind.
   * @param handler Callback receiving the event value (replaces any prior
   * handler).
   */
  void setOnSet_bindHandler(std::function<void(const SFBool &)> handler) {
    _onSet_bindHandler = std::move(handler);
  }

  /**
   * @brief The X3D type name of this node (e.g. "X3DBindableNode").
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
   * @brief Member variable for bindTime.
   */

  SFTime _bindTime{};

  /**
   * @brief Member variable for isBound.
   */

  SFBool _isBound{};

  /**
   * @brief Registered event handler for set_bind (inputOnly); empty until set.
   */
  std::function<void(const SFBool &)> _onSet_bindHandler{};
};

} // namespace x3d::nodes
