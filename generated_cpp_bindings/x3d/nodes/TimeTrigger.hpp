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
 * @class TimeTrigger
 * @brief TimeTrigger converts boolean true events to time events.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/eventUtilities.html#TimeTrigger
 */
class TimeTrigger : public virtual X3DTriggerNode {
public:
  TimeTrigger() = default;

  ~TimeTrigger() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "EventUtilities"; }

  static int componentLevel() { return 1; }

  /**
   * SFBool; access: inputOnly.
   * @details If input event set_boolean is true, send output triggerTime event.
   *          Dispatches to the handler registered via
   * setOnSet_booleanHandler(); a no-op if none is set.
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
   * SFTime; access: outputOnly. Borrows the stored value.
   * @details triggerTime is output time event, sent when input event
   * set_boolean is true.
   */
  const SFTime &getTriggerTime() const { return _triggerTime; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details triggerTime is output time event, sent when input event
   * set_boolean is true.
   */
  void emitTriggerTime(const SFTime &value) { _triggerTime = value; }

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
  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const SFBool &)> _onSet_booleanHandler{};

  SFTime _triggerTime{};
};

} // namespace x3d::nodes
