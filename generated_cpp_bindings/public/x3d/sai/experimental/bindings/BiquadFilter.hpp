#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/schema.hpp"

namespace x3d::sai::experimental::bindings {

struct BiquadFilter {
  static constexpr std::string_view x3d_name = "BiquadFilter";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<BiquadFilter, std::int32_t> channelCount{
      "channelCount", access_type::output_only};
  inline static constexpr field_key<BiquadFilter, std::string> channelCountMode{
      "channelCountMode", access_type::input_output};
  inline static constexpr field_key<BiquadFilter, std::string>
      channelInterpretation{"channelInterpretation", access_type::input_output};
  inline static constexpr field_key<BiquadFilter,
                                    ::x3d::sai::experimental::node_list>
      children{"children", access_type::input_output};
  inline static constexpr field_key<BiquadFilter, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<BiquadFilter, float> detune{
      "detune", access_type::input_output};
  inline static constexpr field_key<BiquadFilter,
                                    ::x3d::sai::experimental::time_value>
      elapsedTime{"elapsedTime", access_type::output_only};
  inline static constexpr field_key<BiquadFilter, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<BiquadFilter, float> frequency{
      "frequency", access_type::input_output};
  inline static constexpr field_key<BiquadFilter, float> gain{
      "gain", access_type::input_output};
  inline static constexpr field_key<BiquadFilter, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<BiquadFilter, bool> isPaused{
      "isPaused", access_type::output_only};
  inline static constexpr field_key<BiquadFilter,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<BiquadFilter,
                                    ::x3d::sai::experimental::time_value>
      pauseTime{"pauseTime", access_type::input_output};
  inline static constexpr field_key<BiquadFilter, float> qualityFactor{
      "qualityFactor", access_type::input_output};
  inline static constexpr field_key<BiquadFilter,
                                    ::x3d::sai::experimental::time_value>
      resumeTime{"resumeTime", access_type::input_output};
  inline static constexpr field_key<BiquadFilter,
                                    ::x3d::sai::experimental::time_value>
      startTime{"startTime", access_type::input_output};
  inline static constexpr field_key<BiquadFilter,
                                    ::x3d::sai::experimental::time_value>
      stopTime{"stopTime", access_type::input_output};
  inline static constexpr field_key<BiquadFilter,
                                    ::x3d::sai::experimental::time_value>
      tailTime{"tailTime", access_type::input_output};
  inline static constexpr field_key<BiquadFilter, std::string> type{
      "type", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 20> field_keys{{
      {channelCount.name(), channelCount.kind, channelCount.access()},
      {channelCountMode.name(), channelCountMode.kind,
       channelCountMode.access()},
      {channelInterpretation.name(), channelInterpretation.kind,
       channelInterpretation.access()},
      {children.name(), children.kind, children.access()},
      {description.name(), description.kind, description.access()},
      {detune.name(), detune.kind, detune.access()},
      {elapsedTime.name(), elapsedTime.kind, elapsedTime.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {frequency.name(), frequency.kind, frequency.access()},
      {gain.name(), gain.kind, gain.access()},
      {isActive.name(), isActive.kind, isActive.access()},
      {isPaused.name(), isPaused.kind, isPaused.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {pauseTime.name(), pauseTime.kind, pauseTime.access()},
      {qualityFactor.name(), qualityFactor.kind, qualityFactor.access()},
      {resumeTime.name(), resumeTime.kind, resumeTime.access()},
      {startTime.name(), startTime.kind, startTime.access()},
      {stopTime.name(), stopTime.kind, stopTime.access()},
      {tailTime.name(), tailTime.kind, tailTime.access()},
      {type.name(), type.kind, type.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
