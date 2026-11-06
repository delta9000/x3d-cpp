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
  MotorJoint() = default;

  ~MotorJoint() = default;

  static SFBool getDefaultAutoCalc() { return false; }

  static SFFloat getDefaultAxis1Angle() { return 0; }

  static SFFloat getDefaultAxis1Torque() { return 0; }

  static SFFloat getDefaultAxis2Angle() { return 0; }

  static SFFloat getDefaultAxis2Torque() { return 0; }

  static SFFloat getDefaultAxis3Angle() { return 0; }

  static SFFloat getDefaultAxis3Torque() { return 0; }

  static SFInt32 getDefaultEnabledAxes() { return 1; }

  static SFVec3f getDefaultMotor1Axis() { return SFVec3f{1, 0, 0}; }

  static SFVec3f getDefaultMotor2Axis() { return SFVec3f{0, 1, 0}; }

  static SFVec3f getDefaultMotor3Axis() { return SFVec3f{0, 0, 1}; }

  static SFFloat getDefaultStop1Bounce() { return 0; }

  static SFFloat getDefaultStop1ErrorCorrection() { return 0.8; }

  static SFFloat getDefaultStop2Bounce() { return 0; }

  static SFFloat getDefaultStop2ErrorCorrection() { return 0.8; }

  static SFFloat getDefaultStop3Bounce() { return 0; }

  static SFFloat getDefaultStop3ErrorCorrection() { return 0.8; }

  static std::string getContainerFieldType() { return "xs:NMTOKEN"; }

  static std::string getDefaultContainerField() { return "joints"; }

  static std::string componentName() { return "RigidBodyPhysics"; }

  static int componentLevel() { return 2; }

  /**
   * SFBool; access: initializeOnly. Borrows the stored value.
   * @details autoCalc controls whether user manually provides individual angle
   * rotations each frame (false) or if angle values are automatically
   * calculated by motor implementations (true).
   */
  const SFBool &getAutoCalc() const { return _autoCalc; }
  /**
   * initializeOnly fields can be written at parse time, but not by runtime
   * events.
   */
  void setAutoCalcUnchecked(const SFBool &value) { _autoCalc = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details axis1Angle (radians) is rotation angle for corresponding motor
   * axis when in user-calculated mode.
   */
  const SFFloat &getAxis1Angle() const { return _axis1Angle; }

  /**
   * Access: inputOutput.
   * @details axis1Angle (radians) is rotation angle for corresponding motor
   * axis when in user-calculated mode.
   */
  void setAxis1Angle(const SFFloat &value) { _axis1Angle = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details axis1Torque is rotational torque applied by corresponding motor
   * axis when in user-calculated mode.
   */
  const SFFloat &getAxis1Torque() const { return _axis1Torque; }

  /**
   * Access: inputOutput.
   * @details axis1Torque is rotational torque applied by corresponding motor
   * axis when in user-calculated mode.
   */
  void setAxis1Torque(const SFFloat &value) { _axis1Torque = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details axis2Angle (radians) is rotation angle for corresponding motor
   * axis when in user-calculated mode.
   */
  const SFFloat &getAxis2Angle() const { return _axis2Angle; }

  /**
   * Access: inputOutput.
   * @details axis2Angle (radians) is rotation angle for corresponding motor
   * axis when in user-calculated mode.
   */
  void setAxis2Angle(const SFFloat &value) { _axis2Angle = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details axis2Torque is rotational torque applied by corresponding motor
   * axis when in user-calculated mode.
   */
  const SFFloat &getAxis2Torque() const { return _axis2Torque; }

  /**
   * Access: inputOutput.
   * @details axis2Torque is rotational torque applied by corresponding motor
   * axis when in user-calculated mode.
   */
  void setAxis2Torque(const SFFloat &value) { _axis2Torque = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details axis3Angle (radians) is rotation angle for corresponding motor
   * axis when in user-calculated mode.
   */
  const SFFloat &getAxis3Angle() const { return _axis3Angle; }

  /**
   * Access: inputOutput.
   * @details axis3Angle (radians) is rotation angle for corresponding motor
   * axis when in user-calculated mode.
   */
  void setAxis3Angle(const SFFloat &value) { _axis3Angle = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details axis3Torque is rotational torque applied by corresponding motor
   * axis when in user-calculated mode.
   */
  const SFFloat &getAxis3Torque() const { return _axis3Torque; }

  /**
   * Access: inputOutput.
   * @details axis3Torque is rotational torque applied by corresponding motor
   * axis when in user-calculated mode.
   */
  void setAxis3Torque(const SFFloat &value) { _axis3Torque = value; }

  /**
   * SFInt32; access: inputOutput. Borrows the stored value.
   * @details enabledAxes indicates which motor axes are active.
   */
  const SFInt32 &getEnabledAxes() const { return _enabledAxes; }

  /**
   * Access: inputOutput.
   * @details enabledAxes indicates which motor axes are active.
   */
  void setEnabledAxes(const SFInt32 &value) {

    validateEnabledAxes(value);

    _enabledAxes = value;
  }

  /**
   * Preserves out-of-range authored values during reader ingest.
   * Programmatic callers use setEnabledAxes() for range validation.
   */
  void setEnabledAxesUnchecked(const SFInt32 &value) { _enabledAxes = value; }

  /**
   * SFFloat; access: outputOnly. Borrows the stored value.
   * @details motor1Angle provides calculated angle of rotation (radians) for
   * this motor joint from last frame.
   */
  const SFFloat &getMotor1Angle() const { return _motor1Angle; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details motor1Angle provides calculated angle of rotation (radians) for
   * this motor joint from last frame.
   */
  void emitMotor1Angle(const SFFloat &value) { _motor1Angle = value; }

  /**
   * SFFloat; access: outputOnly. Borrows the stored value.
   * @details motor1AngleRate provides calculated anglular rotation rate
   * (radians/second) for this motor joint from last frame.
   */
  const SFFloat &getMotor1AngleRate() const { return _motor1AngleRate; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details motor1AngleRate provides calculated anglular rotation rate
   * (radians/second) for this motor joint from last frame.
   */
  void emitMotor1AngleRate(const SFFloat &value) { _motor1AngleRate = value; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details motor1Axis defines axis vector of corresponding motor axis.
   */
  const SFVec3f &getMotor1Axis() const { return _motor1Axis; }

  /**
   * Access: inputOutput.
   * @details motor1Axis defines axis vector of corresponding motor axis.
   */
  void setMotor1Axis(const SFVec3f &value) { _motor1Axis = value; }

  void setMotor1Axis(SFVec3f &&value) { _motor1Axis = std::move(value); }

  /**
   * SFFloat; access: outputOnly. Borrows the stored value.
   * @details motor2Angle provides calculated angle of rotation (radians) for
   * this motor joint from last frame.
   */
  const SFFloat &getMotor2Angle() const { return _motor2Angle; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details motor2Angle provides calculated angle of rotation (radians) for
   * this motor joint from last frame.
   */
  void emitMotor2Angle(const SFFloat &value) { _motor2Angle = value; }

  /**
   * SFFloat; access: outputOnly. Borrows the stored value.
   * @details motor2AngleRate provides calculated anglular rotation rate
   * (radians/second) for this motor joint from last frame.
   */
  const SFFloat &getMotor2AngleRate() const { return _motor2AngleRate; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details motor2AngleRate provides calculated anglular rotation rate
   * (radians/second) for this motor joint from last frame.
   */
  void emitMotor2AngleRate(const SFFloat &value) { _motor2AngleRate = value; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details motor2Axis defines axis vector of corresponding motor axis.
   */
  const SFVec3f &getMotor2Axis() const { return _motor2Axis; }

  /**
   * Access: inputOutput.
   * @details motor2Axis defines axis vector of corresponding motor axis.
   */
  void setMotor2Axis(const SFVec3f &value) { _motor2Axis = value; }

  void setMotor2Axis(SFVec3f &&value) { _motor2Axis = std::move(value); }

  /**
   * SFFloat; access: outputOnly. Borrows the stored value.
   * @details motor3Angle provides calculated angle of rotation (radians) for
   * this motor joint from last frame.
   */
  const SFFloat &getMotor3Angle() const { return _motor3Angle; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details motor3Angle provides calculated angle of rotation (radians) for
   * this motor joint from last frame.
   */
  void emitMotor3Angle(const SFFloat &value) { _motor3Angle = value; }

  /**
   * SFFloat; access: outputOnly. Borrows the stored value.
   * @details motor3AngleRate provides calculated anglular rotation rate
   * (radians/second) for this motor joint from last frame.
   */
  const SFFloat &getMotor3AngleRate() const { return _motor3AngleRate; }

  /**
   * Access: outputOnly. Runtime output write path.
   * @details motor3AngleRate provides calculated anglular rotation rate
   * (radians/second) for this motor joint from last frame.
   */
  void emitMotor3AngleRate(const SFFloat &value) { _motor3AngleRate = value; }

  /**
   * SFVec3f; access: inputOutput. Borrows the stored value.
   * @details motor3Axis defines axis vector of corresponding motor axis.
   */
  const SFVec3f &getMotor3Axis() const { return _motor3Axis; }

  /**
   * Access: inputOutput.
   * @details motor3Axis defines axis vector of corresponding motor axis.
   */
  void setMotor3Axis(const SFVec3f &value) { _motor3Axis = value; }

  void setMotor3Axis(SFVec3f &&value) { _motor3Axis = std::move(value); }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details stop1Bounce is velocity factor for bounce back once stop point is
   * reached.
   */
  const SFFloat &getStop1Bounce() const { return _stop1Bounce; }

  /**
   * Access: inputOutput.
   * @details stop1Bounce is velocity factor for bounce back once stop point is
   * reached.
   */
  void setStop1Bounce(const SFFloat &value) { _stop1Bounce = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details stop1ErrorCorrection is fraction of error correction performed
   * during time step once stop point is reached.
   */
  const SFFloat &getStop1ErrorCorrection() const {
    return _stop1ErrorCorrection;
  }

  /**
   * Access: inputOutput.
   * @details stop1ErrorCorrection is fraction of error correction performed
   * during time step once stop point is reached.
   */
  void setStop1ErrorCorrection(const SFFloat &value) {

    _stop1ErrorCorrection = value;
  }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details stop2Bounce is velocity factor for bounce back once stop point is
   * reached.
   */
  const SFFloat &getStop2Bounce() const { return _stop2Bounce; }

  /**
   * Access: inputOutput.
   * @details stop2Bounce is velocity factor for bounce back once stop point is
   * reached.
   */
  void setStop2Bounce(const SFFloat &value) { _stop2Bounce = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details stop2ErrorCorrection is fraction of error correction performed
   * during time step once stop point is reached.
   */
  const SFFloat &getStop2ErrorCorrection() const {
    return _stop2ErrorCorrection;
  }

  /**
   * Access: inputOutput.
   * @details stop2ErrorCorrection is fraction of error correction performed
   * during time step once stop point is reached.
   */
  void setStop2ErrorCorrection(const SFFloat &value) {

    _stop2ErrorCorrection = value;
  }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details stop3Bounce is velocity factor for bounce back once stop point is
   * reached.
   */
  const SFFloat &getStop3Bounce() const { return _stop3Bounce; }

  /**
   * Access: inputOutput.
   * @details stop3Bounce is velocity factor for bounce back once stop point is
   * reached.
   */
  void setStop3Bounce(const SFFloat &value) { _stop3Bounce = value; }

  /**
   * SFFloat; access: inputOutput. Borrows the stored value.
   * @details stop3ErrorCorrection is fraction of error correction performed
   * during time step once stop point is reached.
   */
  const SFFloat &getStop3ErrorCorrection() const {
    return _stop3ErrorCorrection;
  }

  /**
   * Access: inputOutput.
   * @details stop3ErrorCorrection is fraction of error correction performed
   * during time step once stop point is reached.
   */
  void setStop3ErrorCorrection(const SFFloat &value) {

    _stop3ErrorCorrection = value;
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

  SFBool _autoCalc{false};

  SFFloat _axis1Angle{0};

  SFFloat _axis1Torque{0};

  SFFloat _axis2Angle{0};

  SFFloat _axis2Torque{0};

  SFFloat _axis3Angle{0};

  SFFloat _axis3Torque{0};

  SFInt32 _enabledAxes{1};

  SFFloat _motor1Angle{};

  SFFloat _motor1AngleRate{};

  SFVec3f _motor1Axis{SFVec3f{1, 0, 0}};

  SFFloat _motor2Angle{};

  SFFloat _motor2AngleRate{};

  SFVec3f _motor2Axis{SFVec3f{0, 1, 0}};

  SFFloat _motor3Angle{};

  SFFloat _motor3AngleRate{};

  SFVec3f _motor3Axis{SFVec3f{0, 0, 1}};

  SFFloat _stop1Bounce{0};

  SFFloat _stop1ErrorCorrection{0.8};

  SFFloat _stop2Bounce{0};

  SFFloat _stop2ErrorCorrection{0.8};

  SFFloat _stop3Bounce{0};

  SFFloat _stop3ErrorCorrection{0.8};
};

} // namespace x3d::nodes
