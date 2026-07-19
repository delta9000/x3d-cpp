#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DTriggerNode {
  static constexpr std::string_view x3d_name = "X3DTriggerNode";
  inline static constexpr field_key<X3DTriggerNode,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<X3DTriggerNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<X3DTriggerNode, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<X3DTriggerNode, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<X3DTriggerNode, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<X3DTriggerNode, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<X3DTriggerNode, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
