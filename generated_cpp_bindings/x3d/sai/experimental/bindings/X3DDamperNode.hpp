#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DDamperNode {
  static constexpr std::string_view x3d_name = "X3DDamperNode";
  inline static constexpr field_key<X3DDamperNode,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<X3DDamperNode, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<X3DDamperNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<X3DDamperNode, std::int32_t> order{
      "order", access_type::initialize_only};
  inline static constexpr field_key<X3DDamperNode,
                                    ::x3d::sai::experimental::time_value>
      tau{"tau", access_type::input_output};
  inline static constexpr field_key<X3DDamperNode, float> tolerance{
      "tolerance", access_type::input_output};
  inline static constexpr field_key<X3DDamperNode, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<X3DDamperNode, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<X3DDamperNode, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<X3DDamperNode, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<X3DDamperNode, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
