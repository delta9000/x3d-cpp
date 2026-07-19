#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct SingleAxisHingeJoint {
  static constexpr std::string_view x3d_name = "SingleAxisHingeJoint";
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
};

} // namespace x3d::sai::experimental::bindings
