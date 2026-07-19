#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct GeoElevationGrid {
  static constexpr std::string_view x3d_name = "GeoElevationGrid";
  inline static constexpr field_key<GeoElevationGrid, bool> ccw{
      "ccw", access_type::initialize_only};
  inline static constexpr field_key<GeoElevationGrid,
                                    ::x3d::sai::experimental::node_id>
      color{"color", access_type::input_output};
  inline static constexpr field_key<GeoElevationGrid, bool> colorPerVertex{
      "colorPerVertex", access_type::initialize_only};
  inline static constexpr field_key<GeoElevationGrid, double> creaseAngle{
      "creaseAngle", access_type::initialize_only};
  inline static constexpr field_key<GeoElevationGrid,
                                    ::x3d::sai::experimental::vec3d>
      geoGridOrigin{"geoGridOrigin", access_type::initialize_only};
  inline static constexpr field_key<GeoElevationGrid,
                                    ::x3d::sai::experimental::node_id>
      geoOrigin{"geoOrigin", access_type::initialize_only};
  inline static constexpr field_key<GeoElevationGrid,
                                    ::x3d::sai::experimental::string_list>
      geoSystem{"geoSystem", access_type::initialize_only};
  inline static constexpr field_key<GeoElevationGrid,
                                    ::x3d::sai::experimental::double_list>
      height{"height", access_type::initialize_only};
  inline static constexpr field_key<GeoElevationGrid,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<GeoElevationGrid,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<GeoElevationGrid,
                                    ::x3d::sai::experimental::node_id>
      normal{"normal", access_type::input_output};
  inline static constexpr field_key<GeoElevationGrid, bool> normalPerVertex{
      "normalPerVertex", access_type::initialize_only};
  inline static constexpr field_key<GeoElevationGrid,
                                    ::x3d::sai::experimental::double_list>
      set_height{"set_height", access_type::input_only};
  inline static constexpr field_key<GeoElevationGrid, bool> solid{
      "solid", access_type::initialize_only};
  inline static constexpr field_key<GeoElevationGrid,
                                    ::x3d::sai::experimental::node_id>
      texCoord{"texCoord", access_type::input_output};
  inline static constexpr field_key<GeoElevationGrid, std::int32_t> xDimension{
      "xDimension", access_type::initialize_only};
  inline static constexpr field_key<GeoElevationGrid, double> xSpacing{
      "xSpacing", access_type::initialize_only};
  inline static constexpr field_key<GeoElevationGrid, float> yScale{
      "yScale", access_type::input_output};
  inline static constexpr field_key<GeoElevationGrid, std::int32_t> zDimension{
      "zDimension", access_type::initialize_only};
  inline static constexpr field_key<GeoElevationGrid, double> zSpacing{
      "zSpacing", access_type::initialize_only};
  inline static constexpr field_key<GeoElevationGrid, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<GeoElevationGrid, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<GeoElevationGrid, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<GeoElevationGrid, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<GeoElevationGrid, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
