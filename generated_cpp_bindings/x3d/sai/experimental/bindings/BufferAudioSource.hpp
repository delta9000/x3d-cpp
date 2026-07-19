#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct BufferAudioSource {
  static constexpr std::string_view x3d_name = "BufferAudioSource";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
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
  inline static constexpr std::array<field_key_descriptor, 35> field_keys{{
      {autoRefresh.name(), autoRefresh.kind, autoRefresh.access()},
      {autoRefreshTimeLimit.name(), autoRefreshTimeLimit.kind,
       autoRefreshTimeLimit.access()},
      {buffer.name(), buffer.kind, buffer.access()},
      {bufferDuration.name(), bufferDuration.kind, bufferDuration.access()},
      {bufferlength.name(), bufferlength.kind, bufferlength.access()},
      {channelCount.name(), channelCount.kind, channelCount.access()},
      {channelCountMode.name(), channelCountMode.kind,
       channelCountMode.access()},
      {channelInterpretation.name(), channelInterpretation.kind,
       channelInterpretation.access()},
      {description.name(), description.kind, description.access()},
      {detune.name(), detune.kind, detune.access()},
      {elapsedTime.name(), elapsedTime.kind, elapsedTime.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {gain.name(), gain.kind, gain.access()},
      {IS.name(), IS.kind, IS.access()},
      {isActive.name(), isActive.kind, isActive.access()},
      {isPaused.name(), isPaused.kind, isPaused.access()},
      {length.name(), length.kind, length.access()},
      {load.name(), load.kind, load.access()},
      {loop.name(), loop.kind, loop.access()},
      {loopEnd.name(), loopEnd.kind, loopEnd.access()},
      {loopStart.name(), loopStart.kind, loopStart.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {numberOfChannels.name(), numberOfChannels.kind,
       numberOfChannels.access()},
      {pauseTime.name(), pauseTime.kind, pauseTime.access()},
      {playbackRate.name(), playbackRate.kind, playbackRate.access()},
      {resumeTime.name(), resumeTime.kind, resumeTime.access()},
      {sampleRate.name(), sampleRate.kind, sampleRate.access()},
      {startTime.name(), startTime.kind, startTime.access()},
      {stopTime.name(), stopTime.kind, stopTime.access()},
      {url.name(), url.kind, url.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
