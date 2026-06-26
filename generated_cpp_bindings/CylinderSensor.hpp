// CylinderSensor.hpp
#ifndef CYLINDERSENSOR_HPP
#define CYLINDERSENSOR_HPP

#include "X3DReflection.hpp"
#include "X3Denums.hpp"
#include "X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "X3DNode.hpp"

#include "X3DChildNode.hpp"

#include "X3DSensorNode.hpp"

#include "X3DPointingDeviceSensorNode.hpp"

#include "X3DDragSensorNode.hpp"

/**
 * @class CylinderSensor
 * @brief CylinderSensor converts pointer motion (for example, a mouse or wand)
 * into rotation values using an invisible cylinder aligned with local Y-axis.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/pointingDeviceSensor.html#CylinderSensor
 */
class CylinderSensor : public virtual X3DDragSensorNode {
public:
  /**
   * @brief Default constructor for CylinderSensor
   */
  CylinderSensor() = default;

  /**
   * @brief Destructor for CylinderSensor
   */
  ~CylinderSensor() = default;

  /**
   * @brief Get the default value for axisRotation
   * @return SFRotation The default value
   */
  static SFRotation getDefaultAxisRotation() { return SFRotation{0, 0, 1, 0}; }

  /**
   * @brief Get the default value for diskAngle
   * @return SFFloat The default value
   */
  static SFFloat getDefaultDiskAngle() { return 0.26179167; }

  /**
   * @brief Get the default value for maxAngle
   * @return SFFloat The default value
   */
  static SFFloat getDefaultMaxAngle() { return -1; }

  /**
   * @brief Get the default value for minAngle
   * @return SFFloat The default value
   */
  static SFFloat getDefaultMinAngle() { return 0; }

  /**
   * @brief Get the default value for offset
   * @return SFFloat The default value
   */
  static SFFloat getDefaultOffset() { return 0; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "children"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "PointingDeviceSensor"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 1; }

  /**
   * @brief Gets the value of axisRotation. AccessType: inputOutput
   * @details axisRotation determines local sensor coordinate system by rotating
   * the local coordinate system.
   * @return SFRotation The current value of axisRotation.
   */
  SFRotation getAxisRotation() const { return _axisRotation; }

  /**
   * @brief Sets the value of axisRotation. AccessType: inputOutput
   * @details axisRotation determines local sensor coordinate system by rotating
   * the local coordinate system.
   * @param value The new value for axisRotation.
   */
  void setAxisRotation(const SFRotation &value) { _axisRotation = value; }

  void setAxisRotation(SFRotation &&value) { _axisRotation = std::move(value); }

  /**
   * @brief Gets the value of diskAngle. AccessType: inputOutput
   * @details Help decide rotation behavior from initial relative bearing of
   * pointer drag: acute angle whether cylinder sides or end-cap disks of
   * virtual-geometry sensor are used for manipulation.
   * @return SFFloat The current value of diskAngle.
   */
  SFFloat getDiskAngle() const { return _diskAngle; }

  /**
   * @brief Sets the value of diskAngle. AccessType: inputOutput
   * @details Help decide rotation behavior from initial relative bearing of
   * pointer drag: acute angle whether cylinder sides or end-cap disks of
   * virtual-geometry sensor are used for manipulation.
   * @param value The new value for diskAngle.
   */
  void setDiskAngle(const SFFloat &value) {

    validateDiskAngle(value);

    _diskAngle = value;
  }

  /**
   * @brief Non-validating write of diskAngle (runtime/reader ingest path).
   * @details Assigns without the range check setDiskAngle() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setDiskAngle() stays the
   *          enforcement point for programmatic callers.
   */
  void setDiskAngleUnchecked(const SFFloat &value) { _diskAngle = value; }

  /**
   * @brief Gets the value of maxAngle. AccessType: inputOutput
   * @details clamps rotation_changed events within range of min/max values
   * Hint: if minAngle > maxAngle, rotation is not clamped.
   * @return SFFloat The current value of maxAngle.
   */
  SFFloat getMaxAngle() const { return _maxAngle; }

  /**
   * @brief Sets the value of maxAngle. AccessType: inputOutput
   * @details clamps rotation_changed events within range of min/max values
   * Hint: if minAngle > maxAngle, rotation is not clamped.
   * @param value The new value for maxAngle.
   */
  void setMaxAngle(const SFFloat &value) { _maxAngle = value; }

  /**
   * @brief Gets the value of minAngle. AccessType: inputOutput
   * @details clamps rotation_changed events within range of min/max values
   * Hint: if minAngle > maxAngle, rotation is not clamped.
   * @return SFFloat The current value of minAngle.
   */
  SFFloat getMinAngle() const { return _minAngle; }

  /**
   * @brief Sets the value of minAngle. AccessType: inputOutput
   * @details clamps rotation_changed events within range of min/max values
   * Hint: if minAngle > maxAngle, rotation is not clamped.
   * @param value The new value for minAngle.
   */
  void setMinAngle(const SFFloat &value) { _minAngle = value; }

  /**
   * @brief Gets the value of offset. AccessType: inputOutput
   * @details Sends event and remembers last value sensed.
   * @return SFFloat The current value of offset.
   */
  SFFloat getOffset() const { return _offset; }

  /**
   * @brief Sets the value of offset. AccessType: inputOutput
   * @details Sends event and remembers last value sensed.
   * @param value The new value for offset.
   */
  void setOffset(const SFFloat &value) { _offset = value; }

  /**
   * @brief Gets the value of rotation_changed. AccessType: outputOnly
   * @details rotation_changed events equal sum of relative bearing changes plus
   * offset value about Y-axis in local coordinate system.
   * @return SFRotation The current value of rotation_changed.
   */
  SFRotation getRotation_changed() const { return _rotation_changed; }

  /**
   * @brief Emit an output value on rotation_changed. AccessType: outputOnly
   * @details rotation_changed events equal sum of relative bearing changes plus
   * offset value about Y-axis in local coordinate system. outputOnly fields
   * have no author-facing setter; a node's behavior or the runtime calls this
   * to produce an output event. The event cascade reaches it through the
   * reflected field table so producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitRotation_changed(const SFRotation &value) {
    _rotation_changed = value;
  }

  /**
   * @brief The X3D type name of this node (e.g. "CylinderSensor").
   */
  std::string nodeTypeName() const override;

  /**
   * @brief This node's default containerField: the parent field it attaches
   *        to when an X3D-XML element gives no explicit containerField. Virtual
   *        so codecs can resolve it polymorphically through an X3DNode base
   *        pointer (the static getDefaultContainerField() is not reachable that
   *        way). Mirrors getDefaultContainerField().
   */
  std::string defaultContainerField() const override;

  /**
   * @brief Reflected field table for this node (own + inherited fields).
   * @details Built once (function-local static) from this node's descriptors.
   *          Each FieldInfo carries type-erased get/set thunks bound to this
   *          node's strongly-typed accessors so codecs need no per-node code.
   */
  const FieldTable &fields() const override;

  /**
   * @brief Visitor double-dispatch entry point.
   */
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

    if (value < 0)
      throw std::out_of_range("diskAngle below minimum of 0");
    if (value > 1.5708)
      throw std::out_of_range("diskAngle above maximum of 1.5708");
  }

  /**
   * @brief Member variable for axisRotation.
   */

  SFRotation _axisRotation{SFRotation{0, 0, 1, 0}};

  /**
   * @brief Member variable for diskAngle.
   */

  SFFloat _diskAngle{0.26179167};

  /**
   * @brief Member variable for maxAngle.
   */

  SFFloat _maxAngle{-1};

  /**
   * @brief Member variable for minAngle.
   */

  SFFloat _minAngle{0};

  /**
   * @brief Member variable for offset.
   */

  SFFloat _offset{0};

  /**
   * @brief Member variable for rotation_changed.
   */

  SFRotation _rotation_changed{};
};

#endif // CYLINDERSENSOR_HPP