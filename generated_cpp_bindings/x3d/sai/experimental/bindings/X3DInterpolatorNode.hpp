#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DInterpolatorNode {
  static constexpr std::string_view x3d_name = "X3DInterpolatorNode";
  inline static constexpr field_key<X3DInterpolatorNode,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<X3DInterpolatorNode,
                                    ::x3d::sai::experimental::float_list>
      key{"key", access_type::input_output};
  inline static constexpr field_key<X3DInterpolatorNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<X3DInterpolatorNode, float> set_fraction{
      "set_fraction", access_type::input_only};
  inline static constexpr field_key<X3DInterpolatorNode, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<X3DInterpolatorNode, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<X3DInterpolatorNode, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<X3DInterpolatorNode, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<X3DInterpolatorNode, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
