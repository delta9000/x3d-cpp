#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DScriptNode {
  static constexpr std::string_view x3d_name = "X3DScriptNode";
  inline static constexpr field_key<X3DScriptNode,
                                    ::x3d::sai::experimental::time_value>
      autoRefresh{"autoRefresh", access_type::input_output};
  inline static constexpr field_key<X3DScriptNode,
                                    ::x3d::sai::experimental::time_value>
      autoRefreshTimeLimit{"autoRefreshTimeLimit", access_type::input_output};
  inline static constexpr field_key<X3DScriptNode, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<X3DScriptNode,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<X3DScriptNode, bool> load{
      "load", access_type::input_output};
  inline static constexpr field_key<X3DScriptNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<X3DScriptNode,
                                    ::x3d::sai::experimental::string_list>
      url{"url", access_type::input_output};
  inline static constexpr field_key<X3DScriptNode, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<X3DScriptNode, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<X3DScriptNode, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<X3DScriptNode, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<X3DScriptNode, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
