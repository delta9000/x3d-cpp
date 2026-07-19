#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct MotorJoint {
  static constexpr std::string_view x3d_name = "MotorJoint";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<MotorJoint, bool> autoCalc{
      "autoCalc", access_type::initialize_only};
  inline static constexpr field_key<MotorJoint, float> axis1Angle{
      "axis1Angle", access_type::input_output};
  inline static constexpr field_key<MotorJoint, float> axis1Torque{
      "axis1Torque", access_type::input_output};
  inline static constexpr field_key<MotorJoint, float> axis2Angle{
      "axis2Angle", access_type::input_output};
  inline static constexpr field_key<MotorJoint, float> axis2Torque{
      "axis2Torque", access_type::input_output};
  inline static constexpr field_key<MotorJoint, float> axis3Angle{
      "axis3Angle", access_type::input_output};
  inline static constexpr field_key<MotorJoint, float> axis3Torque{
      "axis3Torque", access_type::input_output};
  inline static constexpr field_key<MotorJoint,
                                    ::x3d::sai::experimental::node_id>
      body1{"body1", access_type::input_output};
  inline static constexpr field_key<MotorJoint,
                                    ::x3d::sai::experimental::node_id>
      body2{"body2", access_type::input_output};
  inline static constexpr field_key<MotorJoint, std::int32_t> enabledAxes{
      "enabledAxes", access_type::input_output};
  inline static constexpr field_key<MotorJoint,
                                    ::x3d::sai::experimental::string_list>
      forceOutput{"forceOutput", access_type::input_output};
  inline static constexpr field_key<MotorJoint,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<MotorJoint,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<MotorJoint, float> motor1Angle{
      "motor1Angle", access_type::output_only};
  inline static constexpr field_key<MotorJoint, float> motor1AngleRate{
      "motor1AngleRate", access_type::output_only};
  inline static constexpr field_key<MotorJoint, ::x3d::sai::experimental::vec3f>
      motor1Axis{"motor1Axis", access_type::input_output};
  inline static constexpr field_key<MotorJoint, float> motor2Angle{
      "motor2Angle", access_type::output_only};
  inline static constexpr field_key<MotorJoint, float> motor2AngleRate{
      "motor2AngleRate", access_type::output_only};
  inline static constexpr field_key<MotorJoint, ::x3d::sai::experimental::vec3f>
      motor2Axis{"motor2Axis", access_type::input_output};
  inline static constexpr field_key<MotorJoint, float> motor3Angle{
      "motor3Angle", access_type::output_only};
  inline static constexpr field_key<MotorJoint, float> motor3AngleRate{
      "motor3AngleRate", access_type::output_only};
  inline static constexpr field_key<MotorJoint, ::x3d::sai::experimental::vec3f>
      motor3Axis{"motor3Axis", access_type::input_output};
  inline static constexpr field_key<MotorJoint, float> stop1Bounce{
      "stop1Bounce", access_type::input_output};
  inline static constexpr field_key<MotorJoint, float> stop1ErrorCorrection{
      "stop1ErrorCorrection", access_type::input_output};
  inline static constexpr field_key<MotorJoint, float> stop2Bounce{
      "stop2Bounce", access_type::input_output};
  inline static constexpr field_key<MotorJoint, float> stop2ErrorCorrection{
      "stop2ErrorCorrection", access_type::input_output};
  inline static constexpr field_key<MotorJoint, float> stop3Bounce{
      "stop3Bounce", access_type::input_output};
  inline static constexpr field_key<MotorJoint, float> stop3ErrorCorrection{
      "stop3ErrorCorrection", access_type::input_output};
  inline static constexpr field_key<MotorJoint, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<MotorJoint, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<MotorJoint, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<MotorJoint, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<MotorJoint, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 33> field_keys{{
      {autoCalc.name(), autoCalc.kind, autoCalc.access()},
      {axis1Angle.name(), axis1Angle.kind, axis1Angle.access()},
      {axis1Torque.name(), axis1Torque.kind, axis1Torque.access()},
      {axis2Angle.name(), axis2Angle.kind, axis2Angle.access()},
      {axis2Torque.name(), axis2Torque.kind, axis2Torque.access()},
      {axis3Angle.name(), axis3Angle.kind, axis3Angle.access()},
      {axis3Torque.name(), axis3Torque.kind, axis3Torque.access()},
      {body1.name(), body1.kind, body1.access()},
      {body2.name(), body2.kind, body2.access()},
      {enabledAxes.name(), enabledAxes.kind, enabledAxes.access()},
      {forceOutput.name(), forceOutput.kind, forceOutput.access()},
      {IS.name(), IS.kind, IS.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {motor1Angle.name(), motor1Angle.kind, motor1Angle.access()},
      {motor1AngleRate.name(), motor1AngleRate.kind, motor1AngleRate.access()},
      {motor1Axis.name(), motor1Axis.kind, motor1Axis.access()},
      {motor2Angle.name(), motor2Angle.kind, motor2Angle.access()},
      {motor2AngleRate.name(), motor2AngleRate.kind, motor2AngleRate.access()},
      {motor2Axis.name(), motor2Axis.kind, motor2Axis.access()},
      {motor3Angle.name(), motor3Angle.kind, motor3Angle.access()},
      {motor3AngleRate.name(), motor3AngleRate.kind, motor3AngleRate.access()},
      {motor3Axis.name(), motor3Axis.kind, motor3Axis.access()},
      {stop1Bounce.name(), stop1Bounce.kind, stop1Bounce.access()},
      {stop1ErrorCorrection.name(), stop1ErrorCorrection.kind,
       stop1ErrorCorrection.access()},
      {stop2Bounce.name(), stop2Bounce.kind, stop2Bounce.access()},
      {stop2ErrorCorrection.name(), stop2ErrorCorrection.kind,
       stop2ErrorCorrection.access()},
      {stop3Bounce.name(), stop3Bounce.kind, stop3Bounce.access()},
      {stop3ErrorCorrection.name(), stop3ErrorCorrection.kind,
       stop3ErrorCorrection.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
