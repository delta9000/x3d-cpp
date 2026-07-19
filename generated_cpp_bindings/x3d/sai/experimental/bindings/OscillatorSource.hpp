#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct OscillatorSource {
  static constexpr std::string_view x3d_name = "OscillatorSource";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<OscillatorSource, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<OscillatorSource, float> detune{
      "detune", access_type::input_output};
  inline static constexpr field_key<OscillatorSource,
                                    ::x3d::sai::experimental::time_value>
      elapsedTime{"elapsedTime", access_type::output_only};
  inline static constexpr field_key<OscillatorSource, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<OscillatorSource, float> frequency{
      "frequency", access_type::input_output};
  inline static constexpr field_key<OscillatorSource, float> gain{
      "gain", access_type::input_output};
  inline static constexpr field_key<OscillatorSource,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<OscillatorSource, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<OscillatorSource, bool> isPaused{
      "isPaused", access_type::output_only};
  inline static constexpr field_key<OscillatorSource,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<OscillatorSource,
                                    ::x3d::sai::experimental::time_value>
      pauseTime{"pauseTime", access_type::input_output};
  inline static constexpr field_key<OscillatorSource,
                                    ::x3d::sai::experimental::time_value>
      resumeTime{"resumeTime", access_type::input_output};
  inline static constexpr field_key<OscillatorSource,
                                    ::x3d::sai::experimental::time_value>
      startTime{"startTime", access_type::input_output};
  inline static constexpr field_key<OscillatorSource,
                                    ::x3d::sai::experimental::time_value>
      stopTime{"stopTime", access_type::input_output};
  inline static constexpr field_key<OscillatorSource, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<OscillatorSource, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<OscillatorSource, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<OscillatorSource, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<OscillatorSource, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 19> field_keys{{
      {description.name(), description.kind, description.access()},
      {detune.name(), detune.kind, detune.access()},
      {elapsedTime.name(), elapsedTime.kind, elapsedTime.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {frequency.name(), frequency.kind, frequency.access()},
      {gain.name(), gain.kind, gain.access()},
      {IS.name(), IS.kind, IS.access()},
      {isActive.name(), isActive.kind, isActive.access()},
      {isPaused.name(), isPaused.kind, isPaused.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {pauseTime.name(), pauseTime.kind, pauseTime.access()},
      {resumeTime.name(), resumeTime.kind, resumeTime.access()},
      {startTime.name(), startTime.kind, startTime.access()},
      {stopTime.name(), stopTime.kind, stopTime.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
