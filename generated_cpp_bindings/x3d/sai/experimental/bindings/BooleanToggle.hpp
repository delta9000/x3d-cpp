#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct BooleanToggle {
  static constexpr std::string_view x3d_name = "BooleanToggle";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<BooleanToggle,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<BooleanToggle, bool> set_boolean{
      "set_boolean", access_type::input_only};
  inline static constexpr field_key<BooleanToggle, bool> toggle{
      "toggle", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 3> field_keys{{
      {metadata.name(), metadata.kind, metadata.access()},
      {set_boolean.name(), set_boolean.kind, set_boolean.access()},
      {toggle.name(), toggle.kind, toggle.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
