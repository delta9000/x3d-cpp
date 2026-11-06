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

#include "x3d/nodes/X3DSensorNode.hpp"

#include "x3d/nodes/X3DPointingDeviceSensorNode.hpp"

#include "x3d/nodes/X3DDragSensorNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class SphereSensor
 * @brief SphereSensor converts pointing device motion into a spherical rotation
 * about the origin of the local coordinate system.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/pointingDeviceSensor.html#SphereSensor
 */
class SphereSensor : public virtual X3DDragSensorNode {
public:
  SphereSensor() = default;

  ~SphereSensor() = default;

  static SFRotation getDefaultOffset() { return SFRotation{0, 1, 0, 0}; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "PointingDeviceSensor"; }

  static int componentLevel() { return 1; }

  /**
   * SFRotation; access: inputOutput. Borrows the stored value.
   * @details Sends event and remembers last value sensed.
   */
  const SFRotation &getOffset() const { return _offset; }

  /**
   * Access: inputOutput.
   * @details Sends event and remembers last value sensed.
   */
  void setOffset(const SFRotation &value) { _offset = value; }

  void setOffset(SFRotation &&value) { _offset = std::move(value); }

  /**
   * SFRotation; access: outputOnly. Borrows the stored value.
   * @details rotation_changed events equal sum of relative bearing changes plus
   * offset value.
   */
  const SFRotation &getRotation_changed() const { return _rotation_changed; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details rotation_changed events equal sum of relative bearing changes plus
   * offset value.
   */
  void emitRotation_changed(const SFRotation &value) {
    _rotation_changed = value;
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
  SFRotation _offset{SFRotation{0, 1, 0, 0}};

  SFRotation _rotation_changed{};
};

} // namespace x3d::nodes
