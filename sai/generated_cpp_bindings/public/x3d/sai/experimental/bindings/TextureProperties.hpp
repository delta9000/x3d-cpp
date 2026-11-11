#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct TextureProperties {
  static constexpr std::string_view x3d_name = "TextureProperties";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<TextureProperties, float> anisotropicDegree{
      "anisotropicDegree", access_type::input_output};
  inline static constexpr field_key<TextureProperties,
                                    ::x3d::sai::experimental::color4f>
      borderColor{"borderColor", access_type::input_output};
  inline static constexpr field_key<TextureProperties, std::int32_t>
      borderWidth{"borderWidth", access_type::input_output};
  inline static constexpr field_key<TextureProperties, std::string>
      boundaryModeR{"boundaryModeR", access_type::input_output};
  inline static constexpr field_key<TextureProperties, std::string>
      boundaryModeS{"boundaryModeS", access_type::input_output};
  inline static constexpr field_key<TextureProperties, std::string>
      boundaryModeT{"boundaryModeT", access_type::input_output};
  inline static constexpr field_key<TextureProperties, bool> generateMipMaps{
      "generateMipMaps", access_type::initialize_only};
  inline static constexpr field_key<TextureProperties, std::string>
      magnificationFilter{"magnificationFilter", access_type::input_output};
  inline static constexpr field_key<TextureProperties,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<TextureProperties, std::string>
      minificationFilter{"minificationFilter", access_type::input_output};
  inline static constexpr field_key<TextureProperties, std::string>
      textureCompression{"textureCompression", access_type::input_output};
  inline static constexpr field_key<TextureProperties, float> texturePriority{
      "texturePriority", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 12> field_keys{{
      {anisotropicDegree.name(), anisotropicDegree.kind,
       anisotropicDegree.access()},
      {borderColor.name(), borderColor.kind, borderColor.access()},
      {borderWidth.name(), borderWidth.kind, borderWidth.access()},
      {boundaryModeR.name(), boundaryModeR.kind, boundaryModeR.access()},
      {boundaryModeS.name(), boundaryModeS.kind, boundaryModeS.access()},
      {boundaryModeT.name(), boundaryModeT.kind, boundaryModeT.access()},
      {generateMipMaps.name(), generateMipMaps.kind, generateMipMaps.access()},
      {magnificationFilter.name(), magnificationFilter.kind,
       magnificationFilter.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {minificationFilter.name(), minificationFilter.kind,
       minificationFilter.access()},
      {textureCompression.name(), textureCompression.kind,
       textureCompression.access()},
      {texturePriority.name(), texturePriority.kind, texturePriority.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
