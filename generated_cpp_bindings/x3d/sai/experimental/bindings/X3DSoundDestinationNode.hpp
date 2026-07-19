#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct X3DSoundDestinationNode {
  static constexpr std::string_view x3d_name = "X3DSoundDestinationNode";
  inline static constexpr field_key<X3DSoundDestinationNode, std::int32_t>
      channelCount{"channelCount", access_type::output_only};
  inline static constexpr field_key<X3DSoundDestinationNode,
                                    ::x3d::sai::experimental::enum_value>
      channelCountMode{"channelCountMode", access_type::input_output};
  inline static constexpr field_key<X3DSoundDestinationNode,
                                    ::x3d::sai::experimental::enum_value>
      channelInterpretation{"channelInterpretation", access_type::input_output};
  inline static constexpr field_key<X3DSoundDestinationNode, std::string>
      description{"description", access_type::input_output};
  inline static constexpr field_key<X3DSoundDestinationNode, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<X3DSoundDestinationNode, float> gain{
      "gain", access_type::input_output};
  inline static constexpr field_key<X3DSoundDestinationNode,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<X3DSoundDestinationNode, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<X3DSoundDestinationNode, std::string>
      mediaDeviceID{"mediaDeviceID", access_type::input_output};
  inline static constexpr field_key<X3DSoundDestinationNode,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<X3DSoundDestinationNode, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<X3DSoundDestinationNode, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<X3DSoundDestinationNode, std::string>
      class_{"class", access_type::input_output};
  inline static constexpr field_key<X3DSoundDestinationNode, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<X3DSoundDestinationNode, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
