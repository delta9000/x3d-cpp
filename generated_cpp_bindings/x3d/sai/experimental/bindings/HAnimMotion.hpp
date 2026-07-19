#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct HAnimMotion {
  static constexpr std::string_view x3d_name = "HAnimMotion";
  inline static constexpr field_key<HAnimMotion, std::string> channels{
      "channels", access_type::input_output};
  inline static constexpr field_key<HAnimMotion,
                                    ::x3d::sai::experimental::bool_list>
      channelsEnabled{"channelsEnabled", access_type::input_output};
  inline static constexpr field_key<HAnimMotion,
                                    ::x3d::sai::experimental::time_value>
      cycleTime{"cycleTime", access_type::output_only};
  inline static constexpr field_key<HAnimMotion, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<HAnimMotion,
                                    ::x3d::sai::experimental::time_value>
      elapsedTime{"elapsedTime", access_type::output_only};
  inline static constexpr field_key<HAnimMotion, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<HAnimMotion, std::int32_t> endFrame{
      "endFrame", access_type::input_output};
  inline static constexpr field_key<HAnimMotion, std::int32_t> frameCount{
      "frameCount", access_type::output_only};
  inline static constexpr field_key<HAnimMotion,
                                    ::x3d::sai::experimental::time_value>
      frameDuration{"frameDuration", access_type::input_output};
  inline static constexpr field_key<HAnimMotion, std::int32_t> frameIncrement{
      "frameIncrement", access_type::input_output};
  inline static constexpr field_key<HAnimMotion, std::int32_t> frameIndex{
      "frameIndex", access_type::input_output};
  inline static constexpr field_key<HAnimMotion,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<HAnimMotion, std::string> joints{
      "joints", access_type::input_output};
  inline static constexpr field_key<HAnimMotion, std::int32_t> loa{
      "loa", access_type::input_output};
  inline static constexpr field_key<HAnimMotion, bool> loop{
      "loop", access_type::input_output};
  inline static constexpr field_key<HAnimMotion,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<HAnimMotion, std::string> name{
      "name", access_type::input_output};
  inline static constexpr field_key<HAnimMotion, bool> next{
      "next", access_type::input_only};
  inline static constexpr field_key<HAnimMotion, bool> previous{
      "previous", access_type::input_only};
  inline static constexpr field_key<HAnimMotion, std::int32_t> startFrame{
      "startFrame", access_type::input_output};
  inline static constexpr field_key<HAnimMotion,
                                    ::x3d::sai::experimental::float_list>
      values{"values", access_type::input_output};
  inline static constexpr field_key<HAnimMotion, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<HAnimMotion, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<HAnimMotion, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<HAnimMotion, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<HAnimMotion, std::string> style{
      "style", access_type::input_output};
};

} // namespace x3d::sai::experimental::bindings
