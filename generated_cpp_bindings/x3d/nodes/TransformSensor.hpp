// TransformSensor.hpp
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
  /**
   * @brief Default constructor for TransformSensor
   */
  TransformSensor() = default;

  /**
   * @brief Destructor for TransformSensor
   */
  ~TransformSensor() = default;

  /**
   * @brief Get the default value for center
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultCenter() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the default value for targetObject
   * @return SFNode The default value
   */
  static SFNode getDefaultTargetObject() { return nullptr; }

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
  static std::string componentName() { return "EnvironmentalSensor"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 3; }

  /**
   * @brief Gets the value of center. AccessType: inputOutput
   * @details Translation offset from origin of local coordinate system.
   * @return SFVec3f The current value of center.
   */
  SFVec3f getCenter() const { return _center; }

  /**
   * @brief Sets the value of center. AccessType: inputOutput
   * @details Translation offset from origin of local coordinate system.
   * @param value The new value for center.
   */
  void setCenter(const SFVec3f &value) { _center = value; }

  void setCenter(SFVec3f &&value) { _center = std::move(value); }

  /**
   * @brief Gets the value of enterTime. AccessType: outputOnly
   * @details Time event generated when targetObject enters the box region for
   * sensor.
   * @return SFTime The current value of enterTime.
   */
  SFTime getEnterTime() const { return _enterTime; }

  /**
   * @brief Emit an output value on enterTime. AccessType: outputOnly
   * @details Time event generated when targetObject enters the box region for
   * sensor. outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitEnterTime(const SFTime &value) { _enterTime = value; }

  /**
   * @brief Gets the value of exitTime. AccessType: outputOnly
   * @details Time event generated when targetObject exits the box region for
   * sensor.
   * @return SFTime The current value of exitTime.
   */
  SFTime getExitTime() const { return _exitTime; }

  /**
   * @brief Emit an output value on exitTime. AccessType: outputOnly
   * @details Time event generated when targetObject exits the box region for
   * sensor. outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitExitTime(const SFTime &value) { _exitTime = value; }

  /**
   * @brief Gets the value of orientation_changed. AccessType: outputOnly
   * @details Sends rotation event relative to center whenever the target object
   * is contained within the box region and results change.
   * @return SFRotation The current value of orientation_changed.
   */
  SFRotation getOrientation_changed() const { return _orientation_changed; }

  /**
   * @brief Emit an output value on orientation_changed. AccessType: outputOnly
   * @details Sends rotation event relative to center whenever the target object
   * is contained within the box region and results change. outputOnly fields
   * have no author-facing setter; a node's behavior or the runtime calls this
   * to produce an output event. The event cascade reaches it through the
   * reflected field table so producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitOrientation_changed(const SFRotation &value) {
    _orientation_changed = value;
  }

  /**
   * @brief Gets the value of position_changed. AccessType: outputOnly
   * @details Sends translation event relative to center whenever the target
   * object is contained within the box region and results change.
   * @return SFVec3f The current value of position_changed.
   */
  SFVec3f getPosition_changed() const { return _position_changed; }

  /**
   * @brief Emit an output value on position_changed. AccessType: outputOnly
   * @details Sends translation event relative to center whenever the target
   * object is contained within the box region and results change. outputOnly
   * fields have no author-facing setter; a node's behavior or the runtime calls
   * this to produce an output event. The event cascade reaches it through the
   * reflected field table so producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitPosition_changed(const SFVec3f &value) { _position_changed = value; }

  /**
   * @brief Gets the value of targetObject. AccessType: inputOutput
   * @details targetObject is the movable geometry represented by any valid
   * X3DGroupingNode or X3DShapeNode which may enter or exit the box.
   * @return SFNode The current value of targetObject.
   */
  SFNode getTargetObject() const { return _targetObject; }

  /**
   * @brief Acceptable node types for the targetObject field.
   * @details Permitted X3D node types: X3DGroupingNode, X3DShapeNode
   * @return const std::vector<std::string>& The accepted node type names.
   */
  static const std::vector<std::string> &acceptableTargetObjectNodeTypes() {
    static const std::vector<std::string> types = {"X3DGroupingNode",
                                                   "X3DShapeNode"};
    return types;
  }

  /**
   * @brief Sets the value of targetObject. AccessType: inputOutput
   * @details targetObject is the movable geometry represented by any valid
   * X3DGroupingNode or X3DShapeNode which may enter or exit the box.
   * @param value The new value for targetObject.
   */
  void setTargetObject(const SFNode &value) { _targetObject = value; }

  void setTargetObject(SFNode &&value) { _targetObject = std::move(value); }

  /**
   * @brief The X3D type name of this node (e.g. "TransformSensor").
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

private:
  /**
   * @brief Member variable for center.
   */

  SFVec3f _center{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for enterTime.
   */

  SFTime _enterTime{};

  /**
   * @brief Member variable for exitTime.
   */

  SFTime _exitTime{};

  /**
   * @brief Member variable for orientation_changed.
   */

  SFRotation _orientation_changed{};

  /**
   * @brief Member variable for position_changed.
   */

  SFVec3f _position_changed{};

  /**
   * @brief Member variable for targetObject.
   */

  SFNode _targetObject{nullptr};
};

} // namespace x3d::nodes
