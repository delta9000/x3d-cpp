#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DNurbsControlCurveNode {
  static constexpr std::string_view x3d_name = "X3DNurbsControlCurveNode";
  inline static constexpr field_key<X3DNurbsControlCurveNode,
                                    ::x3d::sai::experimental::vec2d_list>
      controlPoint{"controlPoint", access_type::input_output};
  inline static constexpr field_key<X3DNurbsControlCurveNode,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<X3DNurbsControlCurveNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<X3DNurbsControlCurveNode, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<X3DNurbsControlCurveNode, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<X3DNurbsControlCurveNode, std::string>
      class_{"class", access_type::input_output};
  inline static constexpr field_key<X3DNurbsControlCurveNode, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<X3DNurbsControlCurveNode, std::string>
      style{"style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
