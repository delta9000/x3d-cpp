#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct ElevationGrid {
  static constexpr std::string_view x3d_name = "ElevationGrid";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
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
  inline static constexpr std::array<field_key_descriptor, 23> field_keys{{
      {attrib.name(), attrib.kind, attrib.access()},
      {ccw.name(), ccw.kind, ccw.access()},
      {color.name(), color.kind, color.access()},
      {colorPerVertex.name(), colorPerVertex.kind, colorPerVertex.access()},
      {creaseAngle.name(), creaseAngle.kind, creaseAngle.access()},
      {fogCoord.name(), fogCoord.kind, fogCoord.access()},
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
