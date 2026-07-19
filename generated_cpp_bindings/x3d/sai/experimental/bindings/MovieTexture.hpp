#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct MovieTexture {
  static constexpr std::string_view x3d_name = "MovieTexture";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
  inline static constexpr field_key<MovieTexture,
                                    ::x3d::sai::experimental::time_value>
      autoRefresh{"autoRefresh", access_type::input_output};
  inline static constexpr field_key<MovieTexture,
                                    ::x3d::sai::experimental::time_value>
      autoRefreshTimeLimit{"autoRefreshTimeLimit", access_type::input_output};
  inline static constexpr field_key<MovieTexture, std::string> description{
      "description", access_type::input_output};
  inline static constexpr field_key<MovieTexture,
                                    ::x3d::sai::experimental::time_value>
      duration_changed{"duration_changed", access_type::output_only};
  inline static constexpr field_key<MovieTexture,
                                    ::x3d::sai::experimental::time_value>
      elapsedTime{"elapsedTime", access_type::output_only};
  inline static constexpr field_key<MovieTexture, bool> enabled{
      "enabled", access_type::input_output};
  inline static constexpr field_key<MovieTexture, float> gain{
      "gain", access_type::input_output};
  inline static constexpr field_key<MovieTexture,
                                    ::x3d::sai::experimental::node_id>
      IS{"IS", access_type::input_output};
  inline static constexpr field_key<MovieTexture, bool> isActive{
      "isActive", access_type::output_only};
  inline static constexpr field_key<MovieTexture, bool> isPaused{
      "isPaused", access_type::output_only};
  inline static constexpr field_key<MovieTexture, bool> load{
      "load", access_type::input_output};
  inline static constexpr field_key<MovieTexture, bool> loop{
      "loop", access_type::input_output};
  inline static constexpr field_key<MovieTexture,
                                    ::x3d::sai::experimental::node_id>
      metadata{"metadata", access_type::input_output};
  inline static constexpr field_key<MovieTexture,
                                    ::x3d::sai::experimental::time_value>
      pauseTime{"pauseTime", access_type::input_output};
  inline static constexpr field_key<MovieTexture, float> pitch{
      "pitch", access_type::input_output};
  inline static constexpr field_key<MovieTexture, bool> repeatS{
      "repeatS", access_type::initialize_only};
  inline static constexpr field_key<MovieTexture, bool> repeatT{
      "repeatT", access_type::initialize_only};
  inline static constexpr field_key<MovieTexture,
                                    ::x3d::sai::experimental::time_value>
      resumeTime{"resumeTime", access_type::input_output};
  inline static constexpr field_key<MovieTexture, float> speed{
      "speed", access_type::input_output};
  inline static constexpr field_key<MovieTexture,
                                    ::x3d::sai::experimental::time_value>
      startTime{"startTime", access_type::input_output};
  inline static constexpr field_key<MovieTexture,
                                    ::x3d::sai::experimental::time_value>
      stopTime{"stopTime", access_type::input_output};
  inline static constexpr field_key<MovieTexture,
                                    ::x3d::sai::experimental::node_id>
      textureProperties{"textureProperties", access_type::initialize_only};
  inline static constexpr field_key<MovieTexture,
                                    ::x3d::sai::experimental::string_list>
      url{"url", access_type::input_output};
  inline static constexpr field_key<MovieTexture, std::string> DEF{
      "DEF", access_type::input_output};
  inline static constexpr field_key<MovieTexture, std::string> USE{
      "USE", access_type::input_output};
  inline static constexpr field_key<MovieTexture, std::string> class_{
      "class", access_type::input_output};
  inline static constexpr field_key<MovieTexture, std::string> id{
      "id", access_type::input_output};
  inline static constexpr field_key<MovieTexture, std::string> style{
      "style", access_type::input_output};
  inline static constexpr std::array<field_key_descriptor, 28> field_keys{{
      {autoRefresh.name(), autoRefresh.kind, autoRefresh.access()},
      {autoRefreshTimeLimit.name(), autoRefreshTimeLimit.kind,
       autoRefreshTimeLimit.access()},
      {description.name(), description.kind, description.access()},
      {duration_changed.name(), duration_changed.kind,
       duration_changed.access()},
      {elapsedTime.name(), elapsedTime.kind, elapsedTime.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {gain.name(), gain.kind, gain.access()},
      {IS.name(), IS.kind, IS.access()},
      {isActive.name(), isActive.kind, isActive.access()},
      {isPaused.name(), isPaused.kind, isPaused.access()},
      {load.name(), load.kind, load.access()},
      {loop.name(), loop.kind, loop.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {pauseTime.name(), pauseTime.kind, pauseTime.access()},
      {pitch.name(), pitch.kind, pitch.access()},
      {repeatS.name(), repeatS.kind, repeatS.access()},
      {repeatT.name(), repeatT.kind, repeatT.access()},
      {resumeTime.name(), resumeTime.kind, resumeTime.access()},
      {speed.name(), speed.kind, speed.access()},
      {startTime.name(), startTime.kind, startTime.access()},
      {stopTime.name(), stopTime.kind, stopTime.access()},
      {textureProperties.name(), textureProperties.kind,
       textureProperties.access()},
      {url.name(), url.kind, url.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
