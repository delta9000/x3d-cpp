// KeySensor.hpp
#ifndef KEYSENSOR_HPP
#define KEYSENSOR_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

#include "X3DSensorNode.hpp"

#include "X3DKeyDeviceSensorNode.hpp"

/**
 * @class KeySensor
 * @brief KeySensor generates events as the user presses keys on the keyboard.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/keyDeviceSensor.html#KeySensor
 */
class KeySensor : public virtual X3DKeyDeviceSensorNode {
public:
  /**
   * @brief Default constructor for KeySensor
   */
  KeySensor() = default;

  /**
   * @brief Destructor for KeySensor
   */
  ~KeySensor() = default;

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
  static std::string componentName() { return "KeyDeviceSensor"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of actionKeyPress. AccessType: outputOnly
   * @details action key press gives following values: HOME=000 END=1001
   * PGUP=1002 PGDN=1003 UP=1004 DOWN=1005 LEFT=1006 RIGHT=1007 F1.
   * @return SFInt32 The current value of actionKeyPress.
   */
  SFInt32 getActionKeyPress() const { return _actionKeyPress; }

  /**
   * @brief Emit an output value on actionKeyPress. AccessType: outputOnly
   * @details action key press gives following values: HOME=000 END=1001
   * PGUP=1002 PGDN=1003 UP=1004 DOWN=1005 LEFT=1006 RIGHT=1007 F1. outputOnly
   * fields have no author-facing setter; a node's behavior or the runtime calls
   * this to produce an output event. The event cascade reaches it through the
   * reflected field table so producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitActionKeyPress(const SFInt32 &value) { _actionKeyPress = value; }

  /**
   * @brief Gets the value of actionKeyRelease. AccessType: outputOnly
   * @details action key release gives following values: HOME=000 END=1001
   * PGUP=1002 PGDN=1003 UP=1004 DOWN=1005 LEFT=1006 RIGHT=1007 F1.
   * @return SFInt32 The current value of actionKeyRelease.
   */
  SFInt32 getActionKeyRelease() const { return _actionKeyRelease; }

  /**
   * @brief Emit an output value on actionKeyRelease. AccessType: outputOnly
   * @details action key release gives following values: HOME=000 END=1001
   * PGUP=1002 PGDN=1003 UP=1004 DOWN=1005 LEFT=1006 RIGHT=1007 F1. outputOnly
   * fields have no author-facing setter; a node's behavior or the runtime calls
   * this to produce an output event. The event cascade reaches it through the
   * reflected field table so producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitActionKeyRelease(const SFInt32 &value) { _actionKeyRelease = value; }

  /**
   * @brief Gets the value of altKey. AccessType: outputOnly
   * @details altKey generates true event when pressed, false event when
   * released.
   * @return SFBool The current value of altKey.
   */
  SFBool getAltKey() const { return _altKey; }

  /**
   * @brief Emit an output value on altKey. AccessType: outputOnly
   * @details altKey generates true event when pressed, false event when
   * released. outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitAltKey(const SFBool &value) { _altKey = value; }

  /**
   * @brief Gets the value of controlKey. AccessType: outputOnly
   * @details controlKey generates true event when pressed, false event when
   * released.
   * @return SFBool The current value of controlKey.
   */
  SFBool getControlKey() const { return _controlKey; }

  /**
   * @brief Emit an output value on controlKey. AccessType: outputOnly
   * @details controlKey generates true event when pressed, false event when
   * released. outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitControlKey(const SFBool &value) { _controlKey = value; }

  /**
   * @brief Gets the value of keyPress. AccessType: outputOnly
   * @details Events generated when user presses character-producing keys on
   * keyboard produces integer UTF-8 character values.
   * @return SFString The current value of keyPress.
   */
  SFString getKeyPress() const { return _keyPress; }

  /**
   * @brief Emit an output value on keyPress. AccessType: outputOnly
   * @details Events generated when user presses character-producing keys on
   * keyboard produces integer UTF-8 character values. outputOnly fields have no
   * author-facing setter; a node's behavior or the runtime calls this to
   * produce an output event. The event cascade reaches it through the reflected
   * field table so producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitKeyPress(const SFString &value) { _keyPress = value; }

  /**
   * @brief Gets the value of keyRelease. AccessType: outputOnly
   * @details Events generated when user releases character-producing keys on
   * keyboard produces integer UTF-8 character values.
   * @return SFString The current value of keyRelease.
   */
  SFString getKeyRelease() const { return _keyRelease; }

  /**
   * @brief Emit an output value on keyRelease. AccessType: outputOnly
   * @details Events generated when user releases character-producing keys on
   * keyboard produces integer UTF-8 character values. outputOnly fields have no
   * author-facing setter; a node's behavior or the runtime calls this to
   * produce an output event. The event cascade reaches it through the reflected
   * field table so producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitKeyRelease(const SFString &value) { _keyRelease = value; }

  /**
   * @brief Gets the value of shiftKey. AccessType: outputOnly
   * @details shiftKey generates true event when pressed, false event when
   * released.
   * @return SFBool The current value of shiftKey.
   */
  SFBool getShiftKey() const { return _shiftKey; }

  /**
   * @brief Emit an output value on shiftKey. AccessType: outputOnly
   * @details shiftKey generates true event when pressed, false event when
   * released. outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitShiftKey(const SFBool &value) { _shiftKey = value; }

  /**
   * @brief The X3D type name of this node (e.g. "KeySensor").
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
   * @brief Member variable for actionKeyPress.
   */

  SFInt32 _actionKeyPress{};

  /**
   * @brief Member variable for actionKeyRelease.
   */

  SFInt32 _actionKeyRelease{};

  /**
   * @brief Member variable for altKey.
   */

  SFBool _altKey{};

  /**
   * @brief Member variable for controlKey.
   */

  SFBool _controlKey{};

  /**
   * @brief Member variable for keyPress.
   */

  SFString _keyPress{};

  /**
   * @brief Member variable for keyRelease.
   */

  SFString _keyRelease{};

  /**
   * @brief Member variable for shiftKey.
   */

  SFBool _shiftKey{};
};

#endif // KEYSENSOR_HPP