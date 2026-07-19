#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct MicrophoneSource {
  static constexpr std::string_view x3d_name = "MicrophoneSource";
  inline static constexpr field_key<MicrophoneSource, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<MicrophoneSource,
                                    ::x3d::sai::experimental::time_value>
      elapsedTime{"elapsedTime", access_type::output_only};
  inline static constexpr field_key<MicrophoneSource, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<MicrophoneSource, float> gain{
      "gain", access_type::input_output};
  inline static constexpr field_key<MicrophoneSource,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<MicrophoneSource, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<MicrophoneSource, bool> isPaused{
      "isPaused", access_type::output_only};
  inline static constexpr field_key<MicrophoneSource, std::string>
      mediaDeviceID{"mediaDeviceID", access_type::input_output};
  inline static constexpr field_key<MicrophoneSource,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<MicrophoneSource,
                                    ::x3d::sai::experimental::time_value>
      pauseTime{"pauseTime", access_type::input_output};
  inline static constexpr field_key<MicrophoneSource,
                                    ::x3d::sai::experimental::time_value>
      resumeTime{"resumeTime", access_type::input_output};
  inline static constexpr field_key<MicrophoneSource,
                                    ::x3d::sai::experimental::time_value>
      startTime{"startTime", access_type::input_output};
  inline static constexpr field_key<MicrophoneSource,
                                    ::x3d::sai::experimental::time_value>
      stopTime{"stopTime", access_type::input_output};
  inline static constexpr field_key<MicrophoneSource, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<MicrophoneSource, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<MicrophoneSource, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<MicrophoneSource, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<MicrophoneSource, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
