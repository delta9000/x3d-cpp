#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DBackgroundNode {
  static constexpr std::string_view x3d_name = "X3DBackgroundNode";
  inline static constexpr field_key<X3DBackgroundNode,
                                    ::x3d::sai::experimental::time_value>
      bindTime{"bindTime", access_type::output_only};
  inline static constexpr field_key<X3DBackgroundNode,
                                    ::x3d::sai::experimental::float_list>
      groundAngle{"groundAngle", access_type::input_output};
  inline static constexpr field_key<X3DBackgroundNode,
                                    ::x3d::sai::experimental::color3f_list>
      groundColor{"groundColor", access_type::input_output};
  inline static constexpr field_key<X3DBackgroundNode,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<X3DBackgroundNode, bool> isBound{
      "isBound", access_type::output_only};
  inline static constexpr field_key<X3DBackgroundNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<X3DBackgroundNode, bool> set_bind{
      "set_bind", access_type::input_only};
  inline static constexpr field_key<X3DBackgroundNode,
                                    ::x3d::sai::experimental::float_list>
      skyAngle{"skyAngle", access_type::input_output};
  inline static constexpr field_key<X3DBackgroundNode,
                                    ::x3d::sai::experimental::color3f_list>
      skyColor{"skyColor", access_type::input_output};
  inline static constexpr field_key<X3DBackgroundNode, float> transparency{
      "transparency", access_type::input_output};
  inline static constexpr field_key<X3DBackgroundNode, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<X3DBackgroundNode, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<X3DBackgroundNode, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<X3DBackgroundNode, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<X3DBackgroundNode, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
