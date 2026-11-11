#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DTouchSensorNode {
  static constexpr std::string_view x3d_name = "X3DTouchSensorNode";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<X3DTouchSensorNode, std::string>
      description{"description", access_type::input_output};
  inline static constexpr field_key<X3DTouchSensorNode, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<X3DTouchSensorNode, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<X3DTouchSensorNode, bool> isOver{
      "isOver", access_type::output_only};
  inline static constexpr field_key<X3DTouchSensorNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<X3DTouchSensorNode,
                                    ::x3d::sai::experimental::time_value>
      touchTime{"touchTime", access_type::output_only};
  inline static constexpr std::array<field_key_descriptor, 6> field_keys{{
      {description.name(), description.kind, description.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {isActive.name(), isActive.kind, isActive.access()},
      {isOver.name(), isOver.kind, isOver.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {touchTime.name(), touchTime.kind, touchTime.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
