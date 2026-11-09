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
 * @class CoordinateInterpolator2D
 * @brief CoordinateInterpolator2D generates a series of SFVec2f or MFVec2f
 * 2-tuple float values.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/interpolators.html#CoordinateInterpolator2D
 */
class CoordinateInterpolator2D : public virtual X3DInterpolatorNode {
public:
  CoordinateInterpolator2D() = default;

  ~CoordinateInterpolator2D() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Interpolation"; }

  static int componentLevel() { return 3; }

  /**
   * MFVec2f; access: inputOutput. Borrows the stored value.
   * @details Output values for linear interpolation, each corresponding to an
   * input-fraction value in the key array.
   */
  const MFVec2f &getKeyValue() const { return _keyValue; }

  /**
   * Access: inputOutput.
   * @details Output values for linear interpolation, each corresponding to an
   * input-fraction value in the key array.
   */
  void setKeyValue(const MFVec2f &value) { _keyValue = value; }

  void setKeyValue(MFVec2f &&value) { _keyValue = std::move(value); }

  /**
   * MFVec2f; access: outputOnly. Borrows the stored value.
   * @details Linearly interpolated output value determined by current key time
   * and corresponding keyValue pair.
   */
  const MFVec2f &getValue_changed() const { return _value_changed; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Linearly interpolated output value determined by current key time
   * and corresponding keyValue pair.
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

private:
  MFVec2f _keyValue{};

  MFVec2f _value_changed{};
};

} // namespace x3d::nodes
