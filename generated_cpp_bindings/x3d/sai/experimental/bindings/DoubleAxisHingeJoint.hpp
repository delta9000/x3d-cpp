#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct DoubleAxisHingeJoint {
  static constexpr std::string_view x3d_name = "DoubleAxisHingeJoint";
  inline static constexpr field_key<DoubleAxisHingeJoint,
                                    ::x3d::sai::experimental::vec3f>
      anchorPoint{"anchorPoint", access_type::input_output};
  inline static constexpr field_key<DoubleAxisHingeJoint,
                                    ::x3d::sai::experimental::vec3f>
      axis1{"axis1", access_type::input_output};
  inline static constexpr field_key<DoubleAxisHingeJoint,
                                    ::x3d::sai::experimental::vec3f>
      axis2{"axis2", access_type::input_output};
  inline static constexpr field_key<DoubleAxisHingeJoint,
                                    ::x3d::sai::experimental::node_id>
      body1{"body1", access_type::input_output};
  inline static constexpr field_key<DoubleAxisHingeJoint,
                                    ::x3d::sai::experimental::vec3f>
      body1AnchorPoint{"body1AnchorPoint", access_type::output_only};
  inline static constexpr field_key<DoubleAxisHingeJoint,
                                    ::x3d::sai::experimental::vec3f>
      body1Axis{"body1Axis", access_type::output_only};
  inline static constexpr field_key<DoubleAxisHingeJoint,
                                    ::x3d::sai::experimental::node_id>
      body2{"body2", access_type::input_output};
  inline static constexpr field_key<DoubleAxisHingeJoint,
                                    ::x3d::sai::experimental::vec3f>
      body2AnchorPoint{"body2AnchorPoint", access_type::output_only};
  inline static constexpr field_key<DoubleAxisHingeJoint,
                                    ::x3d::sai::experimental::vec3f>
      body2Axis{"body2Axis", access_type::output_only};
  inline static constexpr field_key<DoubleAxisHingeJoint, float>
      desiredAngularVelocity1{"desiredAngularVelocity1",
                              access_type::input_output};
  inline static constexpr field_key<DoubleAxisHingeJoint, float>
      desiredAngularVelocity2{"desiredAngularVelocity2",
                              access_type::input_output};
  inline static constexpr field_key<DoubleAxisHingeJoint,
                                    ::x3d::sai::experimental::string_list>
      forceOutput{"forceOutput", access_type::input_output};
  inline static constexpr field_key<DoubleAxisHingeJoint, float> hinge1Angle{
      "hinge1Angle", access_type::output_only};
  inline static constexpr field_key<DoubleAxisHingeJoint, float>
      hinge1AngleRate{"hinge1AngleRate", access_type::output_only};
  inline static constexpr field_key<DoubleAxisHingeJoint, float> hinge2Angle{
      "hinge2Angle", access_type::output_only};
  inline static constexpr field_key<DoubleAxisHingeJoint, float>
      hinge2AngleRate{"hinge2AngleRate", access_type::output_only};
  inline static constexpr field_key<DoubleAxisHingeJoint,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<DoubleAxisHingeJoint, float> maxAngle1{
      "maxAngle1", access_type::input_output};
  inline static constexpr field_key<DoubleAxisHingeJoint, float> maxTorque1{
      "maxTorque1", access_type::input_output};
  inline static constexpr field_key<DoubleAxisHingeJoint, float> maxTorque2{
      "maxTorque2", access_type::input_output};
  inline static constexpr field_key<DoubleAxisHingeJoint,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<DoubleAxisHingeJoint, float> minAngle1{
      "minAngle1", access_type::input_output};
  inline static constexpr field_key<DoubleAxisHingeJoint, float> stop1Bounce{
      "stop1Bounce", access_type::input_output};
  inline static constexpr field_key<DoubleAxisHingeJoint, float>
      stop1ConstantForceMix{"stop1ConstantForceMix", access_type::input_output};
  inline static constexpr field_key<DoubleAxisHingeJoint, float>
      stop1ErrorCorrection{"stop1ErrorCorrection", access_type::input_output};
  inline static constexpr field_key<DoubleAxisHingeJoint, float>
      suspensionErrorCorrection{"suspensionErrorCorrection",
                                access_type::input_output};
  inline static constexpr field_key<DoubleAxisHingeJoint, float>
      suspensionForce{"suspensionForce", access_type::input_output};
  inline static constexpr field_key<DoubleAxisHingeJoint, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<DoubleAxisHingeJoint, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<DoubleAxisHingeJoint, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<DoubleAxisHingeJoint, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<DoubleAxisHingeJoint, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
