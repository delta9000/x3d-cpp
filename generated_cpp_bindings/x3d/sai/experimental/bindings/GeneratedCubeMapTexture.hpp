#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct GeneratedCubeMapTexture {
  static constexpr std::string_view x3d_name = "GeneratedCubeMapTexture";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<GeneratedCubeMapTexture, std::string>
      description{"description", access_type::input_output};
  inline static constexpr field_key<GeneratedCubeMapTexture,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<GeneratedCubeMapTexture, std::int32_t> size{
      "size", access_type::initialize_only};
  inline static constexpr field_key<GeneratedCubeMapTexture,
                                    ::x3d::sai::experimental::node_id>
      textureProperties{"textureProperties", access_type::initialize_only};
  inline static constexpr field_key<GeneratedCubeMapTexture, std::string>
      update{"update", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 5> field_keys{{
      {description.name(), description.kind, description.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {size.name(), size.kind, size.access()},
      {textureProperties.name(), textureProperties.kind,
       textureProperties.access()},
      {update.name(), update.kind, update.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
