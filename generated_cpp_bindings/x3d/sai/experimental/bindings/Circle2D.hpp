#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct Circle2D {
  static constexpr std::string_view x3d_name = "Circle2D";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<Circle2D, ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<Circle2D, float> radius{
      "radius", access_type::initialize_only};
  inline static constexpr std::array<field_key_descriptor, 2> field_keys{{
      {metadata.name(), metadata.kind, metadata.access()},
      {radius.name(), radius.kind, radius.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
