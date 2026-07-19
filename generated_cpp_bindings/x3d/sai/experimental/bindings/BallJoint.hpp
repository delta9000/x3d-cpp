#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct BallJoint {
  static constexpr std::string_view x3d_name = "BallJoint";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
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
  inline static constexpr std::array<field_key_descriptor, 13> field_keys{{
      {anchorPoint.name(), anchorPoint.kind, anchorPoint.access()},
      {body1.name(), body1.kind, body1.access()},
      {body1AnchorPoint.name(), body1AnchorPoint.kind,
       body1AnchorPoint.access()},
      {body2.name(), body2.kind, body2.access()},
      {body2AnchorPoint.name(), body2AnchorPoint.kind,
       body2AnchorPoint.access()},
      {forceOutput.name(), forceOutput.kind, forceOutput.access()},
      {IS.name(), IS.kind, IS.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
