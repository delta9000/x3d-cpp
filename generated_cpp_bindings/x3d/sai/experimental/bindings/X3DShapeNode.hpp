#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DShapeNode {
  static constexpr std::string_view x3d_name = "X3DShapeNode";
  inline static constexpr field_key<X3DShapeNode,
                                    ::x3d::sai::experimental::node_id>
      appearance{"appearance", access_type::input_output};
  inline static constexpr field_key<X3DShapeNode,
                                    ::x3d::sai::experimental::vec3f>
      bboxCenter{"bboxCenter", access_type::initialize_only};
  inline static constexpr field_key<X3DShapeNode, bool> bboxDisplay{
      "bboxDisplay", access_type::input_output};
  inline static constexpr field_key<X3DShapeNode,
                                    ::x3d::sai::experimental::vec3f>
      bboxSize{"bboxSize", access_type::initialize_only};
  inline static constexpr field_key<X3DShapeNode, bool> castShadow{
      "castShadow", access_type::input_output};
  inline static constexpr field_key<X3DShapeNode,
                                    ::x3d::sai::experimental::node_id>
      geometry{"geometry", access_type::input_output};
  inline static constexpr field_key<X3DShapeNode,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<X3DShapeNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<X3DShapeNode, bool> visible{
      "visible", access_type::input_output};
  inline static constexpr field_key<X3DShapeNode, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<X3DShapeNode, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<X3DShapeNode, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<X3DShapeNode, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<X3DShapeNode, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
