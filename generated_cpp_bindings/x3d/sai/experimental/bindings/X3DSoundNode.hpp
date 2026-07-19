#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DSoundNode {
  static constexpr std::string_view x3d_name = "X3DSoundNode";
  inline static constexpr field_key<X3DSoundNode, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<X3DSoundNode, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<X3DSoundNode,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<X3DSoundNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<X3DSoundNode, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<X3DSoundNode, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<X3DSoundNode, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<X3DSoundNode, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<X3DSoundNode, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
