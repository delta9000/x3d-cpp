#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct Box {
  static constexpr std::string_view x3d_name = "Box";
  inline static constexpr field_key<Box, ::x3d::sai::experimental::node_id> IS{
      "IS", access_type::input_output};
  inline static constexpr field_key<Box, ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<Box, ::x3d::sai::experimental::vec3f> size{
      "size", access_type::initialize_only};
  inline static constexpr field_key<Box, bool> solid{
      "solid", access_type::initialize_only};
  inline static constexpr field_key<Box, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<Box, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<Box, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<Box, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<Box, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
