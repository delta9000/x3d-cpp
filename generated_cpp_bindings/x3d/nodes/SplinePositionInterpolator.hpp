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
 * @class SplinePositionInterpolator
 * @brief SplinePositionInterpolator performs non-linear interpolation among
 * paired lists of 3-tuple values and velocities to produce an SFVec3f
 * value_changed output event.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/interpolators.html#SplinePositionInterpolator
 */
class SplinePositionInterpolator : public virtual X3DInterpolatorNode {
public:
  SplinePositionInterpolator() = default;

  ~SplinePositionInterpolator() = default;

  static SFBool getDefaultClosed() { return false; }

  static SFBool getDefaultNormalizeVelocity() { return false; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "Interpolation"; }

  static int componentLevel() { return 4; }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details Whether or not the curve is closed (i.
   */
  const SFBool &getClosed() const { return _closed; }

  /**
   * Access: inputOutput.
   * @details Whether or not the curve is closed (i.
   */
  void setClosed(const SFBool &value) { _closed = value; }

  /**
   * MFVec3f; access: inputOutput. Borrows the stored value.
   * @details Output values for nonlinear interpolation, each corresponding to
   * an input-fraction value in the key array.
   */
  const MFVec3f &getKeyValue() const { return _keyValue; }

  /**
   * Access: inputOutput.
   * @details Output values for nonlinear interpolation, each corresponding to
   * an input-fraction value in the key array.
   */
  void setKeyValue(const MFVec3f &value) { _keyValue = value; }

  void setKeyValue(MFVec3f &&value) { _keyValue = std::move(value); }

  /**
   * MFVec3f; access: inputOutput. Borrows the stored value.
   * @details Output values for nonlinear interpolation, each corresponding to
   * an input-fraction value in the key array.
   */
  const MFVec3f &getKeyVelocity() const { return _keyVelocity; }

  /**
   * Access: inputOutput.
   * @details Output values for nonlinear interpolation, each corresponding to
   * an input-fraction value in the key array.
   */
  void setKeyVelocity(const MFVec3f &value) { _keyVelocity = value; }

  void setKeyVelocity(MFVec3f &&value) { _keyVelocity = std::move(value); }

  /**
   * SFBool; access: inputOutput. Borrows the stored value.
   * @details normalizeVelocity field specifies whether the velocity vectors are
   * normalized to produce smooth speed transitions, or transformed into
   * tangency vectors.
   */
  const SFBool &getNormalizeVelocity() const { return _normalizeVelocity; }

  /**
   * Access: inputOutput.
   * @details normalizeVelocity field specifies whether the velocity vectors are
   * normalized to produce smooth speed transitions, or transformed into
   * tangency vectors.
   */
  void setNormalizeVelocity(const SFBool &value) { _normalizeVelocity = value; }

  /**
   * SFVec3f; access: outputOnly. Borrows the stored value.
   * @details Nonlinearly interpolated output value computed by using current
   * time fraction along with corresponding key, keyValue and keyVelocity
   * values.
   */
  const SFVec3f &getValue_changed() const { return _value_changed; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Nonlinearly interpolated output value computed by using current
   * time fraction along with corresponding key, keyValue and keyVelocity
   * values.
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
  SFBool _closed{false};

  MFVec3f _keyValue{};

  MFVec3f _keyVelocity{};

  SFBool _normalizeVelocity{false};

  SFVec3f _value_changed{};
};

} // namespace x3d::nodes
