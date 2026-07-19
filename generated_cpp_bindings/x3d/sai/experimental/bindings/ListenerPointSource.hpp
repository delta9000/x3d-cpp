#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct ListenerPointSource {
  static constexpr std::string_view x3d_name = "ListenerPointSource";
  inline static constexpr field_key<ListenerPointSource, std::string>
      description{"description", access_type::input_output};
  inline static constexpr field_key<ListenerPointSource, bool> dopplerEnabled{
      "dopplerEnabled", access_type::input_output};
  inline static constexpr field_key<ListenerPointSource,
                                    ::x3d::sai::experimental::time_value>
      elapsedTime{"elapsedTime", access_type::output_only};
  inline static constexpr field_key<ListenerPointSource, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<ListenerPointSource, float> gain{
      "gain", access_type::input_output};
  inline static constexpr field_key<ListenerPointSource, float>
      interauralDistance{"interauralDistance", access_type::input_output};
  inline static constexpr field_key<ListenerPointSource,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<ListenerPointSource, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<ListenerPointSource, bool> isPaused{
      "isPaused", access_type::output_only};
  inline static constexpr field_key<ListenerPointSource,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<ListenerPointSource,
                                    ::x3d::sai::experimental::rotation>
      orientation{"orientation", access_type::input_output};
  inline static constexpr field_key<ListenerPointSource,
                                    ::x3d::sai::experimental::time_value>
      pauseTime{"pauseTime", access_type::input_output};
  inline static constexpr field_key<ListenerPointSource,
                                    ::x3d::sai::experimental::vec3f>
      position{"position", access_type::input_output};
  inline static constexpr field_key<ListenerPointSource,
                                    ::x3d::sai::experimental::time_value>
      resumeTime{"resumeTime", access_type::input_output};
  inline static constexpr field_key<ListenerPointSource,
                                    ::x3d::sai::experimental::time_value>
      startTime{"startTime", access_type::input_output};
  inline static constexpr field_key<ListenerPointSource,
                                    ::x3d::sai::experimental::time_value>
      stopTime{"stopTime", access_type::input_output};
  inline static constexpr field_key<ListenerPointSource, bool> trackCurrentView{
      "trackCurrentView", access_type::input_output};
  inline static constexpr field_key<ListenerPointSource, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<ListenerPointSource, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<ListenerPointSource, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<ListenerPointSource, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<ListenerPointSource, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
