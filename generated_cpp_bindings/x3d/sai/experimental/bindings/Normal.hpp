#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct Normal {
  static constexpr std::string_view x3d_name = "Normal";
  inline static constexpr field_key<Normal, ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<Normal, ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<Normal,
                                    ::x3d::sai::experimental::vec3f_list>
      vector{"vector", access_type::input_output};
  inline static constexpr field_key<Normal, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<Normal, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<Normal, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<Normal, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<Normal, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
