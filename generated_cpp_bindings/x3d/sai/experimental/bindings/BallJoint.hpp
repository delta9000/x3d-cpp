#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct BallJoint {
  static constexpr std::string_view x3d_name = "BallJoint";
  inline static constexpr field_key<BallJoint, ::x3d::sai::experimental::vec3f>
      anchorPoint{"anchorPoint", access_type::input_output};
  inline static constexpr field_key<BallJoint,
                                    ::x3d::sai::experimental::node_id>
      body1{"body1", access_type::input_output};
  inline static constexpr field_key<BallJoint, ::x3d::sai::experimental::vec3f>
      body1AnchorPoint{"body1AnchorPoint", access_type::output_only};
  inline static constexpr field_key<BallJoint,
                                    ::x3d::sai::experimental::node_id>
      body2{"body2", access_type::input_output};
  inline static constexpr field_key<BallJoint, ::x3d::sai::experimental::vec3f>
      body2AnchorPoint{"body2AnchorPoint", access_type::output_only};
  inline static constexpr field_key<BallJoint,
                                    ::x3d::sai::experimental::string_list>
      forceOutput{"forceOutput", access_type::input_output};
  inline static constexpr field_key<BallJoint,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<BallJoint,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<BallJoint, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<BallJoint, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<BallJoint, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<BallJoint, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<BallJoint, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
