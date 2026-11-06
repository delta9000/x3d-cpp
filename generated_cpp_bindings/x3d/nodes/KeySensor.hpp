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
 * @class KeySensor
 * @brief KeySensor generates events as the user presses keys on the keyboard.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/keyDeviceSensor.html#KeySensor
 */
class KeySensor : public virtual X3DKeyDeviceSensorNode {
public:
  KeySensor() = default;

  ~KeySensor() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "KeyDeviceSensor"; }

  static int componentLevel() { return 1; }

  /**
   * SFInt32; access: outputOnly. Borrows the stored value.
   * @details action key press gives following values: HOME=000 END=1001
   * PGUP=1002 PGDN=1003 UP=1004 DOWN=1005 LEFT=1006 RIGHT=1007 F1.
   */
  const SFInt32 &getActionKeyPress() const { return _actionKeyPress; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details action key press gives following values: HOME=000 END=1001
   * PGUP=1002 PGDN=1003 UP=1004 DOWN=1005 LEFT=1006 RIGHT=1007 F1.
   */
  void emitActionKeyPress(const SFInt32 &value) { _actionKeyPress = value; }

  /**
   * SFInt32; access: outputOnly. Borrows the stored value.
   * @details action key release gives following values: HOME=000 END=1001
   * PGUP=1002 PGDN=1003 UP=1004 DOWN=1005 LEFT=1006 RIGHT=1007 F1.
   */
  const SFInt32 &getActionKeyRelease() const { return _actionKeyRelease; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details action key release gives following values: HOME=000 END=1001
   * PGUP=1002 PGDN=1003 UP=1004 DOWN=1005 LEFT=1006 RIGHT=1007 F1.
   */
  void emitActionKeyRelease(const SFInt32 &value) { _actionKeyRelease = value; }

  /**
   * SFBool; access: outputOnly. Borrows the stored value.
   * @details altKey generates true event when pressed, false event when
   * released.
   */
  const SFBool &getAltKey() const { return _altKey; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details altKey generates true event when pressed, false event when
   * released.
   */
  void emitAltKey(const SFBool &value) { _altKey = value; }

  /**
   * SFBool; access: outputOnly. Borrows the stored value.
   * @details controlKey generates true event when pressed, false event when
   * released.
   */
  const SFBool &getControlKey() const { return _controlKey; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details controlKey generates true event when pressed, false event when
   * released.
   */
  void emitControlKey(const SFBool &value) { _controlKey = value; }

  /**
   * SFString; access: outputOnly. Borrows the stored value.
   * @details Events generated when user presses character-producing keys on
   * keyboard produces integer UTF-8 character values.
   */
  const SFString &getKeyPress() const { return _keyPress; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Events generated when user presses character-producing keys on
   * keyboard produces integer UTF-8 character values.
   */
  void emitKeyPress(const SFString &value) { _keyPress = value; }

  /**
   * SFString; access: outputOnly. Borrows the stored value.
   * @details Events generated when user releases character-producing keys on
   * keyboard produces integer UTF-8 character values.
   */
  const SFString &getKeyRelease() const { return _keyRelease; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Events generated when user releases character-producing keys on
   * keyboard produces integer UTF-8 character values.
   */
  void emitKeyRelease(const SFString &value) { _keyRelease = value; }

  /**
   * SFBool; access: outputOnly. Borrows the stored value.
   * @details shiftKey generates true event when pressed, false event when
   * released.
   */
  const SFBool &getShiftKey() const { return _shiftKey; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details shiftKey generates true event when pressed, false event when
   * released.
   */
  void emitShiftKey(const SFBool &value) { _shiftKey = value; }

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
  SFInt32 _actionKeyPress{};

  SFInt32 _actionKeyRelease{};

  SFBool _altKey{};

  SFBool _controlKey{};

  SFString _keyPress{};

  SFString _keyRelease{};

  SFBool _shiftKey{};
};

} // namespace x3d::nodes
