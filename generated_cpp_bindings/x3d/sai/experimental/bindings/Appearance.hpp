#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct Appearance {
  static constexpr std::string_view x3d_name = "Appearance";
  inline static constexpr field_key<Appearance,
                                    ::x3d::sai::experimental::node_id>
      acousticProperties{"acousticProperties", access_type::input_output};
  inline static constexpr field_key<Appearance, float> alphaCutoff{
      "alphaCutoff", access_type::input_output};
  inline static constexpr field_key<Appearance,
                                    ::x3d::sai::experimental::enum_value>
      alphaMode{"alphaMode", access_type::input_output};
  inline static constexpr field_key<Appearance,
                                    ::x3d::sai::experimental::node_id>
      backMaterial{"backMaterial", access_type::input_output};
  inline static constexpr field_key<Appearance,
                                    ::x3d::sai::experimental::node_id>
      fillProperties{"fillProperties", access_type::input_output};
  inline static constexpr field_key<Appearance,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<Appearance,
                                    ::x3d::sai::experimental::node_id>
      lineProperties{"lineProperties", access_type::input_output};
  inline static constexpr field_key<Appearance,
                                    ::x3d::sai::experimental::node_id>
      material{"material", access_type::input_output};
  inline static constexpr field_key<Appearance,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<Appearance,
                                    ::x3d::sai::experimental::node_id>
      pointProperties{"pointProperties", access_type::input_output};
  inline static constexpr field_key<Appearance,
                                    ::x3d::sai::experimental::node_list>
      shaders{"shaders", access_type::input_output};
  inline static constexpr field_key<Appearance,
                                    ::x3d::sai::experimental::node_id>
      texture{"texture", access_type::input_output};
  inline static constexpr field_key<Appearance,
                                    ::x3d::sai::experimental::node_id>
      textureTransform{"textureTransform", access_type::input_output};
  inline static constexpr field_key<Appearance, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<Appearance, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<Appearance, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<Appearance, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<Appearance, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
