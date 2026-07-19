#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct StreamAudioDestination {
  static constexpr std::string_view x3d_name = "StreamAudioDestination";
  inline static constexpr field_key<StreamAudioDestination, std::int32_t>
      channelCount{"channelCount", access_type::output_only};
  inline static constexpr field_key<StreamAudioDestination,
                                    ::x3d::sai::experimental::enum_value>
      channelCountMode{"channelCountMode", access_type::input_output};
  inline static constexpr field_key<StreamAudioDestination,
                                    ::x3d::sai::experimental::enum_value>
      channelInterpretation{"channelInterpretation", access_type::input_output};
  inline static constexpr field_key<StreamAudioDestination,
                                    ::x3d::sai::experimental::node_list>
      children{"children", access_type::input_output};
  inline static constexpr field_key<StreamAudioDestination, std::string>
      description{"description", access_type::input_output};
  inline static constexpr field_key<StreamAudioDestination, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<StreamAudioDestination, float> gain{
      "gain", access_type::input_output};
  inline static constexpr field_key<StreamAudioDestination,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<StreamAudioDestination, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<StreamAudioDestination, std::string>
      mediaDeviceID{"mediaDeviceID", access_type::input_output};
  inline static constexpr field_key<StreamAudioDestination,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<StreamAudioDestination, std::string>
      streamIdentifier{"streamIdentifier", access_type::input_output};
  inline static constexpr field_key<StreamAudioDestination, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<StreamAudioDestination, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<StreamAudioDestination, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<StreamAudioDestination, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<StreamAudioDestination, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
