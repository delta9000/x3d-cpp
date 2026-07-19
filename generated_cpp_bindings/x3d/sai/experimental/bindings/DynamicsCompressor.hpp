#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct DynamicsCompressor {
  static constexpr std::string_view x3d_name = "DynamicsCompressor";
  inline static constexpr field_key<DynamicsCompressor,
                                    ::x3d::sai::experimental::time_value>
      attack{"attack", access_type::input_output};
  inline static constexpr field_key<DynamicsCompressor, std::int32_t>
      channelCount{"channelCount", access_type::output_only};
  inline static constexpr field_key<DynamicsCompressor,
                                    ::x3d::sai::experimental::enum_value>
      channelCountMode{"channelCountMode", access_type::input_output};
  inline static constexpr field_key<DynamicsCompressor,
                                    ::x3d::sai::experimental::enum_value>
      channelInterpretation{"channelInterpretation", access_type::input_output};
  inline static constexpr field_key<DynamicsCompressor,
                                    ::x3d::sai::experimental::node_list>
      children{"children", access_type::input_output};
  inline static constexpr field_key<DynamicsCompressor, std::string>
      description{"description", access_type::input_output};
  inline static constexpr field_key<DynamicsCompressor,
                                    ::x3d::sai::experimental::time_value>
      elapsedTime{"elapsedTime", access_type::output_only};
  inline static constexpr field_key<DynamicsCompressor, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<DynamicsCompressor, float> gain{
      "gain", access_type::input_output};
  inline static constexpr field_key<DynamicsCompressor,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<DynamicsCompressor, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<DynamicsCompressor, bool> isPaused{
      "isPaused", access_type::output_only};
  inline static constexpr field_key<DynamicsCompressor, float> knee{
      "knee", access_type::input_output};
  inline static constexpr field_key<DynamicsCompressor,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<DynamicsCompressor,
                                    ::x3d::sai::experimental::time_value>
      pauseTime{"pauseTime", access_type::input_output};
  inline static constexpr field_key<DynamicsCompressor, float> ratio{
      "ratio", access_type::input_output};
  inline static constexpr field_key<DynamicsCompressor, float> reduction{
      "reduction", access_type::output_only};
  inline static constexpr field_key<DynamicsCompressor,
                                    ::x3d::sai::experimental::time_value>
      release{"release", access_type::input_output};
  inline static constexpr field_key<DynamicsCompressor,
                                    ::x3d::sai::experimental::time_value>
      resumeTime{"resumeTime", access_type::input_output};
  inline static constexpr field_key<DynamicsCompressor,
                                    ::x3d::sai::experimental::time_value>
      startTime{"startTime", access_type::input_output};
  inline static constexpr field_key<DynamicsCompressor,
                                    ::x3d::sai::experimental::time_value>
      stopTime{"stopTime", access_type::input_output};
  inline static constexpr field_key<DynamicsCompressor,
                                    ::x3d::sai::experimental::time_value>
      tailTime{"tailTime", access_type::input_output};
  inline static constexpr field_key<DynamicsCompressor, float> threshold{
      "threshold", access_type::input_output};
  inline static constexpr field_key<DynamicsCompressor, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<DynamicsCompressor, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<DynamicsCompressor, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<DynamicsCompressor, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<DynamicsCompressor, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
