#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DSensorNode {
  static constexpr std::string_view x3d_name = "X3DSensorNode";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<X3DSensorNode, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<X3DSensorNode, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<X3DSensorNode, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<X3DSensorNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 4> field_keys{{
      {description.name(), description.kind, description.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {isActive.name(), isActive.kind, isActive.access()},
      {metadata.name(), metadata.kind, metadata.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
