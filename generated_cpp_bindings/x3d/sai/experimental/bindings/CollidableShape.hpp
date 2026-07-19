#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct CollidableShape {
  static constexpr std::string_view x3d_name = "CollidableShape";
  inline static constexpr field_key<CollidableShape,
                                    ::x3d::sai::experimental::vec3f>
      bboxCenter{"bboxCenter", access_type::initialize_only};
  inline static constexpr field_key<CollidableShape, bool> bboxDisplay{
      "bboxDisplay", access_type::input_output};
  inline static constexpr field_key<CollidableShape,
                                    ::x3d::sai::experimental::vec3f>
      bboxSize{"bboxSize", access_type::initialize_only};
  inline static constexpr field_key<CollidableShape, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<CollidableShape,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<CollidableShape,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<CollidableShape,
                                    ::x3d::sai::experimental::rotation>
      rotation{"rotation", access_type::input_output};
  inline static constexpr field_key<CollidableShape,
                                    ::x3d::sai::experimental::node_id>
      shape{"shape", access_type::initialize_only};
  inline static constexpr field_key<CollidableShape,
                                    ::x3d::sai::experimental::vec3f>
      translation{"translation", access_type::input_output};
  inline static constexpr field_key<CollidableShape, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr field_key<CollidableShape, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<CollidableShape, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<CollidableShape, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<CollidableShape, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<CollidableShape, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
