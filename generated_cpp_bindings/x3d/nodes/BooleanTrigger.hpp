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
 * @class BooleanTrigger
 * @brief BooleanTrigger converts time events to boolean true events.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/eventUtilities.html#BooleanTrigger
 */
class BooleanTrigger : public virtual X3DTriggerNode {
public:
  BooleanTrigger() = default;

  ~BooleanTrigger() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "EventUtilities"; }

  static int componentLevel() { return 1; }

  /**
   * SFTime; access: inputOnly.
   * @details set_triggerTime provides input time event, typical event sent is
   * TouchSensor touchTime. Dispatches to the handler registered via
   * setOnSet_triggerTimeHandler(); a no-op if none is set.
   */
  void onSet_triggerTime(const SFTime &value) {
    if (_onSet_triggerTimeHandler)
      _onSet_triggerTimeHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void
  setOnSet_triggerTimeHandler(std::function<void(const SFTime &)> handler) {
    _onSet_triggerTimeHandler = std::move(handler);
  }

  /**
   * SFBool; access: outputOnly. Borrows the stored value.
   * @details triggerTrue outputs a true value whenever a triggerTime event is
   * received.
   */
  const SFBool &getTriggerTrue() const { return _triggerTrue; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details triggerTrue outputs a true value whenever a triggerTime event is
   * received.
   */
  void emitTriggerTrue(const SFBool &value) { _triggerTrue = value; }

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
  std::function<void(const SFTime &)> _onSet_triggerTimeHandler{};

  SFBool _triggerTrue{};
};

} // namespace x3d::nodes
