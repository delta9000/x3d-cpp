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

#include "x3d/nodes/X3DEnvironmentalSensorNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class ProximitySensor
 * @brief ProximitySensor generates events when the viewer enters, exits and
 * moves within a region of space (defined by a box).
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/environmentalSensor.html#ProximitySensor
 */
class ProximitySensor : public virtual X3DEnvironmentalSensorNode {
public:
  ProximitySensor() = default;

  ~ProximitySensor() = default;

  static SFVec3f getDefaultCenter() { return SFVec3f{0, 0, 0}; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "EnvironmentalSensor"; }

  static int componentLevel() { return 1; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Position offset from origin of local coordinate system.
   */
  const SFVec3f &getCenter() const { return _center; }

  /**
   * Access: inputOutput.
   * @details Position offset from origin of local coordinate system.
   */
  void setCenter(const SFVec3f &value) { _center = value; }

  void setCenter(SFVec3f &&value) { _center = std::move(value); }

  /**
   * SFVec3f; access: outputOnly. Borrows the stored value.
   * @details Sends changed centerOfRotation values, likely caused by user
   * interaction.
   */
  const SFVec3f &getCenterOfRotation_changed() const {
    return _centerOfRotation_changed;
  }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Sends changed centerOfRotation values, likely caused by user
   * interaction.
   */
  void emitCenterOfRotation_changed(const SFVec3f &value) {
    _centerOfRotation_changed = value;
  }

  /**
   * SFTime; access: outputOnly. Borrows the stored value.
   * @details Time event generated when user's camera enters the box.
   */
  const SFTime &getEnterTime() const { return _enterTime; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Time event generated when user's camera enters the box.
   */
  void emitEnterTime(const SFTime &value) { _enterTime = value; }

  /**
   * SFTime; access: outputOnly. Borrows the stored value.
   * @details Time event generated when user's camera exits the box.
   */
  const SFTime &getExitTime() const { return _exitTime; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Time event generated when user's camera exits the box.
   */
  void emitExitTime(const SFTime &value) { _exitTime = value; }

  /**
   * SFRotation; access: outputOnly. Borrows the stored value.
   * @details Sends rotation event relative to center.
   */
  const SFRotation &getOrientation_changed() const {
    return _orientation_changed;
  }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Sends rotation event relative to center.
   */
  void emitOrientation_changed(const SFRotation &value) {
    _orientation_changed = value;
  }

  /**
   * SFVec3f; access: outputOnly. Borrows the stored value.
   * @details Sends translation event relative to center.
   */
  const SFVec3f &getPosition_changed() const { return _position_changed; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Sends translation event relative to center.
   */
  void emitPosition_changed(const SFVec3f &value) { _position_changed = value; }

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

private:
  SFVec3f _center{SFVec3f{0, 0, 0}};

  SFVec3f _centerOfRotation_changed{};

  SFTime _enterTime{};

  SFTime _exitTime{};

  SFRotation _orientation_changed{};

  SFVec3f _position_changed{};
};

} // namespace x3d::nodes
