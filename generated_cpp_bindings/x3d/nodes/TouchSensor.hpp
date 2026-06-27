// TouchSensor.hpp
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

#include "x3d/nodes/X3DTouchSensorNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class TouchSensor
 * @brief TouchSensor tracks location and state of the pointing device,
 * detecting when a user points at or selects (activates) geometry.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/pointingDeviceSensor.html#TouchSensor
 */
class TouchSensor : public virtual X3DTouchSensorNode {
public:
  /**
   * @brief Default constructor for TouchSensor
   */
  TouchSensor() = default;

  /**
   * @brief Destructor for TouchSensor
   */
  ~TouchSensor() = default;

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
   * @brief Gets the value of hitNormal_changed. AccessType: outputOnly
   * @details When pointing device selects geometry, send event containing
   * surface normal vector at the hitPoint.
   * @return SFVec3f The current value of hitNormal_changed.
   */
  SFVec3f getHitNormal_changed() const { return _hitNormal_changed; }

  /**
   * @brief Emit an output value on hitNormal_changed. AccessType: outputOnly
   * @details When pointing device selects geometry, send event containing
   * surface normal vector at the hitPoint. outputOnly fields have no
   * author-facing setter; a node's behavior or the runtime calls this to
   * produce an output event. The event cascade reaches it through the reflected
   * field table so producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitHitNormal_changed(const SFVec3f &value) {
    _hitNormal_changed = value;
  }

  /**
   * @brief Gets the value of hitPoint_changed. AccessType: outputOnly
   * @details When pointing device selects geometry, send event containing 3D
   * point on surface of underlying geometry, as measured in reference frame for
   * TouchSensor's local coordinate system.
   * @return SFVec3f The current value of hitPoint_changed.
   */
  SFVec3f getHitPoint_changed() const { return _hitPoint_changed; }

  /**
   * @brief Emit an output value on hitPoint_changed. AccessType: outputOnly
   * @details When pointing device selects geometry, send event containing 3D
   * point on surface of underlying geometry, as measured in reference frame for
   * TouchSensor's local coordinate system. outputOnly fields have no
   * author-facing setter; a node's behavior or the runtime calls this to
   * produce an output event. The event cascade reaches it through the reflected
   * field table so producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitHitPoint_changed(const SFVec3f &value) { _hitPoint_changed = value; }

  /**
   * @brief Gets the value of hitTexCoord_changed. AccessType: outputOnly
   * @details When pointing device selects geometry, send event containing
   * texture coordinates of surface at the hitPoint.
   * @return SFVec2f The current value of hitTexCoord_changed.
   */
  SFVec2f getHitTexCoord_changed() const { return _hitTexCoord_changed; }

  /**
   * @brief Emit an output value on hitTexCoord_changed. AccessType: outputOnly
   * @details When pointing device selects geometry, send event containing
   * texture coordinates of surface at the hitPoint. outputOnly fields have no
   * author-facing setter; a node's behavior or the runtime calls this to
   * produce an output event. The event cascade reaches it through the reflected
   * field table so producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitHitTexCoord_changed(const SFVec2f &value) {
    _hitTexCoord_changed = value;
  }

  /**
   * @brief The X3D type name of this node (e.g. "TouchSensor").
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
   * @brief Member variable for hitNormal_changed.
   */

  SFVec3f _hitNormal_changed{};

  /**
   * @brief Member variable for hitPoint_changed.
   */

  SFVec3f _hitPoint_changed{};

  /**
   * @brief Member variable for hitTexCoord_changed.
   */

  SFVec2f _hitTexCoord_changed{};
};

} // namespace x3d::nodes
