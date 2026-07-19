#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DSensorNode {
  static constexpr std::string_view x3d_name = "X3DSensorNode";
  inline static constexpr field_key<X3DSensorNode, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<X3DSensorNode, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<X3DSensorNode,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<X3DSensorNode, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<X3DSensorNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<X3DSensorNode, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<X3DSensorNode, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<X3DSensorNode, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<X3DSensorNode, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<X3DSensorNode, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
