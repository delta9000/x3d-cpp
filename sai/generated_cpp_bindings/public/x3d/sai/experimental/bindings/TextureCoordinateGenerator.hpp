#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct TextureCoordinateGenerator {
  static constexpr std::string_view x3d_name = "TextureCoordinateGenerator";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<TextureCoordinateGenerator, std::string>
      mapping{"mapping", access_type::input_output};
  inline static constexpr field_key<TextureCoordinateGenerator,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<TextureCoordinateGenerator, std::string>
      mode{"mode", access_type::input_output};
  inline static constexpr field_key<TextureCoordinateGenerator,
                                    ::x3d::sai::experimental::float_list>
      parameter{"parameter", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 4> field_keys{{
      {mapping.name(), mapping.kind, mapping.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {mode.name(), mode.kind, mode.access()},
      {parameter.name(), parameter.kind, parameter.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
