#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DNetworkSensorNode {
  static constexpr std::string_view x3d_name = "X3DNetworkSensorNode";
  inline static constexpr field_key<X3DNetworkSensorNode, std::string>
      description{"description", access_type::input_output};
  inline static constexpr field_key<X3DNetworkSensorNode, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<X3DNetworkSensorNode,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<X3DNetworkSensorNode, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<X3DNetworkSensorNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<X3DNetworkSensorNode, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<X3DNetworkSensorNode, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<X3DNetworkSensorNode, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<X3DNetworkSensorNode, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<X3DNetworkSensorNode, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
