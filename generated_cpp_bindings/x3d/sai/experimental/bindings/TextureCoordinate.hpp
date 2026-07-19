#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct TextureCoordinate {
  static constexpr std::string_view x3d_name = "TextureCoordinate";
  inline static constexpr field_key<TextureCoordinate,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<TextureCoordinate, std::string> mapping{
      "mapping", access_type::input_output};
  inline static constexpr field_key<TextureCoordinate,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<TextureCoordinate,
                                    ::x3d::sai::experimental::vec2f_list>
      point{"point", access_type::input_output};
  inline static constexpr field_key<TextureCoordinate, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<TextureCoordinate, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<TextureCoordinate, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<TextureCoordinate, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<TextureCoordinate, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
