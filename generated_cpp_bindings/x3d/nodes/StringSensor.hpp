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
  StringSensor() = default;

  ~StringSensor() = default;

  static SFBool getDefaultDeletionAllowed() { return true; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "KeyDeviceSensor"; }

  static int componentLevel() { return 2; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details If deletionAllowed is true, then previously entered character in
   * enteredText can be removed.
   */
  const SFBool &getDeletionAllowed() const { return _deletionAllowed; }

  /**
   * Access: inputOutput.
   * @details If deletionAllowed is true, then previously entered character in
   * enteredText can be removed.
   */
  void setDeletionAllowed(const SFBool &value) { _deletionAllowed = value; }

  /**
   * SFString; access: outputOnly. Borrows the stored value.
   * @details Events generated as character-producing keys are pressed on
   * keyboard.
   */
  const SFString &getEnteredText() const { return _enteredText; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Events generated as character-producing keys are pressed on
   * keyboard.
   */
  void emitEnteredText(const SFString &value) { _enteredText = value; }

  /**
   * SFString; access: outputOnly. Borrows the stored value.
   * @details Events generated when sequence of keystrokes matches keys in
   * terminationText string when this condition occurs, enteredText is moved to
   * finalText and enteredText is set to empty string.
   */
  const SFString &getFinalText() const { return _finalText; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Events generated when sequence of keystrokes matches keys in
   * terminationText string when this condition occurs, enteredText is moved to
   * finalText and enteredText is set to empty string.
   */
  void emitFinalText(const SFString &value) { _finalText = value; }

  std::string nodeTypeName() const override;

  /**
   * Parent field used when an X3D-XML element omits containerField.
   */
  std::string defaultContainerField() const override;

  /**
   * Own and inherited fields; the table lives for the lifetime of the program.
   */
  const FieldTable &fields() const override;

  void accept(NodeVisitor &visitor) const override;

private:
  SFBool _deletionAllowed{true};

  SFString _enteredText{};

  SFString _finalText{};
};

} // namespace x3d::nodes
