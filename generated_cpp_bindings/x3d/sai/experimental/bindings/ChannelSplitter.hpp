#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct ChannelSplitter {
  static constexpr std::string_view x3d_name = "ChannelSplitter";
  inline static constexpr field_key<ChannelSplitter, std::int32_t> channelCount{
      "channelCount", access_type::output_only};
  inline static constexpr field_key<ChannelSplitter,
                                    ::x3d::sai::experimental::enum_value>
      channelCountMode{"channelCountMode", access_type::input_output};
  inline static constexpr field_key<ChannelSplitter,
                                    ::x3d::sai::experimental::enum_value>
      channelInterpretation{"channelInterpretation", access_type::input_output};
  inline static constexpr field_key<ChannelSplitter,
                                    ::x3d::sai::experimental::node_list>
      children{"children", access_type::input_output};
  inline static constexpr field_key<ChannelSplitter, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<ChannelSplitter, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<ChannelSplitter, float> gain{
      "gain", access_type::input_output};
  inline static constexpr field_key<ChannelSplitter,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<ChannelSplitter,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<ChannelSplitter,
                                    ::x3d::sai::experimental::node_list>
      outputs{"outputs", access_type::input_output};
  inline static constexpr field_key<ChannelSplitter, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<ChannelSplitter, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<ChannelSplitter, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<ChannelSplitter, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<ChannelSplitter, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
