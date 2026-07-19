#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct LOD {
  static constexpr std::string_view x3d_name = "LOD";
  inline static constexpr field_key<LOD, ::x3d::sai::experimental::node_list>
      addChildren{"addChildren", access_type::input_only};
  inline static constexpr field_key<LOD, ::x3d::sai::experimental::vec3f>
      bboxCenter{"bboxCenter", access_type::initialize_only};
  inline static constexpr field_key<LOD, bool> bboxDisplay{
      "bboxDisplay", access_type::input_output};
  inline static constexpr field_key<LOD, ::x3d::sai::experimental::vec3f>
      bboxSize{"bboxSize", access_type::initialize_only};
  inline static constexpr field_key<LOD, ::x3d::sai::experimental::vec3f>
      center{"center", access_type::initialize_only};
  inline static constexpr field_key<LOD, ::x3d::sai::experimental::node_list>
      children{"children", access_type::input_output};
  inline static constexpr field_key<LOD, bool> forceTransitions{
      "forceTransitions", access_type::initialize_only};
  inline static constexpr field_key<LOD, ::x3d::sai::experimental::node_id> IS{
      "IS", access_type::input_output};
  inline static constexpr field_key<LOD, std::int32_t> level_changed{
      "level_changed", access_type::output_only};
  inline static constexpr field_key<LOD, ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<LOD, ::x3d::sai::experimental::float_list>
      range{"range", access_type::initialize_only};
  inline static constexpr field_key<LOD, ::x3d::sai::experimental::node_list>
      removeChildren{"removeChildren", access_type::input_only};
  inline static constexpr field_key<LOD, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr field_key<LOD, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<LOD, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<LOD, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<LOD, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<LOD, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
