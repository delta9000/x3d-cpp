// SingleAxisHingeJoint.hpp
#pragma once

#include "x3d/core/X3DReflection.hpp"
#include "x3d/core/X3Denums.hpp"
#include "x3d/core/X3Dtypes.hpp"
#include <algorithm>
#include <any>
#include <functional>
#include <stdexcept>

#include "x3d/nodes/X3DNode.hpp"

#include "x3d/nodes/X3DRigidJointNode.hpp"

namespace x3d::nodes {
using namespace x3d::core;

/**
 * @class SingleAxisHingeJoint
 * @brief SingleAxisHingeJoint has single axis about which to rotate, similar to
 * a traditional door hinge. Contains two RigidBody nodes (containerField values
 * body1, body2).
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rigidBodyPhysics.html#SingleAxisHingeJoint
 */
class SingleAxisHingeJoint : public virtual X3DRigidJointNode {
public:
  /**
   * @brief Default constructor for SingleAxisHingeJoint
   */
  SingleAxisHingeJoint() = default;

  /**
   * @brief Destructor for SingleAxisHingeJoint
   */
  ~SingleAxisHingeJoint() = default;

  /**
   * @brief Get the default value for anchorPoint
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultAnchorPoint() { return SFVec3f{0, 0, 0}; }

  /**
   * @brief Get the default value for axis
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultAxis() { return SFVec3f{0, 1, 0}; }

  /**
   * @brief Get the default value for maxAngle
   * @return SFFloat The default value
   */
  static SFFloat getDefaultMaxAngle() { return 3.141592653; }

  /**
   * @brief Get the default value for minAngle
   * @return SFFloat The default value
   */
  static SFFloat getDefaultMinAngle() { return -3.141592653; }

  /**
   * @brief Get the default value for stopBounce
   * @return SFFloat The default value
   */
  static SFFloat getDefaultStopBounce() { return 0; }

  /**
   * @brief Get the default value for stopErrorCorrection
   * @return SFFloat The default value
   */
  static SFFloat getDefaultStopErrorCorrection() { return 0.8; }

  /**
   * @brief Get the container field type for this node
   * @return std::string The container field type
   */
  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  /**
   * @brief Get the default container field value
   * @return std::string The default container field value
   */
  static std::string getDefaultContainerField() { return "joints"; }

  /**
   * @brief Get the X3D component this node belongs to
   * @return std::string The component name
   */
  static std::string componentName() { return "RigidBodyPhysics"; }

  /**
   * @brief Get the X3D component support level for this node
   * @return int The component level
   */
  static int componentLevel() { return 2; }

  /**
   * @brief Gets the value of anchorPoint. AccessType: inputOutput
   * @details anchorPoint is joint center, specified in world coordinates.
   * @return SFVec3f The current value of anchorPoint.
   */
  SFVec3f getAnchorPoint() const { return _anchorPoint; }

  /**
   * @brief Sets the value of anchorPoint. AccessType: inputOutput
   * @details anchorPoint is joint center, specified in world coordinates.
   * @param value The new value for anchorPoint.
   */
  void setAnchorPoint(const SFVec3f &value) { _anchorPoint = value; }

  void setAnchorPoint(SFVec3f &&value) { _anchorPoint = std::move(value); }

  /**
   * @brief Gets the value of angle. AccessType: outputOnly
   * @details
   * @return SFFloat The current value of angle.
   */
  SFFloat getAngle() const { return _angle; }

  /**
   * @brief Emit an output value on angle. AccessType: outputOnly
   * @details
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitAngle(const SFFloat &value) { _angle = value; }

  /**
   * @brief Gets the value of angleRate. AccessType: outputOnly
   * @details
   * @return SFFloat The current value of angleRate.
   */
  SFFloat getAngleRate() const { return _angleRate; }

  /**
   * @brief Emit an output value on angleRate. AccessType: outputOnly
   * @details
   *          outputOnly fields have no author-facing setter; a node's behavior
   *          or the runtime calls this to produce an output event. The event
   *          cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitAngleRate(const SFFloat &value) { _angleRate = value; }

  /**
   * @brief Gets the value of axis. AccessType: inputOutput
   * @details axis defines vector of joint connection between body1 and body2.
   * @return SFVec3f The current value of axis.
   */
  SFVec3f getAxis() const { return _axis; }

  /**
   * @brief Sets the value of axis. AccessType: inputOutput
   * @details axis defines vector of joint connection between body1 and body2.
   * @param value The new value for axis.
   */
  void setAxis(const SFVec3f &value) { _axis = value; }

  void setAxis(SFVec3f &&value) { _axis = std::move(value); }

  /**
   * @brief Gets the value of body1AnchorPoint. AccessType: outputOnly
   * @details body1AnchorPoint describes anchorPoint position relative to local
   * coordinate reference frame.
   * @return SFVec3f The current value of body1AnchorPoint.
   */
  SFVec3f getBody1AnchorPoint() const { return _body1AnchorPoint; }

  /**
   * @brief Emit an output value on body1AnchorPoint. AccessType: outputOnly
   * @details body1AnchorPoint describes anchorPoint position relative to local
   * coordinate reference frame. outputOnly fields have no author-facing setter;
   * a node's behavior or the runtime calls this to produce an output event. The
   * event cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitBody1AnchorPoint(const SFVec3f &value) { _body1AnchorPoint = value; }

  /**
   * @brief Gets the value of body2AnchorPoint. AccessType: outputOnly
   * @details body2AnchorPoint describes anchorPoint position relative to local
   * coordinate reference frame.
   * @return SFVec3f The current value of body2AnchorPoint.
   */
  SFVec3f getBody2AnchorPoint() const { return _body2AnchorPoint; }

  /**
   * @brief Emit an output value on body2AnchorPoint. AccessType: outputOnly
   * @details body2AnchorPoint describes anchorPoint position relative to local
   * coordinate reference frame. outputOnly fields have no author-facing setter;
   * a node's behavior or the runtime calls this to produce an output event. The
   * event cascade reaches it through the reflected field table so producing
   *          outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitBody2AnchorPoint(const SFVec3f &value) { _body2AnchorPoint = value; }

  /**
   * @brief Gets the value of maxAngle. AccessType: inputOutput
   * @details maxAngle is maximum rotation angle for hinge.
   * @return SFFloat The current value of maxAngle.
   */
  SFFloat getMaxAngle() const { return _maxAngle; }

  /**
   * @brief Sets the value of maxAngle. AccessType: inputOutput
   * @details maxAngle is maximum rotation angle for hinge.
   * @param value The new value for maxAngle.
   */
  void setMaxAngle(const SFFloat &value) { _maxAngle = value; }

  /**
   * @brief Gets the value of minAngle. AccessType: inputOutput
   * @details minAngle is minimum rotation angle for hinge.
   * @return SFFloat The current value of minAngle.
   */
  SFFloat getMinAngle() const { return _minAngle; }

  /**
   * @brief Sets the value of minAngle. AccessType: inputOutput
   * @details minAngle is minimum rotation angle for hinge.
   * @param value The new value for minAngle.
   */
  void setMinAngle(const SFFloat &value) { _minAngle = value; }

  /**
   * @brief Gets the value of stopBounce. AccessType: inputOutput
   * @details stopBounce is velocity factor for bounce back once stop point is
   * reached.
   * @return SFFloat The current value of stopBounce.
   */
  SFFloat getStopBounce() const { return _stopBounce; }

  /**
   * @brief Sets the value of stopBounce. AccessType: inputOutput
   * @details stopBounce is velocity factor for bounce back once stop point is
   * reached.
   * @param value The new value for stopBounce.
   */
  void setStopBounce(const SFFloat &value) { _stopBounce = value; }

  /**
   * @brief Gets the value of stopErrorCorrection. AccessType: inputOutput
   * @details stopErrorCorrection is fraction of error correction performed
   * during time step once stop point is reached.
   * @return SFFloat The current value of stopErrorCorrection.
   */
  SFFloat getStopErrorCorrection() const { return _stopErrorCorrection; }

  /**
   * @brief Sets the value of stopErrorCorrection. AccessType: inputOutput
   * @details stopErrorCorrection is fraction of error correction performed
   * during time step once stop point is reached.
   * @param value The new value for stopErrorCorrection.
   */
  void setStopErrorCorrection(const SFFloat &value) {

    _stopErrorCorrection = value;
  }

  /**
   * @brief The X3D type name of this node (e.g. "SingleAxisHingeJoint").
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
   * @brief Member variable for anchorPoint.
   */

  SFVec3f _anchorPoint{SFVec3f{0, 0, 0}};

  /**
   * @brief Member variable for angle.
   */

  SFFloat _angle{};

  /**
   * @brief Member variable for angleRate.
   */

  SFFloat _angleRate{};

  /**
   * @brief Member variable for axis.
   */

  SFVec3f _axis{SFVec3f{0, 1, 0}};

  /**
   * @brief Member variable for body1AnchorPoint.
   */

  SFVec3f _body1AnchorPoint{};

  /**
   * @brief Member variable for body2AnchorPoint.
   */

  SFVec3f _body2AnchorPoint{};

  /**
   * @brief Member variable for maxAngle.
   */

  SFFloat _maxAngle{3.141592653};

  /**
   * @brief Member variable for minAngle.
   */

  SFFloat _minAngle{-3.141592653};

  /**
   * @brief Member variable for stopBounce.
   */

  SFFloat _stopBounce{0};

  /**
   * @brief Member variable for stopErrorCorrection.
   */

  SFFloat _stopErrorCorrection{0.8};
};

} // namespace x3d::nodes
