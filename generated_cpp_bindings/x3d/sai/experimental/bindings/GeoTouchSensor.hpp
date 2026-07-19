#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct GeoTouchSensor {
  static constexpr std::string_view x3d_name = "GeoTouchSensor";
  inline static constexpr field_key<GeoTouchSensor, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<GeoTouchSensor, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<GeoTouchSensor,
                                    ::x3d::sai::experimental::node_id>
      geoOrigin{"geoOrigin", access_type::initialize_only};
  inline static constexpr field_key<GeoTouchSensor,
                                    ::x3d::sai::experimental::string_list>
      geoSystem{"geoSystem", access_type::initialize_only};
  inline static constexpr field_key<GeoTouchSensor,
                                    ::x3d::sai::experimental::vec3d>
      hitGeoCoord_changed{"hitGeoCoord_changed", access_type::output_only};
  inline static constexpr field_key<GeoTouchSensor,
                                    ::x3d::sai::experimental::vec3f>
      hitNormal_changed{"hitNormal_changed", access_type::output_only};
  inline static constexpr field_key<GeoTouchSensor,
                                    ::x3d::sai::experimental::vec3f>
      hitPoint_changed{"hitPoint_changed", access_type::output_only};
  inline static constexpr field_key<GeoTouchSensor,
                                    ::x3d::sai::experimental::vec2f>
      hitTexCoord_changed{"hitTexCoord_changed", access_type::output_only};
  inline static constexpr field_key<GeoTouchSensor,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<GeoTouchSensor, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<GeoTouchSensor, bool> isOver{
      "isOver", access_type::output_only};
  inline static constexpr field_key<GeoTouchSensor,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<GeoTouchSensor,
                                    ::x3d::sai::experimental::time_value>
      touchTime{"touchTime", access_type::output_only};
  inline static constexpr field_key<GeoTouchSensor, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<GeoTouchSensor, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<GeoTouchSensor, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<GeoTouchSensor, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<GeoTouchSensor, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
