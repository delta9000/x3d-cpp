#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct Delay {
  static constexpr std::string_view x3d_name = "Delay";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<Delay, std::int32_t> channelCount{
      "channelCount", access_type::output_only};
  inline static constexpr field_key<Delay, ::x3d::sai::experimental::enum_value>
      channelCountMode{"channelCountMode", access_type::input_output};
  inline static constexpr field_key<Delay, ::x3d::sai::experimental::enum_value>
      channelInterpretation{"channelInterpretation", access_type::input_output};
  inline static constexpr field_key<Delay, ::x3d::sai::experimental::node_list>
      children{"children", access_type::input_output};
  inline static constexpr field_key<Delay, ::x3d::sai::experimental::time_value>
      delayTime{"delayTime", access_type::input_output};
  inline static constexpr field_key<Delay, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<Delay, ::x3d::sai::experimental::time_value>
      elapsedTime{"elapsedTime", access_type::output_only};
  inline static constexpr field_key<Delay, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<Delay, float> gain{
      "gain", access_type::input_output};
  inline static constexpr field_key<Delay, ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<Delay, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<Delay, bool> isPaused{
      "isPaused", access_type::output_only};
  inline static constexpr field_key<Delay, ::x3d::sai::experimental::time_value>
      maxDelayTime{"maxDelayTime", access_type::input_output};
  inline static constexpr field_key<Delay, ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<Delay, ::x3d::sai::experimental::time_value>
      pauseTime{"pauseTime", access_type::input_output};
  inline static constexpr field_key<Delay, ::x3d::sai::experimental::time_value>
      resumeTime{"resumeTime", access_type::input_output};
  inline static constexpr field_key<Delay, ::x3d::sai::experimental::time_value>
      startTime{"startTime", access_type::input_output};
  inline static constexpr field_key<Delay, ::x3d::sai::experimental::time_value>
      stopTime{"stopTime", access_type::input_output};
  inline static constexpr field_key<Delay, ::x3d::sai::experimental::time_value>
      tailTime{"tailTime", access_type::input_output};
  inline static constexpr field_key<Delay, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<Delay, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<Delay, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<Delay, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<Delay, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 24> field_keys{{
      {channelCount.name(), channelCount.kind, channelCount.access()},
      {channelCountMode.name(), channelCountMode.kind,
       channelCountMode.access()},
      {channelInterpretation.name(), channelInterpretation.kind,
       channelInterpretation.access()},
      {children.name(), children.kind, children.access()},
      {delayTime.name(), delayTime.kind, delayTime.access()},
      {description.name(), description.kind, description.access()},
      {elapsedTime.name(), elapsedTime.kind, elapsedTime.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {gain.name(), gain.kind, gain.access()},
      {IS.name(), IS.kind, IS.access()},
      {isActive.name(), isActive.kind, isActive.access()},
      {isPaused.name(), isPaused.kind, isPaused.access()},
      {maxDelayTime.name(), maxDelayTime.kind, maxDelayTime.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {pauseTime.name(), pauseTime.kind, pauseTime.access()},
      {resumeTime.name(), resumeTime.kind, resumeTime.access()},
      {startTime.name(), startTime.kind, startTime.access()},
      {stopTime.name(), stopTime.kind, stopTime.access()},
      {tailTime.name(), tailTime.kind, tailTime.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
