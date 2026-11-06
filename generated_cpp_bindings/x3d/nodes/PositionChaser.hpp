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
 * @class PositionChaser
 * @brief PositionChaser generates a series of position values that
 * progressively change from initial value to destination value.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/followers.html#PositionChaser
 */
class PositionChaser : public virtual X3DChaserNode {
public:
  PositionChaser() = default;

  ~PositionChaser() = default;

  static SFVec3f getDefaultInitialDestination() { return SFVec3f{0, 0, 0}; }

  static SFVec3f getDefaultInitialValue() { return SFVec3f{0, 0, 0}; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Followers"; }

  static int componentLevel() { return 1; }

  /**
   * SFVec3f; access: initializeOnly. Borrows the stored value.
   * @details Initial destination value for this node.
   */
  const SFVec3f &getInitialDestination() const { return _initialDestination; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setInitialDestinationUnchecked(const SFVec3f &value) {
    _initialDestination = value;
  }

  /**
   * SFVec3f; access: initializeOnly. Borrows the stored value.
   * @details Initial starting value for this node.
   */
  const SFVec3f &getInitialValue() const { return _initialValue; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setInitialValueUnchecked(const SFVec3f &value) { _initialValue = value; }

  /**
   * SFVec3f; access: inputOnly.
   * @details set_destination resets destination value of this node.
   *          Dispatches to the handler registered via
   * setOnSet_destinationHandler(); a no-op if none is set.
   */
  void onSet_destination(const SFVec3f &value) {
    if (_onSet_destinationHandler)
      _onSet_destinationHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void
  setOnSet_destinationHandler(std::function<void(const SFVec3f &)> handler) {
    _onSet_destinationHandler = std::move(handler);
  }

  /**
   * SFVec3f; access: inputOnly.
   * @details set_value resets current value of this node.
   *          Dispatches to the handler registered via setOnSet_valueHandler();
   *          a no-op if none is set.
   */
  void onSet_value(const SFVec3f &value) {
    if (_onSet_valueHandler)
      _onSet_valueHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnSet_valueHandler(std::function<void(const SFVec3f &)> handler) {
    _onSet_valueHandler = std::move(handler);
  }

  /**
   * SFVec3f; access: outputOnly. Borrows the stored value.
   * @details Computed output value that approaches within tolerance of
   * destination value, as determined by elapsed time, order and tau.
   */
  const SFVec3f &getValue_changed() const { return _value_changed; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Computed output value that approaches within tolerance of
   * destination value, as determined by elapsed time, order and tau.
   */
  void emitValue_changed(const SFVec3f &value) { _value_changed = value; }

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
  SFVec3f _initialDestination{SFVec3f{0, 0, 0}};

  SFVec3f _initialValue{SFVec3f{0, 0, 0}};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const SFVec3f &)> _onSet_destinationHandler{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const SFVec3f &)> _onSet_valueHandler{};

  SFVec3f _value_changed{};
};

} // namespace x3d::nodes
