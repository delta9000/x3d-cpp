#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct GeoProximitySensor {
  static constexpr std::string_view x3d_name = "GeoProximitySensor";
  inline static constexpr field_key<GeoProximitySensor,
                                    ::x3d::sai::experimental::vec3d>
      center{"center", access_type::input_output};
  inline static constexpr field_key<GeoProximitySensor,
                                    ::x3d::sai::experimental::vec3f>
      centerOfRotation_changed{"centerOfRotation_changed",
                               access_type::output_only};
  inline static constexpr field_key<GeoProximitySensor, std::string>
      description{"description", access_type::input_output};
  inline static constexpr field_key<GeoProximitySensor, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<GeoProximitySensor,
                                    ::x3d::sai::experimental::time_value>
      enterTime{"enterTime", access_type::output_only};
  inline static constexpr field_key<GeoProximitySensor,
                                    ::x3d::sai::experimental::time_value>
      exitTime{"exitTime", access_type::output_only};
  inline static constexpr field_key<GeoProximitySensor,
                                    ::x3d::sai::experimental::vec3d>
      geoCenter{"geoCenter", access_type::input_output};
  inline static constexpr field_key<GeoProximitySensor,
                                    ::x3d::sai::experimental::vec3d>
      geoCoord_changed{"geoCoord_changed", access_type::output_only};
  inline static constexpr field_key<GeoProximitySensor,
                                    ::x3d::sai::experimental::node_id>
      geoOrigin{"geoOrigin", access_type::initialize_only};
  inline static constexpr field_key<GeoProximitySensor,
                                    ::x3d::sai::experimental::string_list>
      geoSystem{"geoSystem", access_type::initialize_only};
  inline static constexpr field_key<GeoProximitySensor,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<GeoProximitySensor, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<GeoProximitySensor,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<GeoProximitySensor,
                                    ::x3d::sai::experimental::rotation>
      orientation_changed{"orientation_changed", access_type::output_only};
  inline static constexpr field_key<GeoProximitySensor,
                                    ::x3d::sai::experimental::vec3f>
      position_changed{"position_changed", access_type::output_only};
  inline static constexpr field_key<GeoProximitySensor,
                                    ::x3d::sai::experimental::vec3f>
      size{"size", access_type::input_output};
  inline static constexpr field_key<GeoProximitySensor, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<GeoProximitySensor, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<GeoProximitySensor, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<GeoProximitySensor, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<GeoProximitySensor, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
