#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct Rectangle2D {
  static constexpr std::string_view x3d_name = "Rectangle2D";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<Rectangle2D,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<Rectangle2D,
                                    ::x3d::sai::experimental::vec2f>
      size{"size", access_type::initialize_only};
  inline static constexpr field_key<Rectangle2D, bool> solid{
      "solid", access_type::initialize_only};
  inline static constexpr std::array<field_key_descriptor, 3> field_keys{{
      {metadata.name(), metadata.kind, metadata.access()},
      {size.name(), size.kind, size.access()},
      {solid.name(), solid.kind, solid.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
