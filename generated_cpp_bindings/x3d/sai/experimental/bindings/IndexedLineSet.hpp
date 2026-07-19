#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct IndexedLineSet {
  static constexpr std::string_view x3d_name = "IndexedLineSet";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<IndexedLineSet,
                                    ::x3d::sai::experimental::node_list>
      attrib{"attrib", access_type::input_output};
  inline static constexpr field_key<IndexedLineSet,
                                    ::x3d::sai::experimental::node_id>
      color{"color", access_type::input_output};
  inline static constexpr field_key<IndexedLineSet,
                                    ::x3d::sai::experimental::int32_list>
      colorIndex{"colorIndex", access_type::initialize_only};
  inline static constexpr field_key<IndexedLineSet, bool> colorPerVertex{
      "colorPerVertex", access_type::initialize_only};
  inline static constexpr field_key<IndexedLineSet,
                                    ::x3d::sai::experimental::node_id>
      coord{"coord", access_type::input_output};
  inline static constexpr field_key<IndexedLineSet,
                                    ::x3d::sai::experimental::int32_list>
      coordIndex{"coordIndex", access_type::initialize_only};
  inline static constexpr field_key<IndexedLineSet,
                                    ::x3d::sai::experimental::node_id>
      fogCoord{"fogCoord", access_type::input_output};
  inline static constexpr field_key<IndexedLineSet,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<IndexedLineSet,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<IndexedLineSet,
                                    ::x3d::sai::experimental::node_id>
      normal{"normal", access_type::input_output};
  inline static constexpr field_key<IndexedLineSet,
                                    ::x3d::sai::experimental::int32_list>
      set_colorIndex{"set_colorIndex", access_type::input_only};
  inline static constexpr field_key<IndexedLineSet,
                                    ::x3d::sai::experimental::int32_list>
      set_coordIndex{"set_coordIndex", access_type::input_only};
  inline static constexpr field_key<IndexedLineSet, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<IndexedLineSet, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<IndexedLineSet, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<IndexedLineSet, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<IndexedLineSet, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 17> field_keys{{
      {attrib.name(), attrib.kind, attrib.access()},
      {color.name(), color.kind, color.access()},
      {colorIndex.name(), colorIndex.kind, colorIndex.access()},
      {colorPerVertex.name(), colorPerVertex.kind, colorPerVertex.access()},
      {coord.name(), coord.kind, coord.access()},
      {coordIndex.name(), coordIndex.kind, coordIndex.access()},
      {fogCoord.name(), fogCoord.kind, fogCoord.access()},
      {IS.name(), IS.kind, IS.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {normal.name(), normal.kind, normal.access()},
      {set_colorIndex.name(), set_colorIndex.kind, set_colorIndex.access()},
      {set_coordIndex.name(), set_coordIndex.kind, set_coordIndex.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
