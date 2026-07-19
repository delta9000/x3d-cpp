#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct MotorJoint {
  static constexpr std::string_view x3d_name = "MotorJoint";
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
};

} // namespace x3d::sai::experimental::bindings
