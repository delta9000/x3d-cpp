#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct TextureBackground {
  static constexpr std::string_view x3d_name = "TextureBackground";
  inline static constexpr field_key<TextureBackground,
                                    ::x3d::sai::experimental::node_id>
      backTexture{"backTexture", access_type::input_output};
  inline static constexpr field_key<TextureBackground,
                                    ::x3d::sai::experimental::time_value>
      bindTime{"bindTime", access_type::output_only};
  inline static constexpr field_key<TextureBackground,
                                    ::x3d::sai::experimental::node_id>
      bottomTexture{"bottomTexture", access_type::input_output};
  inline static constexpr field_key<TextureBackground,
                                    ::x3d::sai::experimental::node_id>
      frontTexture{"frontTexture", access_type::input_output};
  inline static constexpr field_key<TextureBackground,
                                    ::x3d::sai::experimental::float_list>
      groundAngle{"groundAngle", access_type::input_output};
  inline static constexpr field_key<TextureBackground,
                                    ::x3d::sai::experimental::color3f_list>
      groundColor{"groundColor", access_type::input_output};
  inline static constexpr field_key<TextureBackground,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<TextureBackground, bool> isBound{
      "isBound", access_type::output_only};
  inline static constexpr field_key<TextureBackground,
                                    ::x3d::sai::experimental::node_id>
      leftTexture{"leftTexture", access_type::input_output};
  inline static constexpr field_key<TextureBackground,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<TextureBackground,
                                    ::x3d::sai::experimental::node_id>
      rightTexture{"rightTexture", access_type::input_output};
  inline static constexpr field_key<TextureBackground, bool> set_bind{
      "set_bind", access_type::input_only};
  inline static constexpr field_key<TextureBackground,
                                    ::x3d::sai::experimental::float_list>
      skyAngle{"skyAngle", access_type::input_output};
  inline static constexpr field_key<TextureBackground,
                                    ::x3d::sai::experimental::color3f_list>
      skyColor{"skyColor", access_type::input_output};
  inline static constexpr field_key<TextureBackground,
                                    ::x3d::sai::experimental::node_id>
      topTexture{"topTexture", access_type::input_output};
  inline static constexpr field_key<TextureBackground, float> transparency{
      "transparency", access_type::input_output};
  inline static constexpr field_key<TextureBackground, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<TextureBackground, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<TextureBackground, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<TextureBackground, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<TextureBackground, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
