#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct TextureTransform {
  static constexpr std::string_view x3d_name = "TextureTransform";
  inline static constexpr field_key<TextureTransform,
                                    ::x3d::sai::experimental::vec2f>
      center{"center", access_type::input_output};
  inline static constexpr field_key<TextureTransform,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<TextureTransform, std::string> mapping{
      "mapping", access_type::input_output};
  inline static constexpr field_key<TextureTransform,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<TextureTransform, float> rotation{
      "rotation", access_type::input_output};
  inline static constexpr field_key<TextureTransform,
                                    ::x3d::sai::experimental::vec2f>
      scale{"scale", access_type::input_output};
  inline static constexpr field_key<TextureTransform,
                                    ::x3d::sai::experimental::vec2f>
      translation{"translation", access_type::input_output};
  inline static constexpr field_key<TextureTransform, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<TextureTransform, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<TextureTransform, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<TextureTransform, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<TextureTransform, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
