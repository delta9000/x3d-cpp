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
 * @class EaseInEaseOut
 * @brief EaseInEaseOut enables gradual animation transitions by modifying
 * TimeSensor fraction outputs.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/interpolators.html#EaseInEaseOut
 */
class EaseInEaseOut : public virtual X3DChildNode {
public:
  EaseInEaseOut() = default;

  ~EaseInEaseOut() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Interpolation"; }

  static int componentLevel() { return 4; }

  /**
   * MFVec2f; access: inputOutput. Borrows the stored value.
   * @details Array of paired values for easeOut fraction and easeIn fraction
   * within each key interval.
   */
  const MFVec2f &getEaseInEaseOut() const { return _easeInEaseOut; }

  /**
   * Access: inputOutput.
   * @details Array of paired values for easeOut fraction and easeIn fraction
   * within each key interval.
   */
  void setEaseInEaseOut(const MFVec2f &value) { _easeInEaseOut = value; }

  void setEaseInEaseOut(MFVec2f &&value) { _easeInEaseOut = std::move(value); }

  /**
   * MFFloat; access: inputOutput. Borrows the stored value.
   * @details Definition values for linear-interpolation function input
   * intervals, listed in non-decreasing order and corresponding to
   * easeInEaseOut array.
   */
  const MFFloat &getKey() const { return _key; }

  /**
   * Access: inputOutput.
   * @details Definition values for linear-interpolation function input
   * intervals, listed in non-decreasing order and corresponding to
   * easeInEaseOut array.
   */
  void setKey(const MFFloat &value) { _key = value; }

  void setKey(MFFloat &&value) { _key = std::move(value); }

  /**
   * SFFloat; access: outputOnly. Borrows the stored value.
   * @details Interpolated output value determined by current key time,
   * corresponding easeInEaseOut smoothing intervals, and corresponding key
   * pair.
   */
  const SFFloat &getModifiedFraction_changed() const {
    return _modifiedFraction_changed;
  }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Interpolated output value determined by current key time,
   * corresponding easeInEaseOut smoothing intervals, and corresponding key
   * pair.
   */
  void emitModifiedFraction_changed(const SFFloat &value) {
    _modifiedFraction_changed = value;
  }

  /**
   * SFFloat; access: inputOnly.
   * @details set_fraction selects input fraction for computation of
   * corresponding easeInEaseOut output value, modifiedFraction_changed.
   *          Dispatches to the handler registered via
   * setOnSet_fractionHandler(); a no-op if none is set.
   */
  void onSet_fraction(const SFFloat &value) {
    if (_onSet_fractionHandler)
      _onSet_fractionHandler(value);
  }

  /**
   * Replaces the prior event callback.
   */
  void setOnSet_fractionHandler(std::function<void(const SFFloat &)> handler) {
    _onSet_fractionHandler = std::move(handler);
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
  MFVec2f _easeInEaseOut{};

  MFFloat _key{};

  SFFloat _modifiedFraction_changed{};

  /**
   * Empty callbacks leave input events unhandled.
   */
  std::function<void(const SFFloat &)> _onSet_fractionHandler{};
};

} // namespace x3d::nodes
