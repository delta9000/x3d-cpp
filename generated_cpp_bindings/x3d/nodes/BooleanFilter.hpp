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
 * @class BooleanFilter
 * @brief BooleanFilter selectively passes true, false or negated events.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/eventUtilities.html#BooleanFilter
 */
class BooleanFilter : public virtual X3DChildNode {
public:
  BooleanFilter() = default;

  ~BooleanFilter() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "EventUtilities"; }

  static int componentLevel() { return 1; }

  /**
   * SFBool; access: outputOnly. Borrows the stored value.
   * @details inputFalse only passes a false value, which occurs when
   * set_boolean is false.
   */
  const SFBool &getInputFalse() const { return _inputFalse; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details inputFalse only passes a false value, which occurs when
   * set_boolean is false.
   */
  void emitInputFalse(const SFBool &value) { _inputFalse = value; }

  /**
   * SFBool; access: outputOnly. Borrows the stored value.
   * @details inputNegate is an output event that provides an opposite value by
   * negating set_boolean input.
   */
  const SFBool &getInputNegate() const { return _inputNegate; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details inputNegate is an output event that provides an opposite value by
   * negating set_boolean input.
   */
  void emitInputNegate(const SFBool &value) { _inputNegate = value; }

  /**
   * SFBool; access: outputOnly. Borrows the stored value.
   * @details inputTrue only passes a true value, which occurs when set_boolean
   * input is true.
   */
  const SFBool &getInputTrue() const { return _inputTrue; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details inputTrue only passes a true value, which occurs when set_boolean
   * input is true.
   */
  void emitInputTrue(const SFBool &value) { _inputTrue = value; }

  /**
   * SFBool; access: inputOnly.
   * @details set_boolean is the input value to be filtered.
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
  SFBool _inputFalse{};

  SFBool _inputNegate{};

  SFBool _inputTrue{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const SFBool &)> _onSet_booleanHandler{};
};

} // namespace x3d::nodes
