#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct TimeSensor {
  static constexpr std::string_view x3d_name = "TimeSensor";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<TimeSensor,
                                    ::x3d::sai::experimental::time_value>
      cycleInterval{"cycleInterval", access_type::input_output};
  inline static constexpr field_key<TimeSensor,
                                    ::x3d::sai::experimental::time_value>
      cycleTime{"cycleTime", access_type::output_only};
  inline static constexpr field_key<TimeSensor, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<TimeSensor,
                                    ::x3d::sai::experimental::time_value>
      elapsedTime{"elapsedTime", access_type::output_only};
  inline static constexpr field_key<TimeSensor, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<TimeSensor, float> fraction_changed{
      "fraction_changed", access_type::output_only};
  inline static constexpr field_key<TimeSensor,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<TimeSensor, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<TimeSensor, bool> isPaused{
      "isPaused", access_type::output_only};
  inline static constexpr field_key<TimeSensor, bool> loop{
      "loop", access_type::input_output};
  inline static constexpr field_key<TimeSensor,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<TimeSensor,
                                    ::x3d::sai::experimental::time_value>
      pauseTime{"pauseTime", access_type::input_output};
  inline static constexpr field_key<TimeSensor,
                                    ::x3d::sai::experimental::time_value>
      resumeTime{"resumeTime", access_type::input_output};
  inline static constexpr field_key<TimeSensor,
                                    ::x3d::sai::experimental::time_value>
      startTime{"startTime", access_type::input_output};
  inline static constexpr field_key<TimeSensor,
                                    ::x3d::sai::experimental::time_value>
      stopTime{"stopTime", access_type::input_output};
  inline static constexpr field_key<TimeSensor,
                                    ::x3d::sai::experimental::time_value>
      time{"time", access_type::output_only};
  inline static constexpr field_key<TimeSensor, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<TimeSensor, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<TimeSensor, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<TimeSensor, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<TimeSensor, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 21> field_keys{{
      {cycleInterval.name(), cycleInterval.kind, cycleInterval.access()},
      {cycleTime.name(), cycleTime.kind, cycleTime.access()},
      {description.name(), description.kind, description.access()},
      {elapsedTime.name(), elapsedTime.kind, elapsedTime.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {fraction_changed.name(), fraction_changed.kind,
       fraction_changed.access()},
      {IS.name(), IS.kind, IS.access()},
      {isActive.name(), isActive.kind, isActive.access()},
      {isPaused.name(), isPaused.kind, isPaused.access()},
      {loop.name(), loop.kind, loop.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {pauseTime.name(), pauseTime.kind, pauseTime.access()},
      {resumeTime.name(), resumeTime.kind, resumeTime.access()},
      {startTime.name(), startTime.kind, startTime.access()},
      {stopTime.name(), stopTime.kind, stopTime.access()},
      {time.name(), time.kind, time.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
