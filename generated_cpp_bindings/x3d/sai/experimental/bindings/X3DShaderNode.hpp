#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DShaderNode {
  static constexpr std::string_view x3d_name = "X3DShaderNode";
  inline static constexpr field_key<X3DShaderNode, bool> activate{
      "activate", access_type::input_only};
  inline static constexpr field_key<X3DShaderNode,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<X3DShaderNode, bool> isSelected{
      "isSelected", access_type::output_only};
  inline static constexpr field_key<X3DShaderNode, bool> isValid{
      "isValid", access_type::output_only};
  inline static constexpr field_key<X3DShaderNode, std::string> language{
      "language", access_type::initialize_only};
  inline static constexpr field_key<X3DShaderNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<X3DShaderNode, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<X3DShaderNode, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<X3DShaderNode, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<X3DShaderNode, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<X3DShaderNode, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
