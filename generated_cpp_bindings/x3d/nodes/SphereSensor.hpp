// SphereSensor.hpp
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
  /**
   * @brief Default constructor for SphereSensor
   */
  SphereSensor() = default;

  /**
   * @brief Destructor for SphereSensor
   */
  ~SphereSensor() = default;

  /**
   * @brief Get the default value for offset
   * @return SFRotation The default value
   */
  static SFRotation getDefaultOffset() { return SFRotation{0, 1, 0, 0}; }

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
   * @brief Gets the value of offset. AccessType: inputOutput
   * @details Sends event and remembers last value sensed.
   * @return SFRotation The current value of offset.
   */
  SFRotation getOffset() const { return _offset; }

  /**
   * @brief Sets the value of offset. AccessType: inputOutput
   * @details Sends event and remembers last value sensed.
   * @param value The new value for offset.
   */
  void setOffset(const SFRotation &value) { _offset = value; }

  void setOffset(SFRotation &&value) { _offset = std::move(value); }

  /**
   * @brief Gets the value of rotation_changed. AccessType: outputOnly
   * @details rotation_changed events equal sum of relative bearing changes plus
   * offset value.
   * @return SFRotation The current value of rotation_changed.
   */
  SFRotation getRotation_changed() const { return _rotation_changed; }

  /**
   * @brief Emit an output value on rotation_changed. AccessType: outputOnly
   * @details rotation_changed events equal sum of relative bearing changes plus
   * offset value. outputOnly fields have no author-facing setter; a node's
   * behavior or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitRotation_changed(const SFRotation &value) {
    _rotation_changed = value;
  }

  /**
   * @brief The X3D type name of this node (e.g. "SphereSensor").
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

private:
  /**
   * @brief Member variable for offset.
   */

  SFRotation _offset{SFRotation{0, 1, 0, 0}};

  /**
   * @brief Member variable for rotation_changed.
   */

  SFRotation _rotation_changed{};
};

} // namespace x3d::nodes
