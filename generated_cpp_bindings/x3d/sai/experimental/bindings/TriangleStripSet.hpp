#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct TriangleStripSet {
  static constexpr std::string_view x3d_name = "TriangleStripSet";
  inline static constexpr field_key<TriangleStripSet,
                                    ::x3d::sai::experimental::node_list>
      attrib{"attrib", access_type::input_output};
  inline static constexpr field_key<TriangleStripSet, bool> ccw{
      "ccw", access_type::initialize_only};
  inline static constexpr field_key<TriangleStripSet,
                                    ::x3d::sai::experimental::node_id>
      color{"color", access_type::input_output};
  inline static constexpr field_key<TriangleStripSet, bool> colorPerVertex{
      "colorPerVertex", access_type::initialize_only};
  inline static constexpr field_key<TriangleStripSet,
                                    ::x3d::sai::experimental::node_id>
      coord{"coord", access_type::input_output};
  inline static constexpr field_key<TriangleStripSet,
                                    ::x3d::sai::experimental::node_id>
      fogCoord{"fogCoord", access_type::input_output};
  inline static constexpr field_key<TriangleStripSet,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<TriangleStripSet,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<TriangleStripSet,
                                    ::x3d::sai::experimental::node_id>
      normal{"normal", access_type::input_output};
  inline static constexpr field_key<TriangleStripSet, bool> normalPerVertex{
      "normalPerVertex", access_type::initialize_only};
  inline static constexpr field_key<TriangleStripSet, bool> solid{
      "solid", access_type::initialize_only};
  inline static constexpr field_key<TriangleStripSet,
                                    ::x3d::sai::experimental::int32_list>
      stripCount{"stripCount", access_type::input_output};
  inline static constexpr field_key<TriangleStripSet,
                                    ::x3d::sai::experimental::node_id>
      texCoord{"texCoord", access_type::input_output};
  inline static constexpr field_key<TriangleStripSet, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<TriangleStripSet, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<TriangleStripSet, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<TriangleStripSet, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<TriangleStripSet, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
