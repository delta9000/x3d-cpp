#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct GeoTransform {
  static constexpr std::string_view x3d_name = "GeoTransform";
  inline static constexpr field_key<GeoTransform,
                                    ::x3d::sai::experimental::node_list>
      addChildren{"addChildren", access_type::input_only};
  inline static constexpr field_key<GeoTransform,
                                    ::x3d::sai::experimental::vec3f>
      bboxCenter{"bboxCenter", access_type::initialize_only};
  inline static constexpr field_key<GeoTransform, bool> bboxDisplay{
      "bboxDisplay", access_type::input_output};
  inline static constexpr field_key<GeoTransform,
                                    ::x3d::sai::experimental::vec3f>
      bboxSize{"bboxSize", access_type::initialize_only};
  inline static constexpr field_key<GeoTransform,
                                    ::x3d::sai::experimental::node_list>
      children{"children", access_type::input_output};
  inline static constexpr field_key<GeoTransform,
                                    ::x3d::sai::experimental::vec3d>
      geoCenter{"geoCenter", access_type::input_output};
  inline static constexpr field_key<GeoTransform,
                                    ::x3d::sai::experimental::node_id>
      geoOrigin{"geoOrigin", access_type::initialize_only};
  inline static constexpr field_key<GeoTransform,
                                    ::x3d::sai::experimental::string_list>
      geoSystem{"geoSystem", access_type::initialize_only};
  inline static constexpr field_key<GeoTransform,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<GeoTransform,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<GeoTransform,
                                    ::x3d::sai::experimental::node_list>
      removeChildren{"removeChildren", access_type::input_only};
  inline static constexpr field_key<GeoTransform,
                                    ::x3d::sai::experimental::rotation>
      rotation{"rotation", access_type::input_output};
  inline static constexpr field_key<GeoTransform,
                                    ::x3d::sai::experimental::vec3f>
      scale{"scale", access_type::input_output};
  inline static constexpr field_key<GeoTransform,
                                    ::x3d::sai::experimental::rotation>
      scaleOrientation{"scaleOrientation", access_type::input_output};
  inline static constexpr field_key<GeoTransform,
                                    ::x3d::sai::experimental::vec3f>
      translation{"translation", access_type::input_output};
  inline static constexpr field_key<GeoTransform, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr field_key<GeoTransform, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<GeoTransform, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<GeoTransform, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<GeoTransform, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<GeoTransform, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
