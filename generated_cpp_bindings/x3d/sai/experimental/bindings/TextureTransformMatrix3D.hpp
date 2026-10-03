#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct TextureTransformMatrix3D {
  static constexpr std::string_view x3d_name = "TextureTransformMatrix3D";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<TextureTransformMatrix3D, std::string>
      mapping{"mapping", access_type::input_output};
  inline static constexpr field_key<TextureTransformMatrix3D,
                                    ::x3d::sai::experimental::matrix4f>
      matrix{"matrix", access_type::input_output};
  inline static constexpr field_key<TextureTransformMatrix3D,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 3> field_keys{{
      {mapping.name(), mapping.kind, mapping.access()},
      {matrix.name(), matrix.kind, matrix.access()},
      {metadata.name(), metadata.kind, metadata.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
