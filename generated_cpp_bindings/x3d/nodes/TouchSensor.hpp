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
  TouchSensor() = default;

  ~TouchSensor() = default;

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "PointingDeviceSensor"; }

  static int componentLevel() { return 1; }

  /**
   * SFVec3f; access: outputOnly. Borrows the stored value.
   * @details When pointing device selects geometry, send event containing
   * surface normal vector at the hitPoint.
   */
  const SFVec3f &getHitNormal_changed() const { return _hitNormal_changed; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details When pointing device selects geometry, send event containing
   * surface normal vector at the hitPoint.
   */
  void emitHitNormal_changed(const SFVec3f &value) {
    _hitNormal_changed = value;
  }

  /**
   * SFVec3f; access: outputOnly. Borrows the stored value.
   * @details When pointing device selects geometry, send event containing 3D
   * point on surface of underlying geometry, as measured in reference frame for
   * TouchSensor's local coordinate system.
   */
  const SFVec3f &getHitPoint_changed() const { return _hitPoint_changed; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details When pointing device selects geometry, send event containing 3D
   * point on surface of underlying geometry, as measured in reference frame for
   * TouchSensor's local coordinate system.
   */
  void emitHitPoint_changed(const SFVec3f &value) { _hitPoint_changed = value; }

  /**
   * SFVec2f; access: outputOnly. Borrows the stored value.
   * @details When pointing device selects geometry, send event containing
   * texture coordinates of surface at the hitPoint.
   */
  const SFVec2f &getHitTexCoord_changed() const { return _hitTexCoord_changed; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details When pointing device selects geometry, send event containing
   * texture coordinates of surface at the hitPoint.
   */
  void emitHitTexCoord_changed(const SFVec2f &value) {
    _hitTexCoord_changed = value;
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
  SFVec3f _hitNormal_changed{};

  SFVec3f _hitPoint_changed{};

  SFVec2f _hitTexCoord_changed{};
};

} // namespace x3d::nodes
