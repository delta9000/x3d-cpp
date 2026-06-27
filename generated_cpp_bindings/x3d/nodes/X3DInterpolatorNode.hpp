// X3DInterpolatorNode.hpp
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
 * @class X3DInterpolatorNode
 * @brief Interpolator nodes are designed for linear keyframed animation.
 * Interpolators are driven by an input key ranging [0..1] and produce
 * corresponding piecewise-linear output functions.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/interpolators.html#X3DInterpolatorNode
 */
class X3DInterpolatorNode : public virtual X3DChildNode {
public:
  /**
   * @brief Default constructor for X3DInterpolatorNode
   */
  X3DInterpolatorNode() = default;

  /**
   * @brief Virtual destructor for X3DInterpolatorNode
   */
  virtual ~X3DInterpolatorNode() = default;

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
  static std::string componentName() { return "Interpolation"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of key. AccessType: inputOutput
   * @details
   * @return MFFloat The current value of key.
   */
  MFFloat getKey() const { return _key; }

  /**
   * @brief Sets the value of key. AccessType: inputOutput
   * @details
   * @param value The new value for key.
   */
  void setKey(const MFFloat &value) { _key = value; }

  void setKey(MFFloat &&value) { _key = std::move(value); }

  /**
   * @brief Event handler invoked when an event is received on set_fraction.
   * AccessType: inputOnly
   * @details
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
   * @brief The X3D type name of this node (e.g. "X3DInterpolatorNode").
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
   * @brief Member variable for key.
   */

  MFFloat _key{};

  /**
   * @brief Registered event handler for set_fraction (inputOnly); empty until
   * set.
   */
  std::function<void(const SFFloat &)> _onSet_fractionHandler{};
};

} // namespace x3d::nodes
