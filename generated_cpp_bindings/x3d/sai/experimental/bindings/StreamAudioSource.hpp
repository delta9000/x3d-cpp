#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct StreamAudioSource {
  static constexpr std::string_view x3d_name = "StreamAudioSource";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<StreamAudioSource, std::string>
      channelCountMode{"channelCountMode", access_type::input_output};
  inline static constexpr field_key<StreamAudioSource, std::string>
      channelInterpretation{"channelInterpretation", access_type::input_output};
  inline static constexpr field_key<StreamAudioSource, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<StreamAudioSource,
                                    ::x3d::sai::experimental::time_value>
      elapsedTime{"elapsedTime", access_type::output_only};
  inline static constexpr field_key<StreamAudioSource, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<StreamAudioSource, float> gain{
      "gain", access_type::input_output};
  inline static constexpr field_key<StreamAudioSource, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<StreamAudioSource, bool> isPaused{
      "isPaused", access_type::output_only};
  inline static constexpr field_key<StreamAudioSource,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<StreamAudioSource,
                                    ::x3d::sai::experimental::time_value>
      pauseTime{"pauseTime", access_type::input_output};
  inline static constexpr field_key<StreamAudioSource,
                                    ::x3d::sai::experimental::time_value>
      resumeTime{"resumeTime", access_type::input_output};
  inline static constexpr field_key<StreamAudioSource,
                                    ::x3d::sai::experimental::time_value>
      startTime{"startTime", access_type::input_output};
  inline static constexpr field_key<StreamAudioSource,
                                    ::x3d::sai::experimental::time_value>
      stopTime{"stopTime", access_type::input_output};
  inline static constexpr field_key<StreamAudioSource, std::string>
      streamIdentifier{"streamIdentifier", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 14> field_keys{{
      {channelCountMode.name(), channelCountMode.kind,
       channelCountMode.access()},
      {channelInterpretation.name(), channelInterpretation.kind,
       channelInterpretation.access()},
      {description.name(), description.kind, description.access()},
      {elapsedTime.name(), elapsedTime.kind, elapsedTime.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {gain.name(), gain.kind, gain.access()},
      {isActive.name(), isActive.kind, isActive.access()},
      {isPaused.name(), isPaused.kind, isPaused.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {pauseTime.name(), pauseTime.kind, pauseTime.access()},
      {resumeTime.name(), resumeTime.kind, resumeTime.access()},
      {startTime.name(), startTime.kind, startTime.access()},
      {stopTime.name(), stopTime.kind, stopTime.access()},
      {streamIdentifier.name(), streamIdentifier.kind,
       streamIdentifier.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
