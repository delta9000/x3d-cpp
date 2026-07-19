#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct BufferAudioSource {
  static constexpr std::string_view x3d_name = "BufferAudioSource";
  inline static constexpr field_key<BufferAudioSource,
                                    ::x3d::sai::experimental::time_value>
      autoRefresh{"autoRefresh", access_type::input_output};
  inline static constexpr field_key<BufferAudioSource,
                                    ::x3d::sai::experimental::time_value>
      autoRefreshTimeLimit{"autoRefreshTimeLimit", access_type::input_output};
  inline static constexpr field_key<BufferAudioSource,
                                    ::x3d::sai::experimental::float_list>
      buffer{"buffer", access_type::input_output};
  inline static constexpr field_key<BufferAudioSource,
                                    ::x3d::sai::experimental::time_value>
      bufferDuration{"bufferDuration", access_type::input_output};
  inline static constexpr field_key<BufferAudioSource, std::int32_t>
      bufferlength{"bufferlength", access_type::output_only};
  inline static constexpr field_key<BufferAudioSource, std::int32_t>
      channelCount{"channelCount", access_type::output_only};
  inline static constexpr field_key<BufferAudioSource,
                                    ::x3d::sai::experimental::enum_value>
      channelCountMode{"channelCountMode", access_type::input_output};
  inline static constexpr field_key<BufferAudioSource,
                                    ::x3d::sai::experimental::enum_value>
      channelInterpretation{"channelInterpretation", access_type::input_output};
  inline static constexpr field_key<BufferAudioSource, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<BufferAudioSource, float> detune{
      "detune", access_type::input_output};
  inline static constexpr field_key<BufferAudioSource,
                                    ::x3d::sai::experimental::time_value>
      elapsedTime{"elapsedTime", access_type::output_only};
  inline static constexpr field_key<BufferAudioSource, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<BufferAudioSource, float> gain{
      "gain", access_type::input_output};
  inline static constexpr field_key<BufferAudioSource,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<BufferAudioSource, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<BufferAudioSource, bool> isPaused{
      "isPaused", access_type::output_only};
  inline static constexpr field_key<BufferAudioSource, std::int32_t> length{
      "length", access_type::output_only};
  inline static constexpr field_key<BufferAudioSource, bool> load{
      "load", access_type::input_output};
  inline static constexpr field_key<BufferAudioSource, bool> loop{
      "loop", access_type::input_output};
  inline static constexpr field_key<BufferAudioSource, float> loopEnd{
      "loopEnd", access_type::input_output};
  inline static constexpr field_key<BufferAudioSource, float> loopStart{
      "loopStart", access_type::input_output};
  inline static constexpr field_key<BufferAudioSource,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<BufferAudioSource, std::int32_t>
      numberOfChannels{"numberOfChannels", access_type::input_output};
  inline static constexpr field_key<BufferAudioSource,
                                    ::x3d::sai::experimental::time_value>
      pauseTime{"pauseTime", access_type::input_output};
  inline static constexpr field_key<BufferAudioSource, float> playbackRate{
      "playbackRate", access_type::input_output};
  inline static constexpr field_key<BufferAudioSource,
                                    ::x3d::sai::experimental::time_value>
      resumeTime{"resumeTime", access_type::input_output};
  inline static constexpr field_key<BufferAudioSource, float> sampleRate{
      "sampleRate", access_type::input_output};
  inline static constexpr field_key<BufferAudioSource,
                                    ::x3d::sai::experimental::time_value>
      startTime{"startTime", access_type::input_output};
  inline static constexpr field_key<BufferAudioSource,
                                    ::x3d::sai::experimental::time_value>
      stopTime{"stopTime", access_type::input_output};
  inline static constexpr field_key<BufferAudioSource,
                                    ::x3d::sai::experimental::string_list>
      url{"url", access_type::input_output};
  inline static constexpr field_key<BufferAudioSource, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<BufferAudioSource, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<BufferAudioSource, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<BufferAudioSource, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<BufferAudioSource, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
