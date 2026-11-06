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
 * @class TransformSensor
 * @brief TransformSensor generates output events when its targetObject enters,
 * exits, and moves within a region in space (defined by a box).
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/environmentalSensor.html#TransformSensor
 */
class TransformSensor : public virtual X3DEnvironmentalSensorNode {
public:
  TransformSensor() = default;

  ~TransformSensor() = default;

  static SFVec3f getDefaultCenter() { return SFVec3f{0, 0, 0}; }

  static SFNode getDefaultTargetObject() { return nullptr; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "children"; }

  static std::string componentName() { return "EnvironmentalSensor"; }

  static int componentLevel() { return 3; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details Translation offset from origin of local coordinate system.
   */
  const SFVec3f &getCenter() const { return _center; }

  /**
   * Access: inputOutput.
   * @details Translation offset from origin of local coordinate system.
   */
  void setCenter(const SFVec3f &value) { _center = value; }

  void setCenter(SFVec3f &&value) { _center = std::move(value); }

  /**
   * SFTime; access: outputOnly. Borrows the stored value.
   * @details Time event generated when targetObject enters the box region for
   * sensor.
   */
  const SFTime &getEnterTime() const { return _enterTime; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Time event generated when targetObject enters the box region for
   * sensor.
   */
  void emitEnterTime(const SFTime &value) { _enterTime = value; }

  /**
   * SFTime; access: outputOnly. Borrows the stored value.
   * @details Time event generated when targetObject exits the box region for
   * sensor.
   */
  const SFTime &getExitTime() const { return _exitTime; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Time event generated when targetObject exits the box region for
   * sensor.
   */
  void emitExitTime(const SFTime &value) { _exitTime = value; }

  /**
   * SFRotation; access: outputOnly. Borrows the stored value.
   * @details Sends rotation event relative to center whenever the target object
   * is contained within the box region and results change.
   */
  const SFRotation &getOrientation_changed() const {
    return _orientation_changed;
  }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Sends rotation event relative to center whenever the target object
   * is contained within the box region and results change.
   */
  void emitOrientation_changed(const SFRotation &value) {
    _orientation_changed = value;
  }

  /**
   * SFVec3f; access: outputOnly. Borrows the stored value.
   * @details Sends translation event relative to center whenever the target
   * object is contained within the box region and results change.
   */
  const SFVec3f &getPosition_changed() const { return _position_changed; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details Sends translation event relative to center whenever the target
   * object is contained within the box region and results change.
   */
  void emitPosition_changed(const SFVec3f &value) { _position_changed = value; }

  /**
   * SFNode; access: inputOutput. Borrows the stored value.
   * @details targetObject is the movable geometry represented by any valid
   * X3DGroupingNode or X3DShapeNode which may enter or exit the box.
   */
  const SFNode &getTargetObject() const { return _targetObject; }

  /**
   * @details Permitted X3D node types: X3DGroupingNode, X3DShapeNode
   */
  static const std::vector<std::string> &acceptableTargetObjectNodeTypes() {
    static const std::vector<std::string> types = {"X3DGroupingNode",
                                                   "X3DShapeNode"};
    return types;
  }

  /**
   * Access: inputOutput.
   * @details targetObject is the movable geometry represented by any valid
   * X3DGroupingNode or X3DShapeNode which may enter or exit the box.
   */
  void setTargetObject(const SFNode &value) { _targetObject = value; }

  void setTargetObject(SFNode &&value) { _targetObject = std::move(value); }

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

  SFTime _enterTime{};

  SFTime _exitTime{};

  SFRotation _orientation_changed{};

  SFVec3f _position_changed{};

  SFNode _targetObject{nullptr};
};

} // namespace x3d::nodes
