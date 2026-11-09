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
 * @class CylinderSensor
 * @brief CylinderSensor converts pointer motion (for example, a mouse or wand)
 * into rotation values using an invisible cylinder aligned with local Y-axis.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/pointingDeviceSensor.html#CylinderSensor
 */
class CylinderSensor : public virtual X3DDragSensorNode {
public:
  CylinderSensor() = default;

  ~CylinderSensor() = default;

  static SFRotation getDefaultAxisRotation() { return SFRotation{0, 0, 1, 0}; }

  static SFFloat getDefaultDiskAngle() { return 0.26179167; }

  static SFFloat getDefaultMaxAngle() { return -1; }

  static SFFloat getDefaultMinAngle() { return 0; }

  static SFFloat getDefaultOffset() { return 0; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "PointingDeviceSensor"; }

  static int componentLevel() { return 1; }

  /**
   * SFRotation; access: inputOutput. Borrows the stored value.
   * @details axisRotation determines local sensor coordinate system by rotating
   * the local coordinate system.
   */
  const SFRotation &getAxisRotation() const { return _axisRotation; }

  /**
   * Access: inputOutput.
   * @details axisRotation determines local sensor coordinate system by rotating
   * the local coordinate system.
   */
  void setAxisRotation(const SFRotation &value) { _axisRotation = value; }

  void setAxisRotation(SFRotation &&value) { _axisRotation = std::move(value); }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details Help decide rotation behavior from initial relative bearing of
   * pointer drag: acute angle whether cylinder sides or end-cap disks of
   * virtual-geometry sensor are used for manipulation.
   */
  const SFFloat &getDiskAngle() const { return _diskAngle; }

  /**
   * Access: inputOutput.
   * @details Help decide rotation behavior from initial relative bearing of
   * pointer drag: acute angle whether cylinder sides or end-cap disks of
   * virtual-geometry sensor are used for manipulation.
   */
  void setDiskAngle(const SFFloat &value) {

    validateDiskAngle(value);

    _diskAngle = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setDiskAngle() for range validation.
   */
  void setDiskAngleUnchecked(const SFFloat &value) { _diskAngle = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details clamps rotation_changed events within range of min/max values
   * Hint: if minAngle > maxAngle, rotation is not clamped.
   */
  const SFFloat &getMaxAngle() const { return _maxAngle; }

  /**
   * Access: inputOutput.
   * @details clamps rotation_changed events within range of min/max values
   * Hint: if minAngle > maxAngle, rotation is not clamped.
   */
  void setMaxAngle(const SFFloat &value) { _maxAngle = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details clamps rotation_changed events within range of min/max values
   * Hint: if minAngle > maxAngle, rotation is not clamped.
   */
  const SFFloat &getMinAngle() const { return _minAngle; }

  /**
   * Access: inputOutput.
   * @details clamps rotation_changed events within range of min/max values
   * Hint: if minAngle > maxAngle, rotation is not clamped.
   */
  void setMinAngle(const SFFloat &value) { _minAngle = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details Sends event and remembers last value sensed.
   */
  const SFFloat &getOffset() const { return _offset; }

  /**
   * Access: inputOutput.
   * @details Sends event and remembers last value sensed.
   */
  void setOffset(const SFFloat &value) { _offset = value; }

  /**
   * SFRotation; access: outputOnly. Borrows the stored value.
   * @details rotation_changed events equal sum of relative bearing changes plus
   * offset value about Y-axis in local coordinate system.
   */
  const SFRotation &getRotation_changed() const { return _rotation_changed; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details rotation_changed events equal sum of relative bearing changes plus
   * offset value about Y-axis in local coordinate system.
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

  void validateRanges(std::vector<RangeDiagnostic> &out) const override;

protected:
  /**
   * @brief Non-throwing range check: appends a RangeDiagnostic per out-of-range
   *        component. Used by validateRanges() to surface lenient-read values.
   */
  static void checkRangesDiskAngle(const SFFloat &value,
                                   const std::string &nodeType,
                                   const std::string &defName,
                                   std::vector<RangeDiagnostic> &out);

private:
  static void validateDiskAngle(const SFFloat &value) {

    if (value < 0.0f)
      throw std::out_of_range("diskAngle below minimum of 0");
    if (value > 1.5708f)
      throw std::out_of_range("diskAngle above maximum of 1.5708");
  }

  SFRotation _axisRotation{SFRotation{0, 0, 1, 0}};

  SFFloat _diskAngle{0.26179167};

  SFFloat _maxAngle{-1};

  SFFloat _minAngle{0};

  SFFloat _offset{0};

  SFRotation _rotation_changed{};
};

} // namespace x3d::nodes
