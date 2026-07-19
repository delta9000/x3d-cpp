#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct IndexedTriangleFanSet {
  static constexpr std::string_view x3d_name = "IndexedTriangleFanSet";
  inline static constexpr field_key<IndexedTriangleFanSet,
                                    ::x3d::sai::experimental::node_list>
      attrib{"attrib", access_type::input_output};
  inline static constexpr field_key<IndexedTriangleFanSet, bool> ccw{
      "ccw", access_type::initialize_only};
  inline static constexpr field_key<IndexedTriangleFanSet,
                                    ::x3d::sai::experimental::node_id>
      color{"color", access_type::input_output};
  inline static constexpr field_key<IndexedTriangleFanSet, bool> colorPerVertex{
      "colorPerVertex", access_type::initialize_only};
  inline static constexpr field_key<IndexedTriangleFanSet,
                                    ::x3d::sai::experimental::node_id>
      coord{"coord", access_type::input_output};
  inline static constexpr field_key<IndexedTriangleFanSet,
                                    ::x3d::sai::experimental::node_id>
      fogCoord{"fogCoord", access_type::input_output};
  inline static constexpr field_key<IndexedTriangleFanSet,
                                    ::x3d::sai::experimental::int32_list>
      index{"index", access_type::initialize_only};
  inline static constexpr field_key<IndexedTriangleFanSet,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<IndexedTriangleFanSet,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<IndexedTriangleFanSet,
                                    ::x3d::sai::experimental::node_id>
      normal{"normal", access_type::input_output};
  inline static constexpr field_key<IndexedTriangleFanSet, bool>
      normalPerVertex{"normalPerVertex", access_type::initialize_only};
  inline static constexpr field_key<IndexedTriangleFanSet,
                                    ::x3d::sai::experimental::int32_list>
      set_index{"set_index", access_type::input_only};
  inline static constexpr field_key<IndexedTriangleFanSet, bool> solid{
      "solid", access_type::initialize_only};
  inline static constexpr field_key<IndexedTriangleFanSet,
                                    ::x3d::sai::experimental::node_id>
      texCoord{"texCoord", access_type::input_output};
  inline static constexpr field_key<IndexedTriangleFanSet, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<IndexedTriangleFanSet, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<IndexedTriangleFanSet, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<IndexedTriangleFanSet, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<IndexedTriangleFanSet, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
