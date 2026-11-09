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
 * @class SplineScalarInterpolator
 * @brief SplineScalarInterpolator performs non-linear interpolation among
 * paired lists of float values and velocities to produce an SFFloat
 * value_changed output event.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/interpolators.html#SplineScalarInterpolator
 */
class SplineScalarInterpolator : public virtual X3DInterpolatorNode {
public:
  SplineScalarInterpolator() = default;

  ~SplineScalarInterpolator() = default;

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
   * MFFloat; access: inputOutput. Borrows the stored value.
   * @details Output values for nonlinear interpolation, each corresponding to
   * an input-fraction value in the key array.
   */
  const MFFloat &getKeyValue() const { return _keyValue; }

  /**
   * Access: inputOutput.
   * @details Output values for nonlinear interpolation, each corresponding to
   * an input-fraction value in the key array.
   */
  void setKeyValue(const MFFloat &value) { _keyValue = value; }

  void setKeyValue(MFFloat &&value) { _keyValue = std::move(value); }

  /**
   * MFFloat; access: inputOutput. Borrows the stored value.
   * @details Output values for nonlinear interpolation, each corresponding to
   * an input-fraction value in the key array.
   */
  const MFFloat &getKeyVelocity() const { return _keyVelocity; }

  /**
   * Access: inputOutput.
   * @details Output values for nonlinear interpolation, each corresponding to
   * an input-fraction value in the key array.
   */
  void setKeyVelocity(const MFFloat &value) { _keyVelocity = value; }

  void setKeyVelocity(MFFloat &&value) { _keyVelocity = std::move(value); }

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
   * SFFloat; access: outputOnly. Borrows the stored value.
   * @details Nonlinearly interpolated output value computed by using current
   * time fraction along with corresponding key, keyValue and keyVelocity
   * values.
   */
  const SFFloat &getValue_changed() const { return _value_changed; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Nonlinearly interpolated output value computed by using current
   * time fraction along with corresponding key, keyValue and keyVelocity
   * values.
   */
  void emitValue_changed(const SFFloat &value) { _value_changed = value; }

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

  MFFloat _keyValue{};

  MFFloat _keyVelocity{};

  SFBool _normalizeVelocity{false};

  SFFloat _value_changed{};
};

} // namespace x3d::nodes
