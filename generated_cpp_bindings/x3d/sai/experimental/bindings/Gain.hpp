#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct Gain {
  static constexpr std::string_view x3d_name = "Gain";
  inline static constexpr field_key<Gain, std::int32_t> channelCount{
      "channelCount", access_type::output_only};
  inline static constexpr field_key<Gain, ::x3d::sai::experimental::enum_value>
      channelCountMode{"channelCountMode", access_type::input_output};
  inline static constexpr field_key<Gain, ::x3d::sai::experimental::enum_value>
      channelInterpretation{"channelInterpretation", access_type::input_output};
  inline static constexpr field_key<Gain, ::x3d::sai::experimental::node_list>
      children{"children", access_type::input_output};
  inline static constexpr field_key<Gain, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<Gain, ::x3d::sai::experimental::time_value>
      elapsedTime{"elapsedTime", access_type::output_only};
  inline static constexpr field_key<Gain, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<Gain, float> gain{
      "gain", access_type::input_output};
  inline static constexpr field_key<Gain, ::x3d::sai::experimental::node_id> IS{
      "IS", access_type::input_output};
  inline static constexpr field_key<Gain, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<Gain, bool> isPaused{
      "isPaused", access_type::output_only};
  inline static constexpr field_key<Gain, ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<Gain, ::x3d::sai::experimental::time_value>
      pauseTime{"pauseTime", access_type::input_output};
  inline static constexpr field_key<Gain, ::x3d::sai::experimental::time_value>
      resumeTime{"resumeTime", access_type::input_output};
  inline static constexpr field_key<Gain, ::x3d::sai::experimental::time_value>
      startTime{"startTime", access_type::input_output};
  inline static constexpr field_key<Gain, ::x3d::sai::experimental::time_value>
      stopTime{"stopTime", access_type::input_output};
  inline static constexpr field_key<Gain, ::x3d::sai::experimental::time_value>
      tailTime{"tailTime", access_type::input_output};
  inline static constexpr field_key<Gain, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<Gain, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<Gain, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<Gain, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<Gain, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
