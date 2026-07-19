#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct SingleAxisHingeJoint {
  static constexpr std::string_view x3d_name = "SingleAxisHingeJoint";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<SingleAxisHingeJoint,
                                    ::x3d::sai::experimental::vec3f>
      anchorPoint{"anchorPoint", access_type::input_output};
  inline static constexpr field_key<SingleAxisHingeJoint, float> angle{
      "angle", access_type::output_only};
  inline static constexpr field_key<SingleAxisHingeJoint, float> angleRate{
      "angleRate", access_type::output_only};
  inline static constexpr field_key<SingleAxisHingeJoint,
                                    ::x3d::sai::experimental::vec3f>
      axis{"axis", access_type::input_output};
  inline static constexpr field_key<SingleAxisHingeJoint,
                                    ::x3d::sai::experimental::node_id>
      body1{"body1", access_type::input_output};
  inline static constexpr field_key<SingleAxisHingeJoint,
                                    ::x3d::sai::experimental::vec3f>
      body1AnchorPoint{"body1AnchorPoint", access_type::output_only};
  inline static constexpr field_key<SingleAxisHingeJoint,
                                    ::x3d::sai::experimental::node_id>
      body2{"body2", access_type::input_output};
  inline static constexpr field_key<SingleAxisHingeJoint,
                                    ::x3d::sai::experimental::vec3f>
      body2AnchorPoint{"body2AnchorPoint", access_type::output_only};
  inline static constexpr field_key<SingleAxisHingeJoint,
                                    ::x3d::sai::experimental::string_list>
      forceOutput{"forceOutput", access_type::input_output};
  inline static constexpr field_key<SingleAxisHingeJoint,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<SingleAxisHingeJoint, float> maxAngle{
      "maxAngle", access_type::input_output};
  inline static constexpr field_key<SingleAxisHingeJoint,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<SingleAxisHingeJoint, float> minAngle{
      "minAngle", access_type::input_output};
  inline static constexpr field_key<SingleAxisHingeJoint, float> stopBounce{
      "stopBounce", access_type::input_output};
  inline static constexpr field_key<SingleAxisHingeJoint, float>
      stopErrorCorrection{"stopErrorCorrection", access_type::input_output};
  inline static constexpr field_key<SingleAxisHingeJoint, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<SingleAxisHingeJoint, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<SingleAxisHingeJoint, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<SingleAxisHingeJoint, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<SingleAxisHingeJoint, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 20> field_keys{{
      {anchorPoint.name(), anchorPoint.kind, anchorPoint.access()},
      {angle.name(), angle.kind, angle.access()},
      {angleRate.name(), angleRate.kind, angleRate.access()},
      {axis.name(), axis.kind, axis.access()},
      {body1.name(), body1.kind, body1.access()},
      {body1AnchorPoint.name(), body1AnchorPoint.kind,
       body1AnchorPoint.access()},
      {body2.name(), body2.kind, body2.access()},
      {body2AnchorPoint.name(), body2AnchorPoint.kind,
       body2AnchorPoint.access()},
      {forceOutput.name(), forceOutput.kind, forceOutput.access()},
      {IS.name(), IS.kind, IS.access()},
      {maxAngle.name(), maxAngle.kind, maxAngle.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {minAngle.name(), minAngle.kind, minAngle.access()},
      {stopBounce.name(), stopBounce.kind, stopBounce.access()},
      {stopErrorCorrection.name(), stopErrorCorrection.kind,
       stopErrorCorrection.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
