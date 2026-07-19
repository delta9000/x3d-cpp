#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct DoubleAxisHingeJoint {
  static constexpr std::string_view x3d_name = "DoubleAxisHingeJoint";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
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
  inline static constexpr std::array<field_key_descriptor, 32> field_keys{{
      {anchorPoint.name(), anchorPoint.kind, anchorPoint.access()},
      {axis1.name(), axis1.kind, axis1.access()},
      {axis2.name(), axis2.kind, axis2.access()},
      {body1.name(), body1.kind, body1.access()},
      {body1AnchorPoint.name(), body1AnchorPoint.kind,
       body1AnchorPoint.access()},
      {body1Axis.name(), body1Axis.kind, body1Axis.access()},
      {body2.name(), body2.kind, body2.access()},
      {body2AnchorPoint.name(), body2AnchorPoint.kind,
       body2AnchorPoint.access()},
      {body2Axis.name(), body2Axis.kind, body2Axis.access()},
      {desiredAngularVelocity1.name(), desiredAngularVelocity1.kind,
       desiredAngularVelocity1.access()},
      {desiredAngularVelocity2.name(), desiredAngularVelocity2.kind,
       desiredAngularVelocity2.access()},
      {forceOutput.name(), forceOutput.kind, forceOutput.access()},
      {hinge1Angle.name(), hinge1Angle.kind, hinge1Angle.access()},
      {hinge1AngleRate.name(), hinge1AngleRate.kind, hinge1AngleRate.access()},
      {hinge2Angle.name(), hinge2Angle.kind, hinge2Angle.access()},
      {hinge2AngleRate.name(), hinge2AngleRate.kind, hinge2AngleRate.access()},
      {IS.name(), IS.kind, IS.access()},
      {maxAngle1.name(), maxAngle1.kind, maxAngle1.access()},
      {maxTorque1.name(), maxTorque1.kind, maxTorque1.access()},
      {maxTorque2.name(), maxTorque2.kind, maxTorque2.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {minAngle1.name(), minAngle1.kind, minAngle1.access()},
      {stop1Bounce.name(), stop1Bounce.kind, stop1Bounce.access()},
      {stop1ConstantForceMix.name(), stop1ConstantForceMix.kind,
       stop1ConstantForceMix.access()},
      {stop1ErrorCorrection.name(), stop1ErrorCorrection.kind,
       stop1ErrorCorrection.access()},
      {suspensionErrorCorrection.name(), suspensionErrorCorrection.kind,
       suspensionErrorCorrection.access()},
      {suspensionForce.name(), suspensionForce.kind, suspensionForce.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
