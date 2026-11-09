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

#include "x3d/nodes/X3DInterpolatorNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class PositionInterpolator
 * @brief PositionInterpolator generates a series of 3-tuple SFVec3f values.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/interpolators.html#PositionInterpolator
 */
class PositionInterpolator : public virtual X3DInterpolatorNode {
public:
  PositionInterpolator() = default;

  ~PositionInterpolator() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Interpolation"; }

  static int componentLevel() { return 1; }

  /**
   * MFVec3f; access: inputOutput. Borrows the stored value.
   * @details Output values for linear interpolation, each corresponding to an
   * input-fraction value in the key array.
   */
  const MFVec3f &getKeyValue() const { return _keyValue; }

  /**
   * Access: inputOutput.
   * @details Output values for linear interpolation, each corresponding to an
   * input-fraction value in the key array.
   */
  void setKeyValue(const MFVec3f &value) { _keyValue = value; }

  void setKeyValue(MFVec3f &&value) { _keyValue = std::move(value); }

  /**
   * SFVec3f; access: outputOnly. Borrows the stored value.
   * @details Linearly interpolated output value determined by current key time
   * and corresponding keyValue pair.
   */
  const SFVec3f &getValue_changed() const { return _value_changed; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Linearly interpolated output value determined by current key time
   * and corresponding keyValue pair.
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

private:
  MFVec3f _keyValue{};

  SFVec3f _value_changed{};
};

} // namespace x3d::nodes
