#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DSingleTextureCoordinateNode {
  static constexpr std::string_view x3d_name = "X3DSingleTextureCoordinateNode";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<X3DSingleTextureCoordinateNode, std::string>
      mapping{"mapping", access_type::input_output};
  inline static constexpr field_key<X3DSingleTextureCoordinateNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 2> field_keys{{
      {mapping.name(), mapping.kind, mapping.access()},
      {metadata.name(), metadata.kind, metadata.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
