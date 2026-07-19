#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct TimeSensor {
  static constexpr std::string_view x3d_name = "TimeSensor";
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
};

} // namespace x3d::sai::experimental::bindings
