// EaseInEaseOut.hpp
#ifndef EASEINEASEOUT_HPP
#define EASEINEASEOUT_HPP

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
 * @class EaseInEaseOut
 * @brief EaseInEaseOut enables gradual animation transitions by modifying
 * TimeSensor fraction outputs.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/interpolators.html#EaseInEaseOut
 */
class EaseInEaseOut : public virtual X3DChildNode {
public:
  /**
   * @brief Default constructor for EaseInEaseOut
   */
  EaseInEaseOut() = default;

  /**
   * @brief Destructor for EaseInEaseOut
   */
  ~EaseInEaseOut() = default;

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
  static std::string componentName() { return "Interpolation"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 4; }

  /**
   * @brief Gets the value of easeInEaseOut. AccessType: inputOutput
   * @details Array of paired values for easeOut fraction and easeIn fraction
   * within each key interval.
   * @return MFVec2f The current value of easeInEaseOut.
   */
  MFVec2f getEaseInEaseOut() const { return _easeInEaseOut; }

  /**
   * @brief Sets the value of easeInEaseOut. AccessType: inputOutput
   * @details Array of paired values for easeOut fraction and easeIn fraction
   * within each key interval.
   * @param value The new value for easeInEaseOut.
   */
  void setEaseInEaseOut(const MFVec2f &value) { _easeInEaseOut = value; }

  void setEaseInEaseOut(MFVec2f &&value) { _easeInEaseOut = std::move(value); }

  /**
   * @brief Gets the value of key. AccessType: inputOutput
   * @details Definition values for linear-interpolation function input
   * intervals, listed in non-decreasing order and corresponding to
   * easeInEaseOut array.
   * @return MFFloat The current value of key.
   */
  MFFloat getKey() const { return _key; }

  /**
   * @brief Sets the value of key. AccessType: inputOutput
   * @details Definition values for linear-interpolation function input
   * intervals, listed in non-decreasing order and corresponding to
   * easeInEaseOut array.
   * @param value The new value for key.
   */
  void setKey(const MFFloat &value) { _key = value; }

  void setKey(MFFloat &&value) { _key = std::move(value); }

  /**
   * @brief Gets the value of modifiedFraction_changed. AccessType: outputOnly
   * @details Interpolated output value determined by current key time,
   * corresponding easeInEaseOut smoothing intervals, and corresponding key
   * pair.
   * @return SFFloat The current value of modifiedFraction_changed.
   */
  SFFloat getModifiedFraction_changed() const {
    return _modifiedFraction_changed;
  }

  /**
   * @brief Emit an output value on modifiedFraction_changed. AccessType:
   * outputOnly
   * @details Interpolated output value determined by current key time,
   * corresponding easeInEaseOut smoothing intervals, and corresponding key
   * pair. outputOnly fields have no author-facing setter; a node's behavior or
   * the runtime calls this to produce an output event. The event cascade
   * reaches it through the reflected field table so producing outputs is
   * node-agnostic.
   * @param value The value to emit.
   */
  void emitModifiedFraction_changed(const SFFloat &value) {
    _modifiedFraction_changed = value;
  }

  /**
   * @brief Event handler invoked when an event is received on set_fraction.
   * AccessType: inputOnly
   * @details set_fraction selects input fraction for computation of
   * corresponding easeInEaseOut output value, modifiedFraction_changed.
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
   * @brief The X3D type name of this node (e.g. "EaseInEaseOut").
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
   * @brief Member variable for easeInEaseOut.
   */

  MFVec2f _easeInEaseOut{};

  /**
   * @brief Member variable for key.
   */

  MFFloat _key{};

  /**
   * @brief Member variable for modifiedFraction_changed.
   */

  SFFloat _modifiedFraction_changed{};

  /**
   * @brief Registered event handler for set_fraction (inputOnly); empty until
   * set.
   */
  std::function<void(const SFFloat &)> _onSet_fractionHandler{};
};

#endif // EASEINEASEOUT_HPP