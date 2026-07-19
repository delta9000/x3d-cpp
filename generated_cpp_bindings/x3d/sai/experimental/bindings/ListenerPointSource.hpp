#pragma once

// Auto-generated experimental SAI schema binding.
#include "x3d/sai/experimental/X3DSAIBindings.hpp"
#include "x3d/sai/experimental/kernel.hpp"

namespace x3d::sai::experimental::bindings {

struct ListenerPointSource {
  static constexpr std::string_view x3d_name = "ListenerPointSource";
  static constexpr std::string_view schema_fingerprint = model_fingerprint;
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
  inline static constexpr std::array<field_key_descriptor, 22> field_keys{{
      {description.name(), description.kind, description.access()},
      {dopplerEnabled.name(), dopplerEnabled.kind, dopplerEnabled.access()},
      {elapsedTime.name(), elapsedTime.kind, elapsedTime.access()},
      {enabled.name(), enabled.kind, enabled.access()},
      {gain.name(), gain.kind, gain.access()},
      {interauralDistance.name(), interauralDistance.kind,
       interauralDistance.access()},
      {IS.name(), IS.kind, IS.access()},
      {isActive.name(), isActive.kind, isActive.access()},
      {isPaused.name(), isPaused.kind, isPaused.access()},
      {metadata.name(), metadata.kind, metadata.access()},
      {orientation.name(), orientation.kind, orientation.access()},
      {pauseTime.name(), pauseTime.kind, pauseTime.access()},
      {position.name(), position.kind, position.access()},
      {resumeTime.name(), resumeTime.kind, resumeTime.access()},
      {startTime.name(), startTime.kind, startTime.access()},
      {stopTime.name(), stopTime.kind, stopTime.access()},
      {trackCurrentView.name(), trackCurrentView.kind,
       trackCurrentView.access()},
      {DEF.name(), DEF.kind, DEF.access()},
      {USE.name(), USE.kind, USE.access()},
      {class_.name(), class_.kind, class_.access()},
      {id.name(), id.kind, id.access()},
      {style.name(), style.kind, style.access()},
  }};
};

} // namespace x3d::sai::experimental::bindings
