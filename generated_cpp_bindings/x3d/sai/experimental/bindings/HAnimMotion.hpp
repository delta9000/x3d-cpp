#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct HAnimMotion {
  static constexpr std::string_view x3d_name = "HAnimMotion";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
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
  inline static constexpr std::array<field_key_descriptor, 26> field_keys{{
      {channels.name(), channels.kind, channels.access()},
      {channelsEnabled.name(), channelsEnabled.kind, channelsEnabled.access()},
      {cycleTime.name(), cycleTime.kind, cycleTime.access()},
      {description.name(), description.kind, description.access()},
      {elapsedTime.name(), elapsedTime.kind, elapsedTime.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {endFrame.name(), endFrame.kind, endFrame.access()},
      {frameCount.name(), frameCount.kind, frameCount.access()},
      {frameDuration.name(), frameDuration.kind, frameDuration.access()},
      {frameIncrement.name(), frameIncrement.kind, frameIncrement.access()},
      {frameIndex.name(), frameIndex.kind, frameIndex.access()},
      {IS.name(), IS.kind, IS.access()},
      {joints.name(), joints.kind, joints.access()},
      {loa.name(), loa.kind, loa.access()},
      {loop.name(), loop.kind, loop.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {name.name(), name.kind, name.access()},
      {next.name(), next.kind, next.access()},
      {previous.name(), previous.kind, previous.access()},
      {startFrame.name(), startFrame.kind, startFrame.access()},
      {values.name(), values.kind, values.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
