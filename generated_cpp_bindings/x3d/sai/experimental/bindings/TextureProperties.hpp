#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct TextureProperties {
  static constexpr std::string_view x3d_name = "TextureProperties";
  inline static constexpr field_key<TextureProperties, float> anisotropicDegree{
      "anisotropicDegree", access_type::input_output};
  inline static constexpr field_key<TextureProperties,
                                    ::x3d::sai::experimental::color4f>
      borderColor{"borderColor", access_type::input_output};
  inline static constexpr field_key<TextureProperties, std::int32_t>
      borderWidth{"borderWidth", access_type::input_output};
  inline static constexpr field_key<TextureProperties,
                                    ::x3d::sai::experimental::enum_value>
      boundaryModeR{"boundaryModeR", access_type::input_output};
  inline static constexpr field_key<TextureProperties,
                                    ::x3d::sai::experimental::enum_value>
      boundaryModeS{"boundaryModeS", access_type::input_output};
  inline static constexpr field_key<TextureProperties,
                                    ::x3d::sai::experimental::enum_value>
      boundaryModeT{"boundaryModeT", access_type::input_output};
  inline static constexpr field_key<TextureProperties, bool> generateMipMaps{
      "generateMipMaps", access_type::initialize_only};
  inline static constexpr field_key<TextureProperties,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<TextureProperties,
                                    ::x3d::sai::experimental::enum_value>
      magnificationFilter{"magnificationFilter", access_type::input_output};
  inline static constexpr field_key<TextureProperties,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<TextureProperties,
                                    ::x3d::sai::experimental::enum_value>
      minificationFilter{"minificationFilter", access_type::input_output};
  inline static constexpr field_key<TextureProperties,
                                    ::x3d::sai::experimental::enum_value>
      textureCompression{"textureCompression", access_type::input_output};
  inline static constexpr field_key<TextureProperties, float> texturePriority{
      "texturePriority", access_type::input_output};
  inline static constexpr field_key<TextureProperties, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<TextureProperties, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<TextureProperties, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<TextureProperties, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<TextureProperties, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
