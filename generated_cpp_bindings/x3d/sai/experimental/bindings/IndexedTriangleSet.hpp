#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct IndexedTriangleSet {
  static constexpr std::string_view x3d_name = "IndexedTriangleSet";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<IndexedTriangleSet,
                                    ::x3d::sai::experimental::node_list>
      attrib{"attrib", access_type::input_output};
  inline static constexpr field_key<IndexedTriangleSet, bool> ccw{
      "ccw", access_type::initialize_only};
  inline static constexpr field_key<IndexedTriangleSet,
                                    ::x3d::sai::experimental::node_id>
      color{"color", access_type::input_output};
  inline static constexpr field_key<IndexedTriangleSet, bool> colorPerVertex{
      "colorPerVertex", access_type::initialize_only};
  inline static constexpr field_key<IndexedTriangleSet,
                                    ::x3d::sai::experimental::node_id>
      coord{"coord", access_type::input_output};
  inline static constexpr field_key<IndexedTriangleSet,
                                    ::x3d::sai::experimental::node_id>
      fogCoord{"fogCoord", access_type::input_output};
  inline static constexpr field_key<IndexedTriangleSet,
                                    ::x3d::sai::experimental::int32_list>
      index{"index", access_type::initialize_only};
  inline static constexpr field_key<IndexedTriangleSet,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<IndexedTriangleSet,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<IndexedTriangleSet,
                                    ::x3d::sai::experimental::node_id>
      normal{"normal", access_type::input_output};
  inline static constexpr field_key<IndexedTriangleSet, bool> normalPerVertex{
      "normalPerVertex", access_type::initialize_only};
  inline static constexpr field_key<IndexedTriangleSet,
                                    ::x3d::sai::experimental::int32_list>
      set_index{"set_index", access_type::input_only};
  inline static constexpr field_key<IndexedTriangleSet, bool> solid{
      "solid", access_type::initialize_only};
  inline static constexpr field_key<IndexedTriangleSet,
                                    ::x3d::sai::experimental::node_id>
      texCoord{"texCoord", access_type::input_output};
  inline static constexpr field_key<IndexedTriangleSet, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<IndexedTriangleSet, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<IndexedTriangleSet, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<IndexedTriangleSet, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<IndexedTriangleSet, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 19> field_keys{{
      {attrib.name(), attrib.kind, attrib.access()},
      {ccw.name(), ccw.kind, ccw.access()},
      {color.name(), color.kind, color.access()},
      {colorPerVertex.name(), colorPerVertex.kind, colorPerVertex.access()},
      {coord.name(), coord.kind, coord.access()},
      {fogCoord.name(), fogCoord.kind, fogCoord.access()},
      {index.name(), index.kind, index.access()},
      {IS.name(), IS.kind, IS.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {normal.name(), normal.kind, normal.access()},
      {normalPerVertex.name(), normalPerVertex.kind, normalPerVertex.access()},
      {set_index.name(), set_index.kind, set_index.access()},
      {solid.name(), solid.kind, solid.access()},
      {texCoord.name(), texCoord.kind, texCoord.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
