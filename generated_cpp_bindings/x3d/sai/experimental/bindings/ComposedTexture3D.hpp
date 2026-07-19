#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct ComposedTexture3D {
  static constexpr std::string_view x3d_name = "ComposedTexture3D";
  inline static constexpr field_key<ComposedTexture3D, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<ComposedTexture3D,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<ComposedTexture3D,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<ComposedTexture3D, bool> repeatR{
      "repeatR", access_type::initialize_only};
  inline static constexpr field_key<ComposedTexture3D, bool> repeatS{
      "repeatS", access_type::initialize_only};
  inline static constexpr field_key<ComposedTexture3D, bool> repeatT{
      "repeatT", access_type::initialize_only};
  inline static constexpr field_key<ComposedTexture3D,
                                    ::x3d::sai::experimental::node_list>
      texture{"texture", access_type::input_output};
  inline static constexpr field_key<ComposedTexture3D,
                                    ::x3d::sai::experimental::node_id>
      textureProperties{"textureProperties", access_type::initialize_only};
  inline static constexpr field_key<ComposedTexture3D, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<ComposedTexture3D, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<ComposedTexture3D, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<ComposedTexture3D, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<ComposedTexture3D, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
