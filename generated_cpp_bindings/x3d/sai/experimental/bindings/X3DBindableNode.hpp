#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DBindableNode {
  static constexpr std::string_view x3d_name = "X3DBindableNode";
  inline static constexpr field_key<X3DBindableNode,
                                    ::x3d::sai::experimental::time_value>
      bindTime{"bindTime", access_type::output_only};
  inline static constexpr field_key<X3DBindableNode,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<X3DBindableNode, bool> isBound{
      "isBound", access_type::output_only};
  inline static constexpr field_key<X3DBindableNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<X3DBindableNode, bool> set_bind{
      "set_bind", access_type::input_only};
  inline static constexpr field_key<X3DBindableNode, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<X3DBindableNode, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<X3DBindableNode, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<X3DBindableNode, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<X3DBindableNode, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
