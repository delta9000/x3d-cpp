#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DNormalNode {
  static constexpr std::string_view x3d_name = "X3DNormalNode";
  inline static constexpr field_key<X3DNormalNode,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<X3DNormalNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<X3DNormalNode, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<X3DNormalNode, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<X3DNormalNode, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<X3DNormalNode, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<X3DNormalNode, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
