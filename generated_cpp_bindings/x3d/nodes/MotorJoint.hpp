// MotorJoint.hpp
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
 * @class MotorJoint
 * @brief MotorJoint drives relative angular velocities between body1 and body2
 * within a common reference frame.
 * @details
 * https://www.web3d.org/specifications/X3Dv4/ISO-IEC19775-1v4-IS/Part01/components/rigidBodyPhysics.html#MotorJoint
 */
class MotorJoint : public virtual X3DRigidJointNode {
public:
  /**
   * @brief Default constructor for MotorJoint
   */
  MotorJoint() = default;

  /**
   * @brief Destructor for MotorJoint
   */
  ~MotorJoint() = default;

  /**
   * @brief Get the default value for autoCalc
   * @return SFBool The default value
   */
  static SFBool getDefaultAutoCalc() { return false; }

  /**
   * @brief Get the default value for axis1Angle
   * @return SFFloat The default value
   */
  static SFFloat getDefaultAxis1Angle() { return 0; }

  /**
   * @brief Get the default value for axis1Torque
   * @return SFFloat The default value
   */
  static SFFloat getDefaultAxis1Torque() { return 0; }

  /**
   * @brief Get the default value for axis2Angle
   * @return SFFloat The default value
   */
  static SFFloat getDefaultAxis2Angle() { return 0; }

  /**
   * @brief Get the default value for axis2Torque
   * @return SFFloat The default value
   */
  static SFFloat getDefaultAxis2Torque() { return 0; }

  /**
   * @brief Get the default value for axis3Angle
   * @return SFFloat The default value
   */
  static SFFloat getDefaultAxis3Angle() { return 0; }

  /**
   * @brief Get the default value for axis3Torque
   * @return SFFloat The default value
   */
  static SFFloat getDefaultAxis3Torque() { return 0; }

  /**
   * @brief Get the default value for enabledAxes
   * @return SFInt32 The default value
   */
  static SFInt32 getDefaultEnabledAxes() { return 1; }

  /**
   * @brief Get the default value for motor1Axis
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultMotor1Axis() { return SFVec3f{1, 0, 0}; }

  /**
   * @brief Get the default value for motor2Axis
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultMotor2Axis() { return SFVec3f{0, 1, 0}; }

  /**
   * @brief Get the default value for motor3Axis
   * @return SFVec3f The default value
   */
  static SFVec3f getDefaultMotor3Axis() { return SFVec3f{0, 0, 1}; }

  /**
   * @brief Get the default value for stop1Bounce
   * @return SFFloat The default value
   */
  static SFFloat getDefaultStop1Bounce() { return 0; }

  /**
   * @brief Get the default value for stop1ErrorCorrection
   * @return SFFloat The default value
   */
  static SFFloat getDefaultStop1ErrorCorrection() { return 0.8; }

  /**
   * @brief Get the default value for stop2Bounce
   * @return SFFloat The default value
   */
  static SFFloat getDefaultStop2Bounce() { return 0; }

  /**
   * @brief Get the default value for stop2ErrorCorrection
   * @return SFFloat The default value
   */
  static SFFloat getDefaultStop2ErrorCorrection() { return 0.8; }

  /**
   * @brief Get the default value for stop3Bounce
   * @return SFFloat The default value
   */
  static SFFloat getDefaultStop3Bounce() { return 0; }

  /**
   * @brief Get the default value for stop3ErrorCorrection
   * @return SFFloat The default value
   */
  static SFFloat getDefaultStop3ErrorCorrection() { return 0.8; }

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
   * @brief Gets the value of autoCalc. AccessType: initializeOnly
   * @details autoCalc controls whether user manually provides individual angle
   * rotations each frame (false) or if angle values are automatically
   * calculated by motor implementations (true).
   * @return SFBool The current value of autoCalc.
   */
  SFBool getAutoCalc() const { return _autoCalc; }
  /**
   * @brief Data-layer write of autoCalc (reader/init ingest path).
   * @details autoCalc is initializeOnly: author-settable at parse
   *          time but not via runtime events. No public setAutoCalc().
   */
  void setAutoCalcUnchecked(const SFBool &value) { _autoCalc = value; }

  /**
   * @brief Gets the value of axis1Angle. AccessType: inputOutput
   * @details axis1Angle (radians) is rotation angle for corresponding motor
   * axis when in user-calculated mode.
   * @return SFFloat The current value of axis1Angle.
   */
  SFFloat getAxis1Angle() const { return _axis1Angle; }

  /**
   * @brief Sets the value of axis1Angle. AccessType: inputOutput
   * @details axis1Angle (radians) is rotation angle for corresponding motor
   * axis when in user-calculated mode.
   * @param value The new value for axis1Angle.
   */
  void setAxis1Angle(const SFFloat &value) { _axis1Angle = value; }

  /**
   * @brief Gets the value of axis1Torque. AccessType: inputOutput
   * @details axis1Torque is rotational torque applied by corresponding motor
   * axis when in user-calculated mode.
   * @return SFFloat The current value of axis1Torque.
   */
  SFFloat getAxis1Torque() const { return _axis1Torque; }

  /**
   * @brief Sets the value of axis1Torque. AccessType: inputOutput
   * @details axis1Torque is rotational torque applied by corresponding motor
   * axis when in user-calculated mode.
   * @param value The new value for axis1Torque.
   */
  void setAxis1Torque(const SFFloat &value) { _axis1Torque = value; }

  /**
   * @brief Gets the value of axis2Angle. AccessType: inputOutput
   * @details axis2Angle (radians) is rotation angle for corresponding motor
   * axis when in user-calculated mode.
   * @return SFFloat The current value of axis2Angle.
   */
  SFFloat getAxis2Angle() const { return _axis2Angle; }

  /**
   * @brief Sets the value of axis2Angle. AccessType: inputOutput
   * @details axis2Angle (radians) is rotation angle for corresponding motor
   * axis when in user-calculated mode.
   * @param value The new value for axis2Angle.
   */
  void setAxis2Angle(const SFFloat &value) { _axis2Angle = value; }

  /**
   * @brief Gets the value of axis2Torque. AccessType: inputOutput
   * @details axis2Torque is rotational torque applied by corresponding motor
   * axis when in user-calculated mode.
   * @return SFFloat The current value of axis2Torque.
   */
  SFFloat getAxis2Torque() const { return _axis2Torque; }

  /**
   * @brief Sets the value of axis2Torque. AccessType: inputOutput
   * @details axis2Torque is rotational torque applied by corresponding motor
   * axis when in user-calculated mode.
   * @param value The new value for axis2Torque.
   */
  void setAxis2Torque(const SFFloat &value) { _axis2Torque = value; }

  /**
   * @brief Gets the value of axis3Angle. AccessType: inputOutput
   * @details axis3Angle (radians) is rotation angle for corresponding motor
   * axis when in user-calculated mode.
   * @return SFFloat The current value of axis3Angle.
   */
  SFFloat getAxis3Angle() const { return _axis3Angle; }

  /**
   * @brief Sets the value of axis3Angle. AccessType: inputOutput
   * @details axis3Angle (radians) is rotation angle for corresponding motor
   * axis when in user-calculated mode.
   * @param value The new value for axis3Angle.
   */
  void setAxis3Angle(const SFFloat &value) { _axis3Angle = value; }

  /**
   * @brief Gets the value of axis3Torque. AccessType: inputOutput
   * @details axis3Torque is rotational torque applied by corresponding motor
   * axis when in user-calculated mode.
   * @return SFFloat The current value of axis3Torque.
   */
  SFFloat getAxis3Torque() const { return _axis3Torque; }

  /**
   * @brief Sets the value of axis3Torque. AccessType: inputOutput
   * @details axis3Torque is rotational torque applied by corresponding motor
   * axis when in user-calculated mode.
   * @param value The new value for axis3Torque.
   */
  void setAxis3Torque(const SFFloat &value) { _axis3Torque = value; }

  /**
   * @brief Gets the value of enabledAxes. AccessType: inputOutput
   * @details enabledAxes indicates which motor axes are active.
   * @return SFInt32 The current value of enabledAxes.
   */
  SFInt32 getEnabledAxes() const { return _enabledAxes; }

  /**
   * @brief Sets the value of enabledAxes. AccessType: inputOutput
   * @details enabledAxes indicates which motor axes are active.
   * @param value The new value for enabledAxes.
   */
  void setEnabledAxes(const SFInt32 &value) {

    validateEnabledAxes(value);

    _enabledAxes = value;
  }

  /**
   * @brief Non-validating write of enabledAxes (runtime/reader ingest path).
   * @details Assigns without the range check setEnabledAxes() enforces, so a
   *          permissive reader keeps an out-of-range authored value rather than
   *          reject the whole document. setEnabledAxes() stays the
   *          enforcement point for programmatic callers.
   */
  void setEnabledAxesUnchecked(const SFInt32 &value) { _enabledAxes = value; }

  /**
   * @brief Gets the value of motor1Angle. AccessType: outputOnly
   * @details motor1Angle provides calculated angle of rotation (radians) for
   * this motor joint from last frame.
   * @return SFFloat The current value of motor1Angle.
   */
  SFFloat getMotor1Angle() const { return _motor1Angle; }

  /**
   * @brief Emit an output value on motor1Angle. AccessType: outputOnly
   * @details motor1Angle provides calculated angle of rotation (radians) for
   * this motor joint from last frame. outputOnly fields have no author-facing
   * setter; a node's behavior or the runtime calls this to produce an output
   * event. The event cascade reaches it through the reflected field table so
   * producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitMotor1Angle(const SFFloat &value) { _motor1Angle = value; }

  /**
   * @brief Gets the value of motor1AngleRate. AccessType: outputOnly
   * @details motor1AngleRate provides calculated anglular rotation rate
   * (radians/second) for this motor joint from last frame.
   * @return SFFloat The current value of motor1AngleRate.
   */
  SFFloat getMotor1AngleRate() const { return _motor1AngleRate; }

  /**
   * @brief Emit an output value on motor1AngleRate. AccessType: outputOnly
   * @details motor1AngleRate provides calculated anglular rotation rate
   * (radians/second) for this motor joint from last frame. outputOnly fields
   * have no author-facing setter; a node's behavior or the runtime calls this
   * to produce an output event. The event cascade reaches it through the
   * reflected field table so producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitMotor1AngleRate(const SFFloat &value) { _motor1AngleRate = value; }

  /**
   * @brief Gets the value of motor1Axis. AccessType: inputOutput
   * @details motor1Axis defines axis vector of corresponding motor axis.
   * @return SFVec3f The current value of motor1Axis.
   */
  SFVec3f getMotor1Axis() const { return _motor1Axis; }

  /**
   * @brief Sets the value of motor1Axis. AccessType: inputOutput
   * @details motor1Axis defines axis vector of corresponding motor axis.
   * @param value The new value for motor1Axis.
   */
  void setMotor1Axis(const SFVec3f &value) { _motor1Axis = value; }

  void setMotor1Axis(SFVec3f &&value) { _motor1Axis = std::move(value); }

  /**
   * @brief Gets the value of motor2Angle. AccessType: outputOnly
   * @details motor2Angle provides calculated angle of rotation (radians) for
   * this motor joint from last frame.
   * @return SFFloat The current value of motor2Angle.
   */
  SFFloat getMotor2Angle() const { return _motor2Angle; }

  /**
   * @brief Emit an output value on motor2Angle. AccessType: outputOnly
   * @details motor2Angle provides calculated angle of rotation (radians) for
   * this motor joint from last frame. outputOnly fields have no author-facing
   * setter; a node's behavior or the runtime calls this to produce an output
   * event. The event cascade reaches it through the reflected field table so
   * producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitMotor2Angle(const SFFloat &value) { _motor2Angle = value; }

  /**
   * @brief Gets the value of motor2AngleRate. AccessType: outputOnly
   * @details motor2AngleRate provides calculated anglular rotation rate
   * (radians/second) for this motor joint from last frame.
   * @return SFFloat The current value of motor2AngleRate.
   */
  SFFloat getMotor2AngleRate() const { return _motor2AngleRate; }

  /**
   * @brief Emit an output value on motor2AngleRate. AccessType: outputOnly
   * @details motor2AngleRate provides calculated anglular rotation rate
   * (radians/second) for this motor joint from last frame. outputOnly fields
   * have no author-facing setter; a node's behavior or the runtime calls this
   * to produce an output event. The event cascade reaches it through the
   * reflected field table so producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitMotor2AngleRate(const SFFloat &value) { _motor2AngleRate = value; }

  /**
   * @brief Gets the value of motor2Axis. AccessType: inputOutput
   * @details motor2Axis defines axis vector of corresponding motor axis.
   * @return SFVec3f The current value of motor2Axis.
   */
  SFVec3f getMotor2Axis() const { return _motor2Axis; }

  /**
   * @brief Sets the value of motor2Axis. AccessType: inputOutput
   * @details motor2Axis defines axis vector of corresponding motor axis.
   * @param value The new value for motor2Axis.
   */
  void setMotor2Axis(const SFVec3f &value) { _motor2Axis = value; }

  void setMotor2Axis(SFVec3f &&value) { _motor2Axis = std::move(value); }

  /**
   * @brief Gets the value of motor3Angle. AccessType: outputOnly
   * @details motor3Angle provides calculated angle of rotation (radians) for
   * this motor joint from last frame.
   * @return SFFloat The current value of motor3Angle.
   */
  SFFloat getMotor3Angle() const { return _motor3Angle; }

  /**
   * @brief Emit an output value on motor3Angle. AccessType: outputOnly
   * @details motor3Angle provides calculated angle of rotation (radians) for
   * this motor joint from last frame. outputOnly fields have no author-facing
   * setter; a node's behavior or the runtime calls this to produce an output
   * event. The event cascade reaches it through the reflected field table so
   * producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitMotor3Angle(const SFFloat &value) { _motor3Angle = value; }

  /**
   * @brief Gets the value of motor3AngleRate. AccessType: outputOnly
   * @details motor3AngleRate provides calculated anglular rotation rate
   * (radians/second) for this motor joint from last frame.
   * @return SFFloat The current value of motor3AngleRate.
   */
  SFFloat getMotor3AngleRate() const { return _motor3AngleRate; }

  /**
   * @brief Emit an output value on motor3AngleRate. AccessType: outputOnly
   * @details motor3AngleRate provides calculated anglular rotation rate
   * (radians/second) for this motor joint from last frame. outputOnly fields
   * have no author-facing setter; a node's behavior or the runtime calls this
   * to produce an output event. The event cascade reaches it through the
   * reflected field table so producing outputs is node-agnostic.
   * @param value The value to emit.
   */
  void emitMotor3AngleRate(const SFFloat &value) { _motor3AngleRate = value; }

  /**
   * @brief Gets the value of motor3Axis. AccessType: inputOutput
   * @details motor3Axis defines axis vector of corresponding motor axis.
   * @return SFVec3f The current value of motor3Axis.
   */
  SFVec3f getMotor3Axis() const { return _motor3Axis; }

  /**
   * @brief Sets the value of motor3Axis. AccessType: inputOutput
   * @details motor3Axis defines axis vector of corresponding motor axis.
   * @param value The new value for motor3Axis.
   */
  void setMotor3Axis(const SFVec3f &value) { _motor3Axis = value; }

  void setMotor3Axis(SFVec3f &&value) { _motor3Axis = std::move(value); }

  /**
   * @brief Gets the value of stop1Bounce. AccessType: inputOutput
   * @details stop1Bounce is velocity factor for bounce back once stop point is
   * reached.
   * @return SFFloat The current value of stop1Bounce.
   */
  SFFloat getStop1Bounce() const { return _stop1Bounce; }

  /**
   * @brief Sets the value of stop1Bounce. AccessType: inputOutput
   * @details stop1Bounce is velocity factor for bounce back once stop point is
   * reached.
   * @param value The new value for stop1Bounce.
   */
  void setStop1Bounce(const SFFloat &value) { _stop1Bounce = value; }

  /**
   * @brief Gets the value of stop1ErrorCorrection. AccessType: inputOutput
   * @details stop1ErrorCorrection is fraction of error correction performed
   * during time step once stop point is reached.
   * @return SFFloat The current value of stop1ErrorCorrection.
   */
  SFFloat getStop1ErrorCorrection() const { return _stop1ErrorCorrection; }

  /**
   * @brief Sets the value of stop1ErrorCorrection. AccessType: inputOutput
   * @details stop1ErrorCorrection is fraction of error correction performed
   * during time step once stop point is reached.
   * @param value The new value for stop1ErrorCorrection.
   */
  void setStop1ErrorCorrection(const SFFloat &value) {

    _stop1ErrorCorrection = value;
  }

  /**
   * @brief Gets the value of stop2Bounce. AccessType: inputOutput
   * @details stop2Bounce is velocity factor for bounce back once stop point is
   * reached.
   * @return SFFloat The current value of stop2Bounce.
   */
  SFFloat getStop2Bounce() const { return _stop2Bounce; }

  /**
   * @brief Sets the value of stop2Bounce. AccessType: inputOutput
   * @details stop2Bounce is velocity factor for bounce back once stop point is
   * reached.
   * @param value The new value for stop2Bounce.
   */
  void setStop2Bounce(const SFFloat &value) { _stop2Bounce = value; }

  /**
   * @brief Gets the value of stop2ErrorCorrection. AccessType: inputOutput
   * @details stop2ErrorCorrection is fraction of error correction performed
   * during time step once stop point is reached.
   * @return SFFloat The current value of stop2ErrorCorrection.
   */
  SFFloat getStop2ErrorCorrection() const { return _stop2ErrorCorrection; }

  /**
   * @brief Sets the value of stop2ErrorCorrection. AccessType: inputOutput
   * @details stop2ErrorCorrection is fraction of error correction performed
   * during time step once stop point is reached.
   * @param value The new value for stop2ErrorCorrection.
   */
  void setStop2ErrorCorrection(const SFFloat &value) {

    _stop2ErrorCorrection = value;
  }

  /**
   * @brief Gets the value of stop3Bounce. AccessType: inputOutput
   * @details stop3Bounce is velocity factor for bounce back once stop point is
   * reached.
   * @return SFFloat The current value of stop3Bounce.
   */
  SFFloat getStop3Bounce() const { return _stop3Bounce; }

  /**
   * @brief Sets the value of stop3Bounce. AccessType: inputOutput
   * @details stop3Bounce is velocity factor for bounce back once stop point is
   * reached.
   * @param value The new value for stop3Bounce.
   */
  void setStop3Bounce(const SFFloat &value) { _stop3Bounce = value; }

  /**
   * @brief Gets the value of stop3ErrorCorrection. AccessType: inputOutput
   * @details stop3ErrorCorrection is fraction of error correction performed
   * during time step once stop point is reached.
   * @return SFFloat The current value of stop3ErrorCorrection.
   */
  SFFloat getStop3ErrorCorrection() const { return _stop3ErrorCorrection; }

  /**
   * @brief Sets the value of stop3ErrorCorrection. AccessType: inputOutput
   * @details stop3ErrorCorrection is fraction of error correction performed
   * during time step once stop point is reached.
   * @param value The new value for stop3ErrorCorrection.
   */
  void setStop3ErrorCorrection(const SFFloat &value) {

    _stop3ErrorCorrection = value;
  }

  /**
   * @brief The X3D type name of this node (e.g. "MotorJoint").
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
  static void checkRangesEnabledAxes(const SFInt32 &value,
                                     const std::string &nodeType,
                                     const std::string &defName,
                                     std::vector<RangeDiagnostic> &out);

private:
  static void validateEnabledAxes(const SFInt32 &value) {

    if (value < 0)
      throw std::out_of_range("enabledAxes below minimum of 0");
    if (value > 3)
      throw std::out_of_range("enabledAxes above maximum of 3");
  }

  /**
   * @brief Member variable for autoCalc.
   */

  SFBool _autoCalc{false};

  /**
   * @brief Member variable for axis1Angle.
   */

  SFFloat _axis1Angle{0};

  /**
   * @brief Member variable for axis1Torque.
   */

  SFFloat _axis1Torque{0};

  /**
   * @brief Member variable for axis2Angle.
   */

  SFFloat _axis2Angle{0};

  /**
   * @brief Member variable for axis2Torque.
   */

  SFFloat _axis2Torque{0};

  /**
   * @brief Member variable for axis3Angle.
   */

  SFFloat _axis3Angle{0};

  /**
   * @brief Member variable for axis3Torque.
   */

  SFFloat _axis3Torque{0};

  /**
   * @brief Member variable for enabledAxes.
   */

  SFInt32 _enabledAxes{1};

  /**
   * @brief Member variable for motor1Angle.
   */

  SFFloat _motor1Angle{};

  /**
   * @brief Member variable for motor1AngleRate.
   */

  SFFloat _motor1AngleRate{};

  /**
   * @brief Member variable for motor1Axis.
   */

  SFVec3f _motor1Axis{SFVec3f{1, 0, 0}};

  /**
   * @brief Member variable for motor2Angle.
   */

  SFFloat _motor2Angle{};

  /**
   * @brief Member variable for motor2AngleRate.
   */

  SFFloat _motor2AngleRate{};

  /**
   * @brief Member variable for motor2Axis.
   */

  SFVec3f _motor2Axis{SFVec3f{0, 1, 0}};

  /**
   * @brief Member variable for motor3Angle.
   */

  SFFloat _motor3Angle{};

  /**
   * @brief Member variable for motor3AngleRate.
   */

  SFFloat _motor3AngleRate{};

  /**
   * @brief Member variable for motor3Axis.
   */

  SFVec3f _motor3Axis{SFVec3f{0, 0, 1}};

  /**
   * @brief Member variable for stop1Bounce.
   */

  SFFloat _stop1Bounce{0};

  /**
   * @brief Member variable for stop1ErrorCorrection.
   */

  SFFloat _stop1ErrorCorrection{0.8};

  /**
   * @brief Member variable for stop2Bounce.
   */

  SFFloat _stop2Bounce{0};

  /**
   * @brief Member variable for stop2ErrorCorrection.
   */

  SFFloat _stop2ErrorCorrection{0.8};

  /**
   * @brief Member variable for stop3Bounce.
   */

  SFFloat _stop3Bounce{0};

  /**
   * @brief Member variable for stop3ErrorCorrection.
   */

  SFFloat _stop3ErrorCorrection{0.8};
};

} // namespace x3d::nodes
