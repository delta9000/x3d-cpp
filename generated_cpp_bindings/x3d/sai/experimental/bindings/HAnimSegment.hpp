#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct HAnimSegment {
  static constexpr std::string_view x3d_name = "HAnimSegment";
  inline static constexpr field_key<HAnimSegment,
                                    ::x3d::sai::experimental::node_list>
      addChildren{"addChildren", access_type::input_only};
  inline static constexpr field_key<HAnimSegment,
                                    ::x3d::sai::experimental::vec3f>
      bboxCenter{"bboxCenter", access_type::initialize_only};
  inline static constexpr field_key<HAnimSegment, bool> bboxDisplay{
      "bboxDisplay", access_type::input_output};
  inline static constexpr field_key<HAnimSegment,
                                    ::x3d::sai::experimental::vec3f>
      bboxSize{"bboxSize", access_type::initialize_only};
  inline static constexpr field_key<HAnimSegment,
                                    ::x3d::sai::experimental::vec3f>
      centerOfMass{"centerOfMass", access_type::input_output};
  inline static constexpr field_key<HAnimSegment,
                                    ::x3d::sai::experimental::node_list>
      children{"children", access_type::input_output};
  inline static constexpr field_key<HAnimSegment,
                                    ::x3d::sai::experimental::node_id>
      coord{"coord", access_type::input_output};
  inline static constexpr field_key<HAnimSegment, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<HAnimSegment,
                                    ::x3d::sai::experimental::node_list>
      displacers{"displacers", access_type::input_output};
  inline static constexpr field_key<HAnimSegment,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<HAnimSegment, float> mass{
      "mass", access_type::input_output};
  inline static constexpr field_key<HAnimSegment,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<HAnimSegment,
                                    ::x3d::sai::experimental::float_list>
      momentsOfInertia{"momentsOfInertia", access_type::input_output};
  inline static constexpr field_key<HAnimSegment, std::string> name{
      "name", access_type::input_output};
  inline static constexpr field_key<HAnimSegment,
                                    ::x3d::sai::experimental::node_list>
      removeChildren{"removeChildren", access_type::input_only};
  inline static constexpr field_key<HAnimSegment, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr field_key<HAnimSegment, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<HAnimSegment, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<HAnimSegment, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<HAnimSegment, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<HAnimSegment, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
