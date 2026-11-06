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
 * @class BooleanToggle
 * @brief BooleanToggle maintains state and negates output when a true input is
 * provided.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/eventUtilities.html#BooleanToggle
 */
class BooleanToggle : public virtual X3DChildNode {
public:
  BooleanToggle() = default;

  ~BooleanToggle() = default;

  static SFBool getDefaultToggle() { return false; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "EventUtilities"; }

  static int componentLevel() { return 1; }

  /**
   * SFBool; access: inputOnly.
   * @details If input event set_boolean is true, flip state by negating current
   * value of the toggle field Hint: for logical consistency, input event
   * set_boolean false has no effect (under review as part of Mantis issue 519).
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
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details Persistent state value that gets toggled or reset.
   */
  const SFBool &getToggle() const { return _toggle; }

  /**
   * Access: inputOutput.
   * @details Persistent state value that gets toggled or reset.
   */
  void setToggle(const SFBool &value) { _toggle = value; }

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

  SFBool _toggle{false};
};

} // namespace x3d::nodes
