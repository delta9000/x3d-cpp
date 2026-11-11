#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DViewpointNode {
  static constexpr std::string_view x3d_name = "X3DViewpointNode";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<X3DViewpointNode,
                                    ::x3d::sai::experimental::time_value>
      bindTime{"bindTime", access_type::output_only};
  inline static constexpr field_key<X3DViewpointNode, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<X3DViewpointNode, float> farDistance{
      "farDistance", access_type::input_output};
  inline static constexpr field_key<X3DViewpointNode, bool> isBound{
      "isBound", access_type::output_only};
  inline static constexpr field_key<X3DViewpointNode, bool> jump{
      "jump", access_type::input_output};
  inline static constexpr field_key<X3DViewpointNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<X3DViewpointNode,
                                    ::x3d::sai::experimental::node_id>
      navigationInfo{"navigationInfo", access_type::input_output};
  inline static constexpr field_key<X3DViewpointNode, float> nearDistance{
      "nearDistance", access_type::input_output};
  inline static constexpr field_key<X3DViewpointNode,
                                    ::x3d::sai::experimental::rotation>
      orientation{"orientation", access_type::input_output};
  inline static constexpr field_key<X3DViewpointNode, bool> retainUserOffsets{
      "retainUserOffsets", access_type::input_output};
  inline static constexpr field_key<X3DViewpointNode, bool> set_bind{
      "set_bind", access_type::input_only};
  inline static constexpr field_key<X3DViewpointNode, bool> viewAll{
      "viewAll", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 12> field_keys{{
      {bindTime.name(), bindTime.kind, bindTime.access()},
      {description.name(), description.kind, description.access()},
      {farDistance.name(), farDistance.kind, farDistance.access()},
      {isBound.name(), isBound.kind, isBound.access()},
      {jump.name(), jump.kind, jump.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {navigationInfo.name(), navigationInfo.kind, navigationInfo.access()},
      {nearDistance.name(), nearDistance.kind, nearDistance.access()},
      {orientation.name(), orientation.kind, orientation.access()},
      {retainUserOffsets.name(), retainUserOffsets.kind,
       retainUserOffsets.access()},
      {set_bind.name(), set_bind.kind, set_bind.access()},
      {viewAll.name(), viewAll.kind, viewAll.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
