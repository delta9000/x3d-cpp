#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct Box {
  static constexpr std::string_view x3d_name = "Box";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<Box, ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<Box, ::x3d::sai::experimental::vec3f> size{
      "size", access_type::initialize_only};
  inline static constexpr field_key<Box, bool> solid{
      "solid", access_type::initialize_only};
  inline static constexpr std::array<field_key_descriptor, 3> field_keys{{
      {metadata.name(), metadata.kind, metadata.access()},
      {size.name(), size.kind, size.access()},
      {solid.name(), solid.kind, solid.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
