#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct ElevationGrid {
  static constexpr std::string_view x3d_name = "ElevationGrid";
  inline static constexpr field_key<ElevationGrid,
                                    ::x3d::sai::experimental::node_list>
      attrib{"attrib", access_type::input_output};
  inline static constexpr field_key<ElevationGrid, bool> ccw{
      "ccw", access_type::initialize_only};
  inline static constexpr field_key<ElevationGrid,
                                    ::x3d::sai::experimental::node_id>
      color{"color", access_type::input_output};
  inline static constexpr field_key<ElevationGrid, bool> colorPerVertex{
      "colorPerVertex", access_type::initialize_only};
  inline static constexpr field_key<ElevationGrid, float> creaseAngle{
      "creaseAngle", access_type::initialize_only};
  inline static constexpr field_key<ElevationGrid,
                                    ::x3d::sai::experimental::node_id>
      fogCoord{"fogCoord", access_type::input_output};
  inline static constexpr field_key<ElevationGrid,
                                    ::x3d::sai::experimental::float_list>
      height{"height", access_type::initialize_only};
  inline static constexpr field_key<ElevationGrid,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<ElevationGrid,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<ElevationGrid,
                                    ::x3d::sai::experimental::node_id>
      normal{"normal", access_type::input_output};
  inline static constexpr field_key<ElevationGrid, bool> normalPerVertex{
      "normalPerVertex", access_type::initialize_only};
  inline static constexpr field_key<ElevationGrid,
                                    ::x3d::sai::experimental::float_list>
      set_height{"set_height", access_type::input_only};
  inline static constexpr field_key<ElevationGrid, bool> solid{
      "solid", access_type::initialize_only};
  inline static constexpr field_key<ElevationGrid,
                                    ::x3d::sai::experimental::node_id>
      texCoord{"texCoord", access_type::input_output};
  inline static constexpr field_key<ElevationGrid, std::int32_t> xDimension{
      "xDimension", access_type::initialize_only};
  inline static constexpr field_key<ElevationGrid, float> xSpacing{
      "xSpacing", access_type::initialize_only};
  inline static constexpr field_key<ElevationGrid, std::int32_t> zDimension{
      "zDimension", access_type::initialize_only};
  inline static constexpr field_key<ElevationGrid, float> zSpacing{
      "zSpacing", access_type::initialize_only};
  inline static constexpr field_key<ElevationGrid, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<ElevationGrid, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<ElevationGrid, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<ElevationGrid, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<ElevationGrid, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
