#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct GeoLOD {
  static constexpr std::string_view x3d_name = "GeoLOD";
  inline static constexpr field_key<GeoLOD, ::x3d::sai::experimental::vec3f>
      bboxCenter{"bboxCenter", access_type::initialize_only};
  inline static constexpr field_key<GeoLOD, bool> bboxDisplay{
      "bboxDisplay", access_type::input_output};
  inline static constexpr field_key<GeoLOD, ::x3d::sai::experimental::vec3f>
      bboxSize{"bboxSize", access_type::initialize_only};
  inline static constexpr field_key<GeoLOD, ::x3d::sai::experimental::vec3d>
      center{"center", access_type::initialize_only};
  inline static constexpr field_key<GeoLOD,
                                    ::x3d::sai::experimental::string_list>
      child1Url{"child1Url", access_type::initialize_only};
  inline static constexpr field_key<GeoLOD,
                                    ::x3d::sai::experimental::string_list>
      child2Url{"child2Url", access_type::initialize_only};
  inline static constexpr field_key<GeoLOD,
                                    ::x3d::sai::experimental::string_list>
      child3Url{"child3Url", access_type::initialize_only};
  inline static constexpr field_key<GeoLOD,
                                    ::x3d::sai::experimental::string_list>
      child4Url{"child4Url", access_type::initialize_only};
  inline static constexpr field_key<GeoLOD, ::x3d::sai::experimental::node_list>
      children{"children", access_type::output_only};
  inline static constexpr field_key<GeoLOD, ::x3d::sai::experimental::node_id>
      geoOrigin{"geoOrigin", access_type::initialize_only};
  inline static constexpr field_key<GeoLOD,
                                    ::x3d::sai::experimental::string_list>
      geoSystem{"geoSystem", access_type::initialize_only};
  inline static constexpr field_key<GeoLOD, ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<GeoLOD, std::int32_t> level_changed{
      "level_changed", access_type::output_only};
  inline static constexpr field_key<GeoLOD, ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<GeoLOD, float> range{
      "range", access_type::initialize_only};
  inline static constexpr field_key<GeoLOD, ::x3d::sai::experimental::node_list>
      rootNode{"rootNode", access_type::initialize_only};
  inline static constexpr field_key<GeoLOD,
                                    ::x3d::sai::experimental::string_list>
      rootUrl{"rootUrl", access_type::initialize_only};
  inline static constexpr field_key<GeoLOD, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr field_key<GeoLOD, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<GeoLOD, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<GeoLOD, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<GeoLOD, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<GeoLOD, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
