#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct PixelTexture {
  static constexpr std::string_view x3d_name = "PixelTexture";
  inline static constexpr field_key<PixelTexture, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<PixelTexture,
                                    ::x3d::sai::experimental::image>
      image{"image", access_type::input_output};
  inline static constexpr field_key<PixelTexture,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<PixelTexture,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<PixelTexture, bool> repeatS{
      "repeatS", access_type::initialize_only};
  inline static constexpr field_key<PixelTexture, bool> repeatT{
      "repeatT", access_type::initialize_only};
  inline static constexpr field_key<PixelTexture,
                                    ::x3d::sai::experimental::node_id>
      textureProperties{"textureProperties", access_type::initialize_only};
  inline static constexpr field_key<PixelTexture, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<PixelTexture, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<PixelTexture, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<PixelTexture, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<PixelTexture, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
