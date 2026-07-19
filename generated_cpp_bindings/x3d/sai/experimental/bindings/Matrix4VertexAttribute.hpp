#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct Matrix4VertexAttribute {
  static constexpr std::string_view x3d_name = "Matrix4VertexAttribute";
  inline static constexpr field_key<Matrix4VertexAttribute,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<Matrix4VertexAttribute,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<Matrix4VertexAttribute, std::string> name{
      "name", access_type::initialize_only};
  inline static constexpr field_key<Matrix4VertexAttribute,
                                    ::x3d::sai::experimental::matrix4f_list>
      value{"value", access_type::input_output};
  inline static constexpr field_key<Matrix4VertexAttribute, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<Matrix4VertexAttribute, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<Matrix4VertexAttribute, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<Matrix4VertexAttribute, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<Matrix4VertexAttribute, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
