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
 * @class IntegerTrigger
 * @brief IntegerTrigger converts set_boolean true input events to an integer
 * value (for example, useful when animating whichChoice in a Switch node).
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/eventUtilities.html#IntegerTrigger
 */
class IntegerTrigger : public virtual X3DTriggerNode {
public:
  IntegerTrigger() = default;

  ~IntegerTrigger() = default;

  static SFInt32 getDefaultIntegerKey() { return -1; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "EventUtilities"; }

  static int componentLevel() { return 1; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details integerKey is value for output when triggered.
   */
  const SFInt32 &getIntegerKey() const { return _integerKey; }

  /**
   * Access: inputOutput.
   * @details integerKey is value for output when triggered.
   */
  void setIntegerKey(const SFInt32 &value) { _integerKey = value; }

  /**
   * SFBool; access: inputOnly.
   * @details If input event set_boolean is true, trigger output of integer
   * value. Dispatches to the handler registered via setOnSet_booleanHandler();
   *          a no-op if none is set.
   */
  void onSet_boolean(const SFBool &value) {
    if (_onSet_booleanHandler)
      _onSet_booleanHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnSet_booleanHandler(std::function<void(const SFBool &)> handler) {
    _onSet_booleanHandler = std::move(handler);
  }

  /**
   * SFInt32; access: outputOnly. Borrows the stored value.
   * @details triggerValue provides integer event output matching integerKey
   * when true set_boolean received.
   */
  const SFInt32 &getTriggerValue() const { return _triggerValue; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details triggerValue provides integer event output matching integerKey
   * when true set_boolean received.
   */
  void emitTriggerValue(const SFInt32 &value) { _triggerValue = value; }

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
  SFInt32 _integerKey{-1};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const SFBool &)> _onSet_booleanHandler{};

  SFInt32 _triggerValue{};
};

} // namespace x3d::nodes
