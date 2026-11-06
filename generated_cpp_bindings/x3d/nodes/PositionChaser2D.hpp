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

#include "x3d/nodes/X3DFollowerNode.hpp"

#include "x3d/nodes/X3DChaserNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class PositionChaser2D
 * @brief PositionChaser2D generates a series of 2D position values that
 * progressively change from initial value to destination value.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/followers.html#PositionChaser2D
 */
class PositionChaser2D : public virtual X3DChaserNode {
public:
  PositionChaser2D() = default;

  ~PositionChaser2D() = default;

  static SFVec2f getDefaultInitialDestination() { return SFVec2f{0, 0}; }

  static SFVec2f getDefaultInitialValue() { return SFVec2f{0, 0}; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Followers"; }

  static int componentLevel() { return 1; }

  /**
   * SFVec2f; access: initializeOnly. Borrows the stored value.
   * @details Initial destination value for this node.
   */
  const SFVec2f &getInitialDestination() const { return _initialDestination; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setInitialDestinationUnchecked(const SFVec2f &value) {
    _initialDestination = value;
  }

  /**
   * SFVec2f; access: initializeOnly. Borrows the stored value.
   * @details Initial starting value for this node.
   */
  const SFVec2f &getInitialValue() const { return _initialValue; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setInitialValueUnchecked(const SFVec2f &value) { _initialValue = value; }

  /**
   * SFVec2f; access: inputOnly.
   * @details set_destination resets destination value of this node.
   *          Dispatches to the handler registered via
   * setOnSet_destinationHandler(); a no-op if none is set.
   */
  void onSet_destination(const SFVec2f &value) {
    if (_onSet_destinationHandler)
      _onSet_destinationHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void
  setOnSet_destinationHandler(std::function<void(const SFVec2f &)> handler) {
    _onSet_destinationHandler = std::move(handler);
  }

  /**
   * SFVec2f; access: inputOnly.
   * @details set_value resets current value of this node.
   *          Dispatches to the handler registered via setOnSet_valueHandler();
   *          a no-op if none is set.
   */
  void onSet_value(const SFVec2f &value) {
    if (_onSet_valueHandler)
      _onSet_valueHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnSet_valueHandler(std::function<void(const SFVec2f &)> handler) {
    _onSet_valueHandler = std::move(handler);
  }

  /**
   * SFVec2f; access: outputOnly. Borrows the stored value.
   * @details Computed output value that approaches within tolerance of
   * destination value, as determined by elapsed time, order and tau.
   */
  const SFVec2f &getValue_changed() const { return _value_changed; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Computed output value that approaches within tolerance of
   * destination value, as determined by elapsed time, order and tau.
   */
  void emitValue_changed(const SFVec2f &value) { _value_changed = value; }

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

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

private:
  SFVec2f _initialDestination{SFVec2f{0, 0}};

  SFVec2f _initialValue{SFVec2f{0, 0}};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const SFVec2f &)> _onSet_destinationHandler{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const SFVec2f &)> _onSet_valueHandler{};

  SFVec2f _value_changed{};
};

} // namespace x3d::nodes
