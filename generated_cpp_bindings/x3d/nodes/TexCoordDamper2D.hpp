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

#include "x3d/nodes/X3DDamperNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class TexCoordDamper2D
 * @brief TexCoordDamper2D generates a series of 2D floating-point arrays that
 * progressively change from initial value to destination value.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/followers.html#TexCoordDamper2D
 */
class TexCoordDamper2D : public virtual X3DDamperNode {
public:
  TexCoordDamper2D() = default;

  ~TexCoordDamper2D() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Followers"; }

  static int componentLevel() { return 1; }

  /**
   * MFVec2f; access: initializeOnly. Borrows the stored value.
   * @details Initial destination value for this node.
   */
  const MFVec2f &getInitialDestination() const { return _initialDestination; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setInitialDestinationUnchecked(const MFVec2f &value) {
    _initialDestination = value;
  }

  /**
   * MFVec2f; access: initializeOnly. Borrows the stored value.
   * @details Initial starting value for this node.
   */
  const MFVec2f &getInitialValue() const { return _initialValue; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setInitialValueUnchecked(const MFVec2f &value) { _initialValue = value; }

  /**
   * MFVec2f; access: inputOnly.
   * @details set_destination resets destination value of this node.
   *          Dispatches to the handler registered via
   * setOnSet_destinationHandler(); a no-op if none is set.
   */
  void onSet_destination(const MFVec2f &value) {
    if (_onSet_destinationHandler)
      _onSet_destinationHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void
  setOnSet_destinationHandler(std::function<void(const MFVec2f &)> handler) {
    _onSet_destinationHandler = std::move(handler);
  }

  /**
   * MFVec2f; access: inputOnly.
   * @details set_value resets current value of this node.
   *          Dispatches to the handler registered via setOnSet_valueHandler();
   *          a no-op if none is set.
   */
  void onSet_value(const MFVec2f &value) {
    if (_onSet_valueHandler)
      _onSet_valueHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnSet_valueHandler(std::function<void(const MFVec2f &)> handler) {
    _onSet_valueHandler = std::move(handler);
  }

  /**
   * MFVec2f; access: outputOnly. Borrows the stored value.
   * @details Computed output value that approaches within tolerance of
   * destination value, as determined by elapsed time, order and tau.
   */
  const MFVec2f &getValue_changed() const { return _value_changed; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Computed output value that approaches within tolerance of
   * destination value, as determined by elapsed time, order and tau.
   */
  void emitValue_changed(const MFVec2f &value) { _value_changed = value; }

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
  MFVec2f _initialDestination{};

  MFVec2f _initialValue{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const MFVec2f &)> _onSet_destinationHandler{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const MFVec2f &)> _onSet_valueHandler{};

  MFVec2f _value_changed{};
};

} // namespace x3d::nodes
