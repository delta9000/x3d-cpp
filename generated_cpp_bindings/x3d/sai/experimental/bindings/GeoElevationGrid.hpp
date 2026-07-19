#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct GeoElevationGrid {
  static constexpr std::string_view x3d_name = "GeoElevationGrid";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
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
  inline static constexpr std::array<field_key_descriptor, 25> field_keys{{
      {ccw.name(), ccw.kind, ccw.access()},
      {color.name(), color.kind, color.access()},
      {colorPerVertex.name(), colorPerVertex.kind, colorPerVertex.access()},
      {creaseAngle.name(), creaseAngle.kind, creaseAngle.access()},
      {geoGridOrigin.name(), geoGridOrigin.kind, geoGridOrigin.access()},
      {geoOrigin.name(), geoOrigin.kind, geoOrigin.access()},
      {geoSystem.name(), geoSystem.kind, geoSystem.access()},
      {height.name(), height.kind, height.access()},
      {IS.name(), IS.kind, IS.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {normal.name(), normal.kind, normal.access()},
      {normalPerVertex.name(), normalPerVertex.kind, normalPerVertex.access()},
      {set_height.name(), set_height.kind, set_height.access()},
      {solid.name(), solid.kind, solid.access()},
      {texCoord.name(), texCoord.kind, texCoord.access()},
      {xDimension.name(), xDimension.kind, xDimension.access()},
      {xSpacing.name(), xSpacing.kind, xSpacing.access()},
      {yScale.name(), yScale.kind, yScale.access()},
      {zDimension.name(), zDimension.kind, zDimension.access()},
      {zSpacing.name(), zSpacing.kind, zSpacing.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
