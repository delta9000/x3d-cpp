// StringSensor.hpp
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

#include "x3d/nodes/X3DSensorNode.hpp"

#include "x3d/nodes/X3DKeyDeviceSensorNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class StringSensor
 * @brief StringSensor generates events as the user presses keys on the
 * keyboard.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/keyDeviceSensor.html#StringSensor
 */
class StringSensor : public virtual X3DKeyDeviceSensorNode {
public:
  /**
   * @brief Default constructor for StringSensor
   */
  StringSensor() = default;

  /**
   * @brief Destructor for StringSensor
   */
  ~StringSensor() = default;

  /**
   * @brief Get the default value for deletionAllowed
   * @return SFBool The default value
   */
  static SFBool getDefaultDeletionAllowed() { return true; }

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
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of deletionAllowed. AccessType: inputOutput
   * @details If deletionAllowed is true, then previously entered character in
   * enteredText can be removed.
   * @return SFBool The current value of deletionAllowed.
   */
  SFBool getDeletionAllowed() const { return _deletionAllowed; }

  /**
   * @brief Sets the value of deletionAllowed. AccessType: inputOutput
   * @details If deletionAllowed is true, then previously entered character in
   * enteredText can be removed.
   * @param value The new value for deletionAllowed.
   */
  void setDeletionAllowed(const SFBool &value) { _deletionAllowed = value; }

  /**
   * @brief Gets the value of enteredText. AccessType: outputOnly
   * @details Events generated as character-producing keys are pressed on
   * keyboard.
   * @return SFString The current value of enteredText.
   */
  SFString getEnteredText() const { return _enteredText; }

  /**
   * @brief Emit an output value on enteredText. AccessType: outputOnly
   * @details Events generated as character-producing keys are pressed on
   * keyboard. outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitEnteredText(const SFString &value) { _enteredText = value; }

  /**
   * @brief Gets the value of finalText. AccessType: outputOnly
   * @details Events generated when sequence of keystrokes matches keys in
   * terminationText string when this condition occurs, enteredText is moved to
   * finalText and enteredText is set to empty string.
   * @return SFString The current value of finalText.
   */
  SFString getFinalText() const { return _finalText; }

  /**
   * @brief Emit an output value on finalText. AccessType: outputOnly
   * @details Events generated when sequence of keystrokes matches keys in
   * terminationText string when this condition occurs, enteredText is moved to
   * finalText and enteredText is set to empty string. outputOnly fields have no
   * author-facing setter; a node's behavior or the runtime calls this to
   * produce an output event. The event cascade reaches it through the reflected
   * field table so producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitFinalText(const SFString &value) { _finalText = value; }

  /**
   * @brief The X3D type name of this node (e.g. "StringSensor").
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
   * @brief Member variable for deletionAllowed.
   */

  SFBool _deletionAllowed{true};

  /**
   * @brief Member variable for enteredText.
   */

  SFString _enteredText{};

  /**
   * @brief Member variable for finalText.
   */

  SFString _finalText{};
};

} // namespace x3d::nodes
