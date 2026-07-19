#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct AudioClip {
  static constexpr std::string_view x3d_name = "AudioClip";
  inline static constexpr field_key<AudioClip,
                                    ::x3d::sai::experimental::time_value>
      autoRefresh{"autoRefresh", access_type::input_output};
  inline static constexpr field_key<AudioClip,
                                    ::x3d::sai::experimental::time_value>
      autoRefreshTimeLimit{"autoRefreshTimeLimit", access_type::input_output};
  inline static constexpr field_key<AudioClip, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<AudioClip,
                                    ::x3d::sai::experimental::time_value>
      duration_changed{"duration_changed", access_type::output_only};
  inline static constexpr field_key<AudioClip,
                                    ::x3d::sai::experimental::time_value>
      elapsedTime{"elapsedTime", access_type::output_only};
  inline static constexpr field_key<AudioClip, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<AudioClip, float> gain{
      "gain", access_type::input_output};
  inline static constexpr field_key<AudioClip,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<AudioClip, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<AudioClip, bool> isPaused{
      "isPaused", access_type::output_only};
  inline static constexpr field_key<AudioClip, bool> load{
      "load", access_type::input_output};
  inline static constexpr field_key<AudioClip, bool> loop{
      "loop", access_type::input_output};
  inline static constexpr field_key<AudioClip,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<AudioClip,
                                    ::x3d::sai::experimental::time_value>
      pauseTime{"pauseTime", access_type::input_output};
  inline static constexpr field_key<AudioClip, float> pitch{
      "pitch", access_type::input_output};
  inline static constexpr field_key<AudioClip,
                                    ::x3d::sai::experimental::time_value>
      resumeTime{"resumeTime", access_type::input_output};
  inline static constexpr field_key<AudioClip,
                                    ::x3d::sai::experimental::time_value>
      startTime{"startTime", access_type::input_output};
  inline static constexpr field_key<AudioClip,
                                    ::x3d::sai::experimental::time_value>
      stopTime{"stopTime", access_type::input_output};
  inline static constexpr field_key<AudioClip,
                                    ::x3d::sai::experimental::string_list>
      url{"url", access_type::input_output};
  inline static constexpr field_key<AudioClip, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<AudioClip, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<AudioClip, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<AudioClip, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<AudioClip, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
