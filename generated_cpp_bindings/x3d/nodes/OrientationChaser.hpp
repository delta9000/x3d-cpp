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
 * @class OrientationChaser
 * @brief OrientationChaser generates a series of 4-tuple axis-angle SFRotation
 * values that progressively change from initial value to destination value.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/followers.html#OrientationChaser
 */
class OrientationChaser : public virtual X3DChaserNode {
public:
  OrientationChaser() = default;

  ~OrientationChaser() = default;

  static SFRotation getDefaultInitialDestination() {
    return SFRotation{0, 1, 0, 0};
  }

  static SFRotation getDefaultInitialValue() { return SFRotation{0, 1, 0, 0}; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Followers"; }

  static int componentLevel() { return 1; }

  /**
   * SFRotation; access: initializeOnly. Borrows the stored value.
   * @details Initial destination value for this node.
   */
  const SFRotation &getInitialDestination() const {
    return _initialDestination;
  }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setInitialDestinationUnchecked(const SFRotation &value) {
    _initialDestination = value;
  }

  /**
   * SFRotation; access: initializeOnly. Borrows the stored value.
   * @details Initial starting value for this node.
   */
  const SFRotation &getInitialValue() const { return _initialValue; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setInitialValueUnchecked(const SFRotation &value) {
    _initialValue = value;
  }

  /**
   * SFRotation; access: inputOnly.
   * @details set_destination resets destination value of this node.
   *          Dispatches to the handler registered via
   * setOnSet_destinationHandler(); a no-op if none is set.
   */
  void onSet_destination(const SFRotation &value) {
    if (_onSet_destinationHandler)
      _onSet_destinationHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void
  setOnSet_destinationHandler(std::function<void(const SFRotation &)> handler) {
    _onSet_destinationHandler = std::move(handler);
  }

  /**
   * SFRotation; access: inputOnly.
   * @details set_value resets current value of this node.
   *          Dispatches to the handler registered via setOnSet_valueHandler();
   *          a no-op if none is set.
   */
  void onSet_value(const SFRotation &value) {
    if (_onSet_valueHandler)
      _onSet_valueHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnSet_valueHandler(std::function<void(const SFRotation &)> handler) {
    _onSet_valueHandler = std::move(handler);
  }

  /**
   * SFRotation; access: outputOnly. Borrows the stored value.
   * @details Computed output value that approaches within tolerance of
   * destination value, as determined by elapsed time, order and tau.
   */
  const SFRotation &getValue_changed() const { return _value_changed; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Computed output value that approaches within tolerance of
   * destination value, as determined by elapsed time, order and tau.
   */
  void emitValue_changed(const SFRotation &value) { _value_changed = value; }

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
  SFRotation _initialDestination{SFRotation{0, 1, 0, 0}};

  SFRotation _initialValue{SFRotation{0, 1, 0, 0}};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const SFRotation &)> _onSet_destinationHandler{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const SFRotation &)> _onSet_valueHandler{};

  SFRotation _value_changed{};
};

} // namespace x3d::nodes
