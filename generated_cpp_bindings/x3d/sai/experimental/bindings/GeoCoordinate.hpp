#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct GeoCoordinate {
  static constexpr std::string_view x3d_name = "GeoCoordinate";
  inline static constexpr field_key<GeoCoordinate,
                                    ::x3d::sai::experimental::node_id>
      geoOrigin{"geoOrigin", access_type::initialize_only};
  inline static constexpr field_key<GeoCoordinate,
                                    ::x3d::sai::experimental::string_list>
      geoSystem{"geoSystem", access_type::initialize_only};
  inline static constexpr field_key<GeoCoordinate,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<GeoCoordinate,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<GeoCoordinate,
                                    ::x3d::sai::experimental::vec3d_list>
      point{"point", access_type::input_output};
  inline static constexpr field_key<GeoCoordinate, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<GeoCoordinate, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<GeoCoordinate, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<GeoCoordinate, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<GeoCoordinate, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
